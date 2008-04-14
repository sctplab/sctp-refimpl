struct sctp_sndrcvinfo {
 uint16_t sinfo_stream;
 uint16_t sinfo_ssn;
 uint16_t sinfo_flags;
 uint16_t sinfo_pr_policy;
 uint32_t sinfo_ppid;
 uint32_t sinfo_context;
 uint32_t sinfo_timetolive;
 uint32_t sinfo_tsn;
 uint32_t sinfo_cumtsn;
 sctp_assoc_t sinfo_assoc_id;
};
