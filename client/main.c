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
  // NOTE: check for current active window to handle diff input 4 diff win
  switch (ch) {
  case 'i':
    app->current_active_win = CAW_Input;
    if (app->selected_client >= 0) {
      // read input from user
    }
    break;
  case 9:
    toggle_active_window(app);
    break;
  case 27:
  case 'q':
    endwin();
    exit(0);
    break;
  }
}
