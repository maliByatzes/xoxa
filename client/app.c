/* app.c */

#include "app.h"
#include "config.h"

App *new_app() 
{
  App *app = (App *)malloc(sizeof(App));

  app->message_count = 0;
  app->client_count = 0;
  app->selected_client = -1;

  app->cfg = load_config();

  if (!validate_config(app->cfg)) {
    free_config(app->cfg);
    exit(1);
  }

  return app;
}

void toggle_active_window(App *app) 
{
  switch (app->current_active_win) {
  case CAW_Message:
    app->current_active_win = CAW_Message;
    break;
  case CAW_Input:
    app->current_active_win = CAW_Input;
    break;
  case CAW_Sidebar:
    app->current_active_win = CAW_Sidebar;
    break;
  case CAW_Status:
    app->current_active_win = CAW_Status;
    break;
  }
}

void free_app(App *app) 
{
  free_config(app->cfg);
  free(app);
}