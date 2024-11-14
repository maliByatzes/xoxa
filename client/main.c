/* main.c */

#include "app.h"
#include "ui.h"
#include <curses.h>
//#include "client.h"

void run_app(App *app);
void handle_key(App *app, int ch);

int main()
{
  App *app = new_app();
  run_app(app);

  return 0;
}

void run_app(App *app) 
{
  init_ui(app);
  
  for (;;) {
    draw_ui(app);

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
      // TODO: move up the client list
      break;
    case KEY_DOWN:
      // TODO: move down the client list
      break;
    }
  }
  
}
