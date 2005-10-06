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
		   int mapping_array_size, 
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

	m_size = (mapping_array_size << 3);

	if (highest_tsn_inside_map >= mapping_array_base_tsn) {
			maxi = (highest_tsn_inside_map - mapping_array_base_tsn);
	} else {
		maxi = (highest_tsn_inside_map  + (MAX_TSN - mapping_array_base_tsn) + 1);
	}
	if (maxi > m_size) {
		/* impossible but who knows, someone is playing with us  :> */
		return (0);
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
