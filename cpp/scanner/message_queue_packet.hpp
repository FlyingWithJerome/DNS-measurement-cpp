#ifndef MESSAGE_QUEUE_PACKET_
#define MESSAGE_QUEUE_PACKET_

typedef struct{
    char ip_address[17];
    char question[70];
    int  query_type;
}message_pack;

#endif
