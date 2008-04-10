struct sctp_paddr_change {
 uint16_t spc_type;
 uint16_t spc_flags;
 uint32_t spc_length;
 struct sockaddr_storage spc_aaddr;
 uint32_t spc_state;
 uint32_t spc_error;
 sctp_assoc_t spc_assoc_id;
 uint8_t spc_padding[4];
};
