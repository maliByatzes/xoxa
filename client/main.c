/* main.c */

#include "client.h"

int main()
{
  Application app;
  app = init_application();

  run_application(&app);

  return 0;
}
