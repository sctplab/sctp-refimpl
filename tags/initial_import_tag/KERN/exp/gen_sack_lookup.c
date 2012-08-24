#include <sys/types.h>
#include <stdio.h>


#define SCTP_MAX_GAPS_INARRAY 4

struct sctp_gap_ack_block {
	uint16_t start;		/* Gap Ack block start */
	uint16_t end;		/* Gap Ack block end */
};

struct sack_track {
	uint8_t right_edge;
	uint8_t left_edge;
	uint8_t num_entries;
	uint8_t spare;
	struct sctp_gap_ack_block gaps[SCTP_MAX_GAPS_INARRAY];
};


int
main(int argc, char **argv)
{
	struct sack_track sack_array;
	int num, i, bits, at, copyof, j;
	int seeing_one;

	if (argc != 2) {
out:
		printf("Use %s num (where 0 < num <= 256) \n",
		    argv[0]);
		exit(0);
	}
	num = strtol(argv[1], NULL, 0);

	if ((num < 0) || (num > 256)) {
		goto out;
	}
	printf("struct sack_track sack_array[%d] = {\n", num);
	for (i = 0; i < num; i++) {
		memset(&sack_array, 0, sizeof(sack_array));
		at = 0;
		if (i & 0x01) {
			sack_array.right_edge = 1;
		}
		if (i & 0x80) {
			sack_array.left_edge = 1;
		}
		copyof = i;
		seeing_one = 0;
		for (bits = 0; bits < 8; bits++) {
			if (seeing_one == 0) {
				/* seeing a zero last */
				if (copyof & 0x01) {
					/* mark the spot and transition */
					seeing_one = 1;
					sack_array.gaps[at].start = bits;
				}
			} else {
				/* seeing a one last */
				if ((copyof & 0x01) == 0) {
					/* transitioned to zero mark end */
					sack_array.gaps[at].end = bits - 1;
					seeing_one = 0;
					at++;
				}
			}
			copyof >>= 1;
		}
		if (seeing_one) {
			/* If at the end I was seeing one's record it */
			sack_array.gaps[at].end = 7;
			at++;
		}
		sack_array.num_entries = at;
		printf("{%d, %d, %d, %d,     /* 0x%2.2x */\n",
		    sack_array.right_edge,
		    sack_array.left_edge,
		    sack_array.num_entries,
		    sack_array.spare,
		    (uint32_t) i);
		printf(" { { %d, %d},\n",
		    sack_array.gaps[0].start,
		    sack_array.gaps[0].end);
		printf("   { %d, %d},\n",
		    sack_array.gaps[1].start,
		    sack_array.gaps[1].end);
		printf("   { %d, %d},\n",
		    sack_array.gaps[2].start,
		    sack_array.gaps[2].end);
		printf("   { %d, %d}\n",
		    sack_array.gaps[3].start,
		    sack_array.gaps[3].end);
		printf(" }\n");
		if (i == (num - 1)) {
			printf("}\n");
		} else {
			printf("},\n");
		}

	}
	printf("};\n");
}
