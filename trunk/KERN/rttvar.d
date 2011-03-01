
sctp:cwnd:net:rttvar
{
	printf("%d %d %d %ld %ld %ld %ld",
		   (uint16_t)((arg4 >> 16) & 0x000000000000ffff),
		   (uint32_t)((arg4 >> 32) & 0x00000000ffffffff),
	       	   (uint16_t)(arg4 & 0x000000000000ffff),
		   ((arg1 >> 32) & 0x00000000ffffffff),
		   (arg1 & 0x00000000ffffffff),
		   arg2,
		   arg3);
}
