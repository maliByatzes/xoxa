/* server.c */

#include "server.h"
#include "models.h"
#include "xoxa.h"

Client *newClient(SOCKET clientfd, char *name, char *hostname, char *ip_address, char *port)
{
  Client *this = (Client*)malloc(sizeof(Client));

  this->clientfd = clientfd;
  strcpy(this->name, name);
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
    log_message(error, "Position out of range.");
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
    log_message(warn, "List is empty.");
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
    log_message(warn, "List is empty.");
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
    log_message(warn, "List is empty.");
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
    log_message(error, "Position out of range.");
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
  log_message(info, "Configuring local address...");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  if (getaddrinfo(NULL, "8080", &hints, &bind_address)) {
    log_message(error, "getaddrinfo() failed. (%d)", GETSOCKETERRNO());
    exit(1);
  }

  log_message(info, "Creating a new socket...");
  SOCKET connfd;
  connfd = socket(bind_address->ai_family, bind_address->ai_socktype,
                  bind_address->ai_protocol);
  if (!ISVALIDSOCKET(connfd)) {
    log_message(error, "socket() failed. (%d)\n", GETSOCKETERRNO());
    exit(1);
  }

  log_message(info, "Binding address to socket...");
  if (bind(connfd, bind_address->ai_addr, bind_address->ai_addrlen)) {
    log_message(error, "bind failed. (%d)\n", GETSOCKETERRNO());
    exit(1);
  }
  freeaddrinfo(bind_address);

  return connfd;
}

void run_server(SOCKET connfd, sqlite3 *db)
{
  NodeClient *head = NULL;
  int err_code, create_user = 1;

  log_message(info, "Listening...");
  if (listen(connfd, BACKLOG) < 0) {
    log_message(error, "listen() failed. (%d)\n", GETSOCKETERRNO());
    exit(1);
  }

  fd_set master;
  FD_ZERO(&master);
  FD_SET(connfd, &master);
  SOCKET max_socket = connfd;

  log_message(info, "Waiting for connections.");

  while (1) {

    fd_set reads;
    reads = master;

    if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
      log_message(error, "select() failed. (%d)\n", GETSOCKETERRNO());
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
            log_message(error, "accept() failed. (%d)\n", GETSOCKETERRNO());
            exit(1);
          }

          FD_SET(clientfd, &master);
          if (clientfd > max_socket)
            max_socket = clientfd;

          char client_name[100];
          memset(client_name, 0, sizeof(client_name));
          recv(clientfd, client_name, 100, 0);
          log_message(info, "Client's name: %s", client_name);

          // Check if client's name exists in the users table already
          UserFilter filter = {0};
          filter.name = client_name;
          UserArr *users = getUsers(db, filter, &err_code); 

          if (users) {
            // Don't create a new user later if users->count == 1
            printf("Found %zu users.\n", users->count);

            if (users->count == 1) {
            printf("id: %d, name: %s, created_at: %s, updated_at: %s\n", 
                   users->users[0].id, users->users[0].name,
                   users->users[0].created_at, users->users[0].updated_at);
              create_user = 0;
            }
            
            freeUsersArr(users);
            free(users);
          } else {
            fprintf(stderr, "Failed to get users. Error code: %d\n", err_code);
          }

          char address_buffer[100];
          memset(address_buffer, 0, sizeof(address_buffer));
          char service_buffer[100];
          memset(service_buffer, 0, sizeof(service_buffer));

          getnameinfo((struct sockaddr *)&client_address, client_len,
                      address_buffer, sizeof(address_buffer),
                      service_buffer, sizeof(service_buffer),
                      NI_NUMERICHOST);
          log_message(info, "New connection from %s:%s", address_buffer, service_buffer);

          // Create a new user on the database
          if (create_user) {
            User user = {
              .name = client_name,
            };
            int result = createUser(db, &user);

            if (result != SQLITE_OK) {
              CLOSESOCKET(clientfd);
              exit(1);
            }
          }

          Client *newC = newClient(clientfd, client_name ,address_buffer,
                                   address_buffer, service_buffer);
          insertAtEnd(&head, newC);

        } else {

          char read[8192];
          memset(read, 0, sizeof(read));
          int bits_read = recv(i, read, 8192, 0);
          if (bits_read < 1) {
            FD_CLR(i, &master);
            CLOSESOCKET(i);
            continue;
          }

          log_message(info, "request from client %d: %s", i, read);

          char result[50];
          char *colon_pos = strchr(read, '\n');
          if (colon_pos != NULL) {
            size_t len = colon_pos - read;
            strncpy(result, read, len);
            result[len] = '\0';
          }
          result[strcspn(result, "\n")] = '\0';

          // Handle `sendto` with name ip port and message
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
              char message[7168];
              memset(message, 0, sizeof(message));
              while (j < bits_read) {
                strncat(message, &read[j], sizeof(char));
                ++j;
              }

              int bits_sent = send(destfd, message, strlen(message), 0);
              log_message(info, "Sent (%d) bits", bits_sent);
            } else {
              char msg[] = "user not found.\n";
              send(i, msg, strlen(msg), 0);
            }
          }

          // Handle `list` to return all connected cients
          if (strcmp(result, "list") == 0) {
            char response[5120];
            memset(response, 0, sizeof(response));
            NodeClient *temp = head;

            strncat(response, "Client List:\n", 13);
            while (temp != NULL) {
              if (temp->data->clientfd == i) {
                temp = temp->next;
              } else {
                char res[512];
                snprintf(res, sizeof(res), "%s %s:%s\n",
                        temp->data->name,
                        temp->data->ip_address,
                        temp->data->port);

                strncat(response, res, strlen(res));
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

void log_message(enum Level level, const char *format, ...)
{
  va_list args;
  va_start(args, format);

  time_t current_time = time(NULL);
  char timestamp[26];
  strncpy(timestamp, asctime(gmtime(&current_time)), sizeof(timestamp));
  timestamp[24] = '\0';

  switch (level) {
  case info:
    printf("\033[32m[INFO] [%s]\033[0m - ", timestamp);
    vprintf(format, args);
    printf("\n");
    break;

  case warn:
    printf("\033[33m[WARN] [%s]\033[0m - ", timestamp);
    vprintf(format, args);
    printf("\n");
    break;

  case error:
    fprintf(stderr, "\033]31m[ERROR] [%s]\033[0m - ", timestamp);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    break;

  }

  va_end(args);
}
