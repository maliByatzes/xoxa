/* main.c */

#include "client.h"

// TODO: Use the gift of enum to have a status on the current window (active window)
// TODO: Use struct to keep track of the application entire state
// TODO: Run the `UI` in an infinite loop to always be listening for events from server && client input

int main()
{
  Application app;
  app = init_application();

  run_application(&app);

  return 0;
}
