struct sctp_assoc_change {
 uint16_t sac_type;
 uint16_t sac_flags;
 uint32_t sac_length;
 uint16_t sac_state;
 uint16_t sac_error;
 uint16_t sac_outbound_streams;
 uint16_t sac_inbound_streams;
 sctp_assoc_t sac_assoc_id;
};
