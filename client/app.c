/* app.c */

#include "app.h"
#include "ui.h"
#include "client.h"

App *new_app() 
{
  App *app = (App *)calloc(sizeof(App), 1);

  app->client_count = 0;
  app->selected_client = 0;
  app->current_active_win = CAW_Sidebar;

  app->current_status = NULL;

  app->cfg = load_config();

  if (!validate_config(app->cfg)) {
    free_config(app->cfg);
    exit(1);
  }

  app->clients = (Client *)calloc(sizeof(Client), MAX_CLIENTS);

  return app;
}

int run_app(App *app) 
{
  init_ui();
  
  for (;;) {
    draw_ui(app);
    if (read_from_socket(app)) {
      return 1;
    }

    // Event handling...
    int ch = getch();
    if (ch != ERR) {
      int res = handle_key(app, ch);
      if (res == 1) {
        return 0;
      }
    }
  }

  return 0;
}

int handle_key(App *app, int ch) 
{
  // Global key binds
  switch (ch) {
  case 9:
    toggle_active_window(app);
    break;
  case 'q':
    return 1;
  }

  if (app->current_active_win == CAW_Message) {
    // NOTE: maybe scroll up and down messages on the screen ??
    switch (ch) {
    default:
      break;
    }
  } 
  else if (app->current_active_win == CAW_Input) {    
    
    // automatically get input
    while (1) {
      char *line = read_input(app);
      if (strlen(line) == 0) break;
      update_status(app, line);// temp: just to see the message
    }
    
  }
  else if (app->current_active_win == CAW_Sidebar) {
    switch (ch) {
    case KEY_UP:
      if (app->selected_client > 0) {
        app->selected_client--;
        if (app->selected_client < app->sidebar_scroll) {
          app->sidebar_scroll--;
        }
      }
      break;
    case KEY_DOWN:
      if (app->selected_client < app->client_count - 1) {
        app->selected_client++;
        if (app->selected_client >= app->sidebar_scroll) {
          app->sidebar_scroll++;
        }
      }
      break;
    }
  }
  
  return 0;
}

void toggle_active_window(App *app) 
{
  switch (app->current_active_win) {
  case CAW_Message:
    app->current_active_win = CAW_Input;
    break;
  case CAW_Input:
    app->current_active_win = CAW_Status;
    break;
  case CAW_Sidebar:
    app->current_active_win = CAW_Message;
    break;
  case CAW_Status:
    app->current_active_win = CAW_Sidebar;
    break;
  }
}

void free_app(App *app) 
{
  free(app->clients);
  free_config(app->cfg);
  free(app);
}
