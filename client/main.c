/* main.c */

#include "app.h"
#include "client.h"
#include "ui.h"
#include "xoxa.h"
#include <curses.h>
//#include "client.h"

void run_app(App *app);
void handle_key(App *app, int ch);

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
  if (result < 0) {
    free_app(app);
    display_exit_message();
    return 1;
  }
  
  run_app(app);

  update_status(app, "Closing connection...");
  CLOSESOCKET(app->socket_peer);

#if defined (_WIN32)
  WSACleanup();
#endif
  free_app(app);

  display_exit_message();

  return 0;
}

void run_app(App *app) 
{
  init_ui();
  
  for (;;) {
    draw_ui(app);
    int result = client_loop(app);
    if (result < 0) {
      display_exit_message();
      free_app(app);
      exit(1);
    }

    // Event handling...
    int ch = getch();
    if (ch != ERR) {
      handle_key(app, ch);
    }
  }
}

void handle_key(App *app, int ch) 
{
  // Global key binds
  switch (ch) {
  case 9:
    toggle_active_window(app);
    break;
  case 'q':
    endwin();
    exit(0);
    break;
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
  
}
