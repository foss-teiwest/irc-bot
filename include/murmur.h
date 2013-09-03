#ifndef MURMUR_H
#define MURMUR_H

#include <sys/types.h>
#include <stdbool.h>
#include "irc.h"

/**
 * @file murmur.h
 * Murmur's ICE userConnected/userDisconnected callbacks-notifications adder and listener.
 * Works only with a Murmur that supports ICE's version >=3.4 and runs at localhost.
 * Author: Charalampos Kostas <root@charkost.gr>
 */

#define CB_LISTEN_PORT 65535
#define CB_LISTEN_PORT_S "65535"
#define MURMUR_PORT "6502"
#define ICE_ISA_REPLY_PACKET_SIZE 26
#define ADDCALLBACK_REPLY_PACKET_SIZE 25
#define REMOVECALLBACK_REPLY_PACKET_SIZE 25
#define VALIDATE_CONNECTION_PACKET_SIZE 14
#define READ_BUFFER_SIZE 512

/** Add callbacks */
bool add_murmur_callbacks(const char *port);

/** Validate murmur's server connection */
ssize_t validate_murmur_connection(int murm_acceptfd);

/** Listen for callbacks */
ssize_t listen_murmur_callbacks(Irc server, int murm_acceptfd);

#endif
