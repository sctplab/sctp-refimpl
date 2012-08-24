struct sctp_event_subscribe {
 uint8_t sctp_data_io_event;
 uint8_t sctp_association_event;
 uint8_t sctp_address_event;
 uint8_t sctp_send_failure_event;
 uint8_t sctp_peer_error_event;
 uint8_t sctp_shutdown_event;
 uint8_t sctp_partial_delivery_event;
 uint8_t sctp_adaptation_layer_event;
 uint8_t sctp_authentication_event;
#ifdef __FreeBSD__
 uint8_t sctp_stream_reset_events;
#endif
};
