#include <sys/types.h>
#include <stdio.h>


#define SCTP_IS_TSN_PRESENT(arry, gap) ((arry[(gap >> 3)] >> (gap & 0x07)) & 0x01)

int
main(int argc, char **argv)
{
	int i, map, m_size = 8, all_ones;
	uint8_t map_array[2];

	map_array[1] = 0;
	printf("int8_t sctp_map_lookup_tab[256] = {\n");
	printf("    ");
	for (map = 0; map <= 0xff; map++) {
		map_array[0] = map;
		all_ones = 1;
		for (i = 0; i < m_size; i++) {
			if (!SCTP_IS_TSN_PRESENT(map_array, i)) {
				/*
				 * Ok we found the first place that we are
				 * missing a TSN.
				 */
				printf("%d, ", (i - 1));
				if (((map + 1) % 8) == 0) {
					printf("\n    ");
				}
				all_ones = 0;
				break;
			}
		}
		if (all_ones) {
			printf("%d, ", (i - 1));
			if (((map + 1) % 8) == 0) {
				printf("\n    ");
			}
		}
	}
	printf("} ;\n");
}
