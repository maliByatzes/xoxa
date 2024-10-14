/* server.c */

#include "server.h"

Client *newClient(SOCKET clientfd, char *hostname, char *ip_address, char *port)
{
  Client *this = (Client*)malloc(sizeof(Client));

  this->clientfd = clientfd;
  this->hostname = hostname;
  this->ip_address = ip_address;
  this->port = port;

  return this;
}

void deleteClient(Client *this)
{
  free(this);
}

NodeClient *newNodeClient(Client *data)
{
  NodeClient *newNC = (NodeClient*)malloc(sizeof(NodeClient));
  newNC->data = data;
  newNC->next = NULL;
  return newNC;
}

void insertAtFirst(NodeClient **head, Client *data)
{
  NodeClient *newNC = newNodeClient(data);
  newNC->next = *head;
  *head = newNC;
}

void insertAtEnd(NodeClient **head, Client *data)
{
  NodeClient *newNC = newNodeClient(data);
  if (*head == NULL) {
    *head = newNC;
    return;
  }

  NodeClient *temp = *head;
  while (temp->next != NULL)
    temp = temp->next;

  temp->next = newNC;
}

void insertAtPosition(NodeClient **head, Client *data, int position)
{
  NodeClient *newNC = newNodeClient(data);
  if (position == 0) {
    insertAtFirst(head, data);
    return;
  }

  NodeClient *temp = *head;
  for (int i = 0; temp != NULL && i < position; i++)
    temp = temp->next;

  if (temp == NULL) {
    fprintf(stderr, "Position out of range\n");
    free(newNC->data);
    free(newNC);
    return;
  }

  newNC->next = temp->next;
  temp->next = newNC;
}

void deleteFromFirst(NodeClient **head)
{
  if (*head == NULL) {
    printf("List is empty\n");
    return;
  }

  NodeClient *temp = *head;
  *head = temp->next;
  free(temp->data);
  free(temp);
}

void deleteFromEnd(NodeClient **head)
{
  if (*head == NULL) {
    printf("List is empty\n");
    return;
  }

  NodeClient *temp = *head;
  if (temp->next == NULL) {
    free(temp->data);
    free(temp);
    *head = NULL;
    return;
  }

  while (temp->next->next != NULL)
    temp = temp->next;

  free(temp->next->data);
  free(temp->next);

  temp->next = NULL;
}

void deleteAtPosition(NodeClient **head, int position)
{
  if (*head == NULL) {
    printf("List is empty\n");
    return;
  }

  NodeClient *temp = *head;
  if (position == 0) {
    deleteFromFirst(head);
    return;
  }

  for (int i = 0; temp != NULL && i < position; i++)
    temp = temp->next;

  if (temp == NULL || temp->next == NULL) {
    fprintf(stderr, "Position out of range\n");
    return;
  }

  NodeClient *next = temp->next->next;
  free(temp->next->data);
  free(temp->next);

  temp->next = next;
}

SOCKET create_server()
{
  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  if (getaddrinfo(NULL, "8080", &hints, &bind_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    exit(1);
  }

  printf("Creating a new socket...\n");
  SOCKET connfd;
  connfd = socket(bind_address->ai_family, bind_address->ai_socktype,
                  bind_address->ai_protocol);
  if (!ISVALIDSOCKET(connfd)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    exit(1);
  }

  printf("Binding address to socket...\n");
  if (bind(connfd, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind failed. (%d)\n", GETSOCKETERRNO());
    exit(1);
  }
  freeaddrinfo(bind_address);

  return connfd;
}

void run_server(SOCKET connfd, NodeClient *head)
{
  printf("Listening...\n");
  if (listen(connfd, BACKLOG) < 0) {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    exit(1);
  }

  fd_set master;
  FD_ZERO(&master);
  FD_SET(connfd, &master);
  SOCKET max_socket = connfd;

  printf("Waiting for connections");

  while (1) {

    fd_set reads;
    reads = master;

    if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
      fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
      exit(1);
    }

    SOCKET i;
    for (i = 1; i <= max_socket; ++i) {
      if (FD_ISSET(i, &reads)) {

        if (i == connfd) {
          struct sockaddr_storage client_address;
          socklen_t client_len = sizeof(client_address);
          SOCKET clientfd = accept(connfd, (struct sockaddr *)&client_address,
                                   &client_len);
          if (!ISVALIDSOCKET(clientfd)) {
            fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
            exit(1);
          }

          FD_SET(clientfd, &master);
          if (clientfd > max_socket)
            max_socket = clientfd;

          char address_buffer[100];
          char service_buffer[100];
          getnameinfo((struct sockaddr *)&client_address, client_len,
                      address_buffer, sizeof(address_buffer),
                      service_buffer, sizeof(service_buffer),
                      NI_NUMERICHOST);
          printf("New connection from %s:%s\n", address_buffer, service_buffer);

          Client *newC = newClient(clientfd, address_buffer, address_buffer,
                                   service_buffer);
          insertAtEnd(&head, newC);
        } else {
          // Handle reading request from client
          // Read in ip_address and port
          // Read in the message
          // Search the linked for the combination of ip_address & port
          // Send the message to that clientfd if found
        }

      }
    }

  }
}
