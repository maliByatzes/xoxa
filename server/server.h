/* server.h */

#ifndef __server_h
#define __server_h

#include "xoxa.h"

#include <stddef.h>
#include <stdlib.h>

#define BACKLOG 10

typedef struct Client_ {
  SOCKET clientfd;
  char name[100];
  char hostname[100];
  char ip_address[100];
  char port[100];
} Client;

typedef struct NodeClient_ {
  Client *data;
  struct NodeClient_ *next;
} NodeClient;

Client *newClient(SOCKET /* clientfd */, char */* hostname */,
                  char */* ip_address */, char */* port */);
void printClient(Client *client);
void deleteClient(Client */* this */);

NodeClient *newNodeClient(Client */* data */);
void insertAtFirst(NodeClient **/* head */, Client */* data */);
void insertAtEnd(NodeClient **/* head */, Client */* data */);
void insertAtPosition(NodeClient **/* head */, Client */* data */, int /* position */);
void deleteFromFirst(NodeClient **/* head */);
void deleteFromEnd(NodeClient **/* head */);
void deleteAtPosition(NodeClient **/* head */, int /* position */);
void printNodes(NodeClient */* head */);

SOCKET create_server();
void run_server(SOCKET /* connfd */);

#endif
