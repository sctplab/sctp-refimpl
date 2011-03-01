
sctp:cwnd:net:rttstep
{
	/* probepoint cwnd ret lbw nbw  bwrtt nrtt scwnd sc lstate*/ 
	printf("%d cwnd:%d ret:%d lbw:%d nbw:%d bwrtt:%d nrtt:%d scwnd:%d stepc:%d lstate:%d ",
		   (uint16_t)((arg4 >> 16) & 0x000000000000ffff),
		   (uint32_t)((arg4 >> 32) & 0x00000000ffffffff),
	       	   (uint16_t)(arg4 & 0x000000000000ffff),

		   ((arg1 >> 32) & 0x00000000ffffffff),
		   (arg1 & 0x00000000ffffffff),

		   ((arg2 >> 32) & 0x00000000ffffffff),
		   (arg2 & 0x00000000ffffffff),

		   ((arg3 >> 32) & 0x00000000ffffffff),
		   ((arg3 >> 16) & 0x000000000000ffff),
		   (arg3 & 0x000000000000ffff));

}
