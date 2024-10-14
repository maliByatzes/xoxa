/* main.c */

#include "server.h"

int main()
{
#if defined(_WIN32)
  WSAData d;
  if (WSAStartup(MAKEWORD(2,2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return 1;
  }
#endif

  SOCKET connfd = create_server();
  run_server(connfd);

#if defined(_WIN32)
  WSACleanup();
#endif

  return 0;
}
