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

  NodeClient *head = NULL;
  run_server(connfd, head);

  while (head->next != NULL) {
    deleteFromEnd(&head);
    head = head->next;
  }

  return 0;
}
