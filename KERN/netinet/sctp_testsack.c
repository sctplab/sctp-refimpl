#include <sys/types.h>
#include <stdio.h>
#include "sctp_sack.h"
#include <netinet/sctp.h>

#define SCTP_IS_TSN_PRESENT(arry, gap) ((arry[(gap >> 3)] >> (gap & 0x07)) & 0x01)
#define SCTP_SET_TSN_PRESENT(arry, gap) (arry[(gap >> 3)] |= (0x01 << ((gap & 0x07))))
#define SCTP_UNSET_TSN_PRESENT(arry, gap) (arry[(gap >> 3)] &= ((~(0x01 << ((gap & 0x07)))) & 0xff))
#define MAX_TSN 0xfffffff

struct sctp_sack {
	u_int32_t cum_tsn_ack;		/* cumulative TSN Ack */
	u_int32_t a_rwnd;		/* updated a_rwnd of sender */
	u_int16_t num_gap_ack_blks;	/* number of Gap Ack blocks */
	u_int16_t num_dup_tsns;		/* number of duplicate TSNs */
	/* struct sctp_gap_ack_block's follow */
	/* u_int32_t duplicate_tsn's follow */
};

struct sctp_sack_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_sack sack;
};


int
sctp_generate_sack(u_int8_t *mapping_array, 
		   u_int32_t highest_tsn_inside_map,
		   u_int32_t mapping_array_base_tsn,
		   u_int32_t cumulative_tsn,
		   struct sctp_sack_chunk *sack)
{
	struct sctp_gap_ack_block *gap_descriptor;
	int num_gap_discriptor = 0;
	int seeing_ones,i;
	int start, maxi, m_size;

	start = maxi = 0;
	gap_descriptor = (struct sctp_gap_ack_block *)((caddr_t)sack + sizeof(struct sctp_sack_chunk));
	seeing_ones = 0;

	if (highest_tsn_inside_map >= mapping_array_base_tsn) {
			maxi = (highest_tsn_inside_map - mapping_array_base_tsn);
	} else {
		maxi = (highest_tsn_inside_map  + (MAX_TSN - mapping_array_base_tsn) + 1);
	}
	if (cumulative_tsn >= mapping_array_base_tsn) {
		start = (cumulative_tsn - mapping_array_base_tsn);
	} else {
		/* Set it so we start at 0 */
		start = -1;
	}
	start++;
	for (i = start; i <= maxi; i++) {
		if (seeing_ones) {
			/* while seeing Ones I must
			 * transition back to 0 before
			 * finding the next gap
			 */
			if (SCTP_IS_TSN_PRESENT(mapping_array, i) == 0) {
				gap_descriptor->end = htons(((uint16_t)(i-start)));
				num_gap_discriptor++;
				gap_descriptor++;
				seeing_ones = 0;
			}
		} else {
			if (SCTP_IS_TSN_PRESENT(mapping_array, i)) {
				gap_descriptor->start = htons(((uint16_t)(i+1-start)));
				/* advance struct to next pointer */
				seeing_ones = 1;
			}
		}
	}
	if ((SCTP_IS_TSN_PRESENT(mapping_array, maxi))  &&
	    (seeing_ones)) {
		/* special case where the array is all 1's
		 * to the end of the array 
		 */
		gap_descriptor->end = htons(((uint16_t)((i-start))));
		gap_descriptor++;
		num_gap_discriptor++;
	}
	return(num_gap_discriptor);
}


int
sctp_generate_sack_new(u_int8_t *mapping_array, 
		       u_int32_t highest_tsn_inside_map,
		       u_int32_t mapping_array_base_tsn,
		       u_int32_t cumulative_tsn,
		       struct sctp_sack_chunk *sack)
{
	struct sctp_gap_ack_block *gap_descriptor;
	int num_gap_discriptor = 0;	
	struct sack_track *selector;
	int siz, offset, i,j, jstart;
	int mergeable=0;
	gap_descriptor = (struct sctp_gap_ack_block *)((caddr_t)sack + sizeof(struct sctp_sack_chunk));
	/* calculate the number of bufs to work on */
	siz = ((highest_tsn_inside_map - mapping_array_base_tsn) + 7)/ 8;
	if(cumulative_tsn < mapping_array_base_tsn) {
		offset = 1;
		/* 
		 * cum-ack behind the mapping array, so we
		 * start and use all entries.
		 */
 		jstart = 0;
	} else {
		offset = cumulative_tsn - mapping_array_base_tsn; 
		/* 
		 * we skip the first one when the cum-ack is at or
		 * above the mapping array base.
		 */
		jstart = 1;
	}

	for(i=0; i <siz; i++) {
		selector = &sack_array[mapping_array[i]];
		for(j=jstart; j<selector->num_entries; j++) {
			if((mergeable == 0) || (selector->right_edge == 0)) {
				gap_descriptor->start = htons((selector->gap[j].start + offset));
			}
			gap_descriptor->end = htons((selector->gap[j].end + offset));
			if ((mergeable == 0) || (selector->right_edge == 0)) {
				num_gap_discriptor++;
				gap_descriptor++;
			}
		}
		if(selector->left_edge) {
			mergeable = 1;
		} else {
			mergeable = 0;
		}
		jstart = 0;
		offset += 8;
	}
	if(mergable) {
		/* 
		 * The very end was all mergable, all the
		 * way through to the end, so we never incremented
		 * the last descriptor.
		 */
		num_gap_discriptor++;
		gap_descriptor++;

	}
	return(num_gap_discriptor);	
}

int
main(int argc, char **argv)
{
	u_int8_t sack1_buf[2048];
	u_int8_t sack2_buf[2048];
	struct sctp_sack_chunk *sack1, *sack2;
	int j;
	u_int32_t num_skip=0, num_compared=0, num_diff=0;
	
	u_int8_t mapping_array[32];
	u_int32_t counter, *point;
	u_int32_t highest_tsn_inside_map, mapping_array_base_tsn, cumulative_tsn;

	sack1 = (struct sctp_sack_chunk *)sack1_buf;
	sack2 = (struct sctp_sack_chunk *)sack2_buf;
	point = (u_int32_t *)mapping_array;
	mapping_array_base_tsn = 0x100;
	for(i=0; i<=0xffffffff; i++) {
		memset(sack1_buf, 0, sizeof(sack1_buf));
		memset(sack2_buf, 0, sizeof(sack2_buf));

		/* setup the bit map */
		*point = count;
		/* now figure out where cum-ack is */
		if(SCTP_IS_TSN_PRESENT(mapping_array, 0)) {
			/* figure out where cum-ack is then */
			for(j=0;j<8;j++) {
				if(SCTP_IS_TSN_PRESENT(mapping_array, j)) {
					cumulative_tsn = mapping_array_base_tsn + j;
				}
			}
		} else {
			cumulative_tsn = mapping_array_base_tsn -1;
		}
		/* Now where is the highest TSN present */
		for(j=0; j<32;j++) {
			/* we only have up to 32 bits used */
			if(SCTP_IS_TSN_PRESENT(mapping_array, j)) {
				/* this one is here */
				highest_tsn_inside_map = mapping_array_base_tsn + j;
			}
		}
		if(highest_tsn_inside_map == cumulative_tsn) {
			/* no gap ack blocks, skip this one */
			num_skip++;
			continue;
		}
		num_compared++;
		/* generate gap ack blocks */
		sack1->sack.num_gap_ack_blks = sctp_generate_sack_new(mapping_array, 
								      highest_tsn_inside_map,
								      mapping_array_base_tsn,
								      cumulative_tsn,
								      sack1);

		sack2->sack.num_gap_ack_blks = sctp_generate_sack_new(mapping_array, 
								      highest_tsn_inside_map,
								      mapping_array_base_tsn,
								      cumulative_tsn,
								      sack2);
		if(sack1->sack.num_gap_ack_blks != sack2->sack.num_gap_ack_blks) {
			printf("had a difference with count:%x sack1->num:%d sack2->num:%d\n",
			       i,
			       sack1->sack.num_gap_ack_blks,sack2->sack.num_gap_ack_blks);
			num_diff++;
		} else {
			int cmp_size = ((sack1->sack.num_gap_ack_blks * sizeof(struct sctp_gap_ack_block)) + 
					sizeof(struct sctp_sack_chunk));
		        if(memcmp(sack1, sack2, cmp_size)) {
				printf("Value %x came up with different blocks\n", i);
				num_diff++;
			}
		}
	}
	printf("Total compared:%d total skipped %d total diffs %d\n",
	       num_compared,
	       num_skip,
	       num_diff);
}
