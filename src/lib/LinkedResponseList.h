//
// Created by discord on 5/21/18.
//

#ifndef MOTORDAEMONMEGA_LINKEDRESPONSELIST_H
#define MOTORDAEMONMEGA_LINKEDRESPONSELIST_H

struct linked_response_list
{
    struct linked_response_list* next = nullptr;
    char* data;
    unsigned int resultID;
};

typedef struct linked_response_list LinkedResponseList;

#endif //MOTORDAEMONMEGA_LINKEDRESPONSELIST_H