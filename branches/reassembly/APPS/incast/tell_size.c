#include <stdio.h>
#include <incast_fmt.h>

main()
{
	printf("Size of incast_peer_outrec:%d\n",
		sizeof(struct incast_peer_outrec));
	printf("Size of peer_record:%d\n",
		sizeof(struct peer_record));
	printf("Size of incast_lead_hdr:%d\n",
		sizeof(struct incast_lead_hdr));
	printf("Sizeof elephant_lead_hdr:%d\n",
		sizeof(struct elephant_lead_hdr));
	printf("Sizeof elephant_sink_rec:%d\n",
		sizeof(struct elephant_sink_rec));
}
