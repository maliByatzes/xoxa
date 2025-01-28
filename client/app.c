/* app.c */

#include "app.h"
#include "ui.h"
#include "client.h"
#include <pthread.h>
#include <sys/time.h>

App *new_app() 
{
  App *app = (App *)calloc(sizeof(App), 1);

  app->client_count = 0;
  app->selected_client = 0;
  app->last_selected_client = -1;
  app->current_active_win = CAW_Sidebar;
  // app->last_active_win = -1;

  app->current_client = NULL;
  app->current_status = NULL;
  
  app->cfg = load_config();

  if (!validate_config(app->cfg)) {
    free_config(app->cfg);
    exit(1);
  }

  app->clients = (Client *)calloc(sizeof(Client), MAX_CLIENTS);

  // app->running = 1;
  // pthread_mutex_init(&app->mutex, NULL);
  
  return app;
}

int run_app(App *app) 
{  
  /*
  pthread_t event_handling_thread, ui_thread;
  pthread_create(&event_handling_thread, NULL, event_handling, app);
  pthread_create(&ui_thread, NULL, ui_handling, app);

  pthread_join(event_handling_thread, NULL);
  pthread_join(ui_thread, NULL);
  */

  draw_ui(app);

  while (app->running) {
    update_ui(app);
    if (read_from_socket(app)) {
      return 1;
    }

    // Event handling
    int ch = getch();
    if (ch != ERR) {
      int res = handle_key(app, ch);
      if (res == 1) {
        return 0;
      }
    }
    
    // usleep(100000);
  }

  return 0;
}

/*
void *event_handling(void *arg) 
{
  App *app = (App *)arg;
  int local_running = 1;

  pthread_mutex_lock(&app->mutex);
  local_running = app->running;
  pthread_mutex_unlock(&app->mutex);
  
  while (local_running) {
    pthread_mutex_lock(&app->mutex);
    local_running = app->running;
    pthread_mutex_unlock(&app->mutex);
    
    // Event handling...
    int ch = getch();
    if (ch != ERR) {
      int res = handle_key(app, ch);
      if (res == 1) {
        pthread_mutex_lock(&app->mutex);
        app->running = 0;
        pthread_mutex_unlock(&app->mutex);
        return NULL;
      }
    }

    usleep(100000);
  }

  return NULL;
}

void *ui_handling(void *arg) 
{
  App *app = (App *)arg;

  draw_ui(app);

  while (app->running) {
    // update_ui(app);
    if (read_from_socket(app)) {
      return NULL; // FIXME: This is wrong
    }
    
    usleep(100000);
  }

  return NULL;
}*/

int handle_key(App *app, int ch) 
{
  // Global key binds
  switch (ch) {
  case 9:
    toggle_active_window(app);
    break;
  case 'l':
    send_list_cmd(app);
    goto end;
    break;
  case 'q':
    app->running = 0;
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
    char *line = NULL;
    
    // automatically get input
    // read only one line for now
    line = read_input(app);

    char complete_request[512];
    memset(complete_request, 0, sizeof(complete_request));
    snprintf(complete_request, sizeof(complete_request), "sendto\n%s %s\n%s",
              app->clients[app->selected_client].ip,
              app->clients[app->selected_client].port,
              line
    );

    send(app->socket_peer, complete_request, strlen(complete_request), 0);
    
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
    case 'm':
      send_get_msgs_cmd(app);
      break;
    }
  }
  
end:
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
  // pthread_mutex_destroy(&app->mutex);
  free(app->clients);
  free_config(app->cfg);
  free(app);
}
