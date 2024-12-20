/* client.c */

#include "client.h"
#include "app.h"
#include "xoxa.h"

int connect_to_remote(App *app) 
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *peer_address;
  if (getaddrinfo(app->cfg->server_ip, app->cfg->server_port, &hints,
                  &peer_address)) {
    fprintf(stderr, "getaddrinfo() error: %d\n", GETSOCKETERRNO());
    return 1;
  }

  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    fprintf(stderr, "socket() error: %d\n", GETSOCKETERRNO());
    freeaddrinfo(peer_address);
    return 1;
  }

  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    fprintf(stderr, "connect() error: %d\n", GETSOCKETERRNO());
    freeaddrinfo(peer_address);
    return 1;
  }

  freeaddrinfo(peer_address);

  // send client's name
  send(socket_peer, app->cfg->client_name, strlen(app->cfg->client_name), 0);

  update_status(app, "Connected.");

  app->socket_peer = socket_peer;

  return 0;
}

int read_from_socket(App *app) 
{

  fd_set reads;
  FD_ZERO(&reads);
  FD_SET(app->socket_peer, &reads);

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 100000;

  if (select(app->socket_peer + 1, &reads, 0, 0, &timeout) < 0) {
    char status[100];
    snprintf(status, sizeof(status), "select error: %d", GETSOCKETERRNO());
    update_status(app, status);
    return 1;
  }

  // Read data from socket_peer
  if (FD_ISSET(app->socket_peer, &reads)) {
    char read_buffer[8196]; // NOTE: Adjust
    memset(read_buffer, 0, sizeof(read_buffer));
    int bytes_read = recv(app->socket_peer, read_buffer, 4096, 0);
    if (bytes_read < 1) {
      update_status(app, "Connection close by peer.");
      return 1;
    }
    read_buffer[bytes_read] = '\0';

    if (strstr(read_buffer, "Client List:\n") == read_buffer) {
      update_status(app, "Received client list...");
      get_clients(app, read_buffer + 13);
    } else if (strstr(read_buffer, "Messages:\n") == read_buffer) {
      update_status(app, "Received messages...");
      get_messages_for_client(&app->clients[app->selected_client], read_buffer + 10);
    } else {
      add_message(app, read_buffer); 
    }
  }

  return 0;
}

void send_list_cmd(App *app) 
{
  send(app->socket_peer, "list\n", 5, 0);  
}

void send_get_msgs_cmd(App *app) {
  char msg[100];
  memset(msg, 0, sizeof(msg));
  snprintf(msg, sizeof(msg), "messages\n%s\n%s\n", 
           app->cfg->client_name, 
           app->clients[app->selected_client].name);
  send(app->socket_peer, msg, strlen(msg), 0);
}

void get_clients(App *app, const char *list_data) {
  app->client_count = 0;
  char *data_cpy = strdup(list_data);
  char *line = strtok(data_cpy, "\n");

  while (line != NULL && app->client_count < MAX_CLIENTS) {
    // `name ip:port`
    char name[MAX_CLIENT_NAME];
    char ip[16];
    char port[6];

    if (sscanf(line, "%s %[^:]:%s", name, ip, port) == 3) {
      strncpy(app->clients[app->client_count].name, name, MAX_CLIENT_NAME - 1);
      strncpy(app->clients[app->client_count].ip, ip, 15);
      strncpy(app->clients[app->client_count].port, port, 5);
      app->client_count++;
    }

    line = strtok(NULL, "\n");
  }

  free(data_cpy);
}

void get_messages_for_client(Client *client, const char *data) {
  // `from: message\nfrom: message...`
  client->message_count = 0;
  char *data_cpy = strdup(data);
  char *line = strtok(data_cpy, "\n");

  while (line != NULL && client->message_count < MAX_MESSAGES) {
    strncpy(client->messages[client->message_count], line, MAX_MESSAGE_LENGTH - 1);
    client->message_count++;
    
    line = strtok(NULL, "\n");
  }

  free(data_cpy);
}

void add_message(App *app, const char *message) {
  char *msg_cpy = strdup(message);
  char *client_name = strtok(msg_cpy, ":");

  Client *curr_client = NULL;

  for (int i = 0; i < app->client_count; i++) {
    if (strcmp(app->clients[i].name, client_name) == 0) {
      curr_client = &app->clients[i];
    }
  }

  if (curr_client != NULL) {

    if (app->selected_client >= 0) {
      if (curr_client->message_count < MAX_MESSAGES) {
        strncpy(curr_client->messages[curr_client->message_count], message,
                MAX_MESSAGE_LENGTH - 1);
        curr_client->message_count++;
      } else {
        for (int i = 0; i < MAX_MESSAGES - 1; i++) {
          strcpy(curr_client->messages[i], curr_client->messages[i + 1]);
        }
        strncpy(curr_client->messages[MAX_MESSAGES - 1], message,
                MAX_MESSAGE_LENGTH - 1);
      }
    }
  }

  free(msg_cpy);
}
