#ifndef __pdapi_req_h__
#define __pdapi_req_h__


#define PDAPI_DATA_MESSAGE    1
#define PDAPI_REQUEST_MESSAGE 2
#define PDAPI_END_MESSAGE     3

struct pdapi_request {
  unsigned char request;
  union {  
    uint32_t checksum;
    int size;
    unsigned char data[0];
  }msg;
};


#endif
