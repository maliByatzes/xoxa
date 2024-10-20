/* server.c */

#include "server.h"

Client *newClient(SOCKET clientfd, char *hostname, char *ip_address, char *port)
{
  Client *this = (Client*)malloc(sizeof(Client));

  this->clientfd = clientfd;
  strcpy(this->hostname, hostname);
  strcpy(this->ip_address, ip_address);
  strcpy(this->port, port);

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
    deleteClient(newNC->data);
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
  deleteClient(temp->data);
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
    deleteClient(temp->data);
    free(temp);
    *head = NULL;
    return;
  }

  while (temp->next->next != NULL)
    temp = temp->next;

  deleteClient(temp->next->data);
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
  deleteClient(temp->next->data);
  free(temp->next);

  temp->next = next;
}

void printClient(Client *client)
{
  printf("[\n");
  printf("\tclientfd: %d\n", client->clientfd);
  printf("\thostname: %s\n", client->hostname);
  printf("\tip_address: %s\n", client->ip_address);
  printf("\tport: %s\n", client->port);
  printf("]\n");
}

void printNodes(NodeClient *head)
{
  NodeClient *temp = head;
  while (temp != NULL) {
    printClient(temp->data);
    temp = temp->next;
  }
  printf("NULL\n");
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

void run_server(SOCKET connfd)
{
  NodeClient *head = NULL;

  printf("Listening...\n");
  if (listen(connfd, BACKLOG) < 0) {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    exit(1);
  }

  fd_set master;
  FD_ZERO(&master);
  FD_SET(connfd, &master);
  SOCKET max_socket = connfd;

  printf("Waiting for connections\n");

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
          memset(address_buffer, 0, sizeof(address_buffer));
          char service_buffer[100];
          memset(service_buffer, 0, sizeof(service_buffer));

          getnameinfo((struct sockaddr *)&client_address, client_len,
                      address_buffer, sizeof(address_buffer),
                      service_buffer, sizeof(service_buffer),
                      NI_NUMERICHOST);
          printf("New connection from %s:%s\n", address_buffer, service_buffer);

          Client *newC = newClient(clientfd, address_buffer, address_buffer,
                                   service_buffer);
          insertAtEnd(&head, newC);

        } else {

          char read[8192];
          int bits_read = recv(i, read, 8192, 0);
          if (bits_read < 1) {
            FD_CLR(i, &master);
            CLOSESOCKET(i);
            continue;
          }

          printf("msg from client %d:\n %s\n", i, read);

          char result[50];
          char *colon_pos = strchr(read, '\n');
          if (colon_pos != NULL) {
            size_t len = colon_pos - read;
            strncpy(result, read, len);
            result[len] = '\0';
          }
          result[strcspn(result, "\n")] = '\0';

          // Handle `sendto` with ip:port and message
          if (strcmp(result, "sendto") == 0) {

            char buf[100];
            char address_buffer[100];
            char service_buffer[100];
            int j = 0;

            memset(buf, 0, sizeof(buf));
            while (j < bits_read && read[j] != '\n') ++j;
            ++j;

            while(j < bits_read && read[j] != ' ') {
              strncat(buf, &read[j], sizeof(char));
              ++j;
            }
            ++j;

            memset(address_buffer, 0, sizeof(address_buffer));
            strncpy(address_buffer, buf, strlen(buf));

            memset(buf, 0, sizeof(buf));
            while(j < bits_read && read[j] != '\n') {
              strncat(buf, &read[j], sizeof(char));
              ++j;
            }
            ++j;

            memset(service_buffer, 0, sizeof(service_buffer));
            strncpy(service_buffer, buf, strlen(buf));

            SOCKET destfd = 0;
            NodeClient *temp = head;

            while (temp != NULL) {
              if (strcmp(temp->data->ip_address, address_buffer) == 0) {
                if (strcmp(temp->data->port, service_buffer) == 0) {
                  destfd = temp->data->clientfd;
                }
              }
              temp = temp->next;
            }

            if (destfd != 0) {
              char message[2048];
              memset(message, 0, sizeof(message));
              while (j < bits_read) {
                strncat(message, &read[j], sizeof(char));
                ++j;
              }

              send(destfd, message, strlen(message), 0);
            } else {
              char msg[] = "user not found.\n";
              send(i, msg, strlen(msg), 0);
            }
          }

          // Handle `list` to return all connected cients
          if (strcmp(result, "list") == 0) {
            char response[5120];
            memset(response, 0, sizeof(response));
            int u = 1;
            NodeClient *temp = head;

            while (temp != NULL) {
              if (temp->data->clientfd == i) {
                temp = temp->next;
              } else {
                char res[512];
                snprintf(res, sizeof(res), "User %d: [%s] [%s] [%s]\n",
                        u,
                        temp->data->hostname,
                        temp->data->ip_address,
                        temp->data->port);

                strncat(response, res, strlen(res));
                ++u;
                temp = temp->next;
              }
            }

            send(i, response, strlen(response), 0);
          } // end "list"

        } // end else

      } // end if loop
    } // end for loop

  } // end while(1)

  while (head != NULL)
    deleteFromFirst(&head);
}
