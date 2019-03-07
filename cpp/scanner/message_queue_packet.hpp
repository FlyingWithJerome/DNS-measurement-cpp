#ifndef MESSAGE_QUEUE_PACKET_
#define MESSAGE_QUEUE_PACKET_

typedef struct{
    char ip_address[17];
    char question[70];
    // char hex_form[12];
    // unsigned int ip_in_int;
}message_pack;

#endif