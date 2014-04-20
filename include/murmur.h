#ifndef MURMUR_H
#define MURMUR_H

/**
 * @file murmur.h
 * Murmur's ICE userConnected/userDisconnected callbacks-notifications adder & listener,
 * getUsers request & response parsing in order to print userlist.
 * Works only with a Murmur that supports ICE's version >=3.4 and runs at localhost.
 * Author: Charalampos Kostas <root@charkost.gr>
 */

#include <stdbool.h>
#include "irc.h"

#define CB_LISTEN_PORT 65535
#define CB_LISTEN_PORT_S "65535"
#define ICE_ISA_REPLY_PACKET_SIZE 26
#define ADDCALLBACK_REPLY_PACKET_SIZE 25
#define REMOVECALLBACK_REPLY_PACKET_SIZE 25
#define VALIDATE_CONNECTION_PACKET_SIZE 14
#define READ_BUFFER_SIZE 512
#define USERLIST_BUFFER_SIZE 4096

/** Add callbacks */
bool add_murmur_callbacks(const char *port);

/** Returns mumble user list
  * @warning  String must be freed */
char *fetch_murmur_users(void);

/** Accept and check that the incoming connection is valid
 *  @return non-blocking socket */
int accept_murmur_connection(int murm_listenfd);

/** Listen for callbacks */
bool listen_murmur_callbacks(Irc server, int murm_acceptfd);

#endif

