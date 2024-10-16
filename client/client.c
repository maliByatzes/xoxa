/* client.c */

#include "xoxa.h"

int main(int argc, char ** argv)
{
#if defined(_WIN32)
  WSAData d;
  if (WSAStartup(MAKEWORD(2,2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return 1;
  }
#endif

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
    return 1;
  }

  printf("Configuring remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *peer_address;
  if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  printf("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
              address_buffer, sizeof(address_buffer),
              service_buffer, sizeof(service_buffer),
              NI_NUMERICHOST);
  printf("%s %s\n", address_buffer, service_buffer);

  printf("Creating a new socket...\n");
  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  printf("Connecting...\n");
  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  printf("Connected.\n"
         "Enter data below. The request must structured as followed: \n"
         "First line: sendto\n"
         "Second line: <destination ip> <destination port>\n"
         "Third line: <message>\n"
         "OR\n"
         "First line: list\n"
  );

  while (1) {

    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(socket_peer, &reads);
#if !defined(_WIN32)
    FD_SET(0, &reads);
#endif

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    if (select(socket_peer+1, &reads, 0, 0, &timeout) < 0) {
      fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
    }

    if (FD_ISSET(socket_peer, &reads)) {
      char read[4096];
      int bytes_read = recv(socket_peer, read, 4096, 0);
      if (bytes_read < 1) {
        printf("Connection closed by peer.\n");
        break;
      }

      printf("Received (%d bytes): %.*s", bytes_read, bytes_read, read);
    }

#if defined(_WIN32)
    if (_kbhit()) {
#else
    if (FD_ISSET(0, &reads)) {
#endif

        char action[50];
        char destination[200];
        char msg[8192];
        char buffer[512];

        // read action until NL
        if (!fgets(action, 50, stdin)) break;
        strncat(msg, action, strlen(action));

        // "sendto" action
        if (strcmp(action, "sendto\n") == 0) {
          // read destination until NL
          if (!fgets(destination, 200, stdin)) break;
          strncat(msg, destination, strlen(destination));

          // read message until empty line
          while(1) {
            if (fgets(buffer, 512, stdin) != NULL) {
              buffer[strcspn(buffer, "\n")] = '\0';

              if (strlen(buffer) == 0) break;

              strncat(msg, buffer, strlen(buffer));
            }
          }

          int bytes_sent = send(socket_peer, msg, strlen(msg), 0);
          printf("Sent %d bytes.\n", bytes_sent);
         }
         // "list" action
        else if (strcmp(action, "list\n") == 0) {}

        else {
          printf("Invalid action. Available action => `sendto`, `list`");
        }

    }

  }

  printf("Closing socket.\n");
  CLOSESOCKET(socket_peer);

#if defined(_WIN32)
  WSACleanup();
#endif

  return 0;
}
