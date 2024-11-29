/* main.c */

#include "client.h"

// TODO: use the beautiful gift of threads to separate reading
// keys/input from user to the main/or whatever thread that is
// updating the ui. This will help to not block the ui by
// waiting for input before updating it.

int main()
{
#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2,2), &d)) {
    fprintf(stderr, "Failed to initiliaze.\n");
    return 1;
  }
#endif

  App *app = new_app();

  int result = connect_to_remote(app);
  if (result > 0) {
    free_app(app);
    display_exit_message();
    return 1;
  }
  
  if (run_app(app)) {
    free_app(app);
    display_exit_message();
    return 1;
  }

  update_status(app, "Closing connection...");
  CLOSESOCKET(app->socket_peer);

#if defined (_WIN32)
  WSACleanup();
#endif
  free_app(app);

  display_exit_message();

  return 0;
}
