/* ui.c */

#include "ui.h"
#include "app.h"
#include <curses.h>

void init_ui(App *app) 
{
  initscr();
  cbreak();
  noecho();
  raw();
  keypad(stdscr, TRUE);

  if (!has_colors()) {
    endwin();
    fprintf(stderr, "Your terminal does not support colors.\n");
    exit(1);
  }

  start_color();
  
  init_pair(1, COLOR_WHITE, COLOR_BLUE);
  init_pair(2, COLOR_BLACK, COLOR_WHITE);
  init_pair(3, COLOR_WHITE, COLOR_BLACK);
  init_pair(4, COLOR_GREEN, COLOR_BLACK);
  init_pair(5, COLOR_RED, COLOR_BLACK);
  init_pair(6, COLOR_BLACK, COLOR_CYAN);
  init_pair(7, COLOR_CYAN, COLOR_BLACK);

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  app->sidebar_win = newwin(max_y - STATUS_HEIGHT, SIDEBAR_WIDTH, 0, 0);
  app->message_win = newwin(max_y - INPUT_HEIGHT - STATUS_HEIGHT, max_x - SIDEBAR_WIDTH, 0, SIDEBAR_WIDTH);
  app->input_win = newwin(INPUT_HEIGHT, max_x - SIDEBAR_WIDTH, max_y - INPUT_HEIGHT - STATUS_HEIGHT, SIDEBAR_WIDTH);
  app->status_win = newwin(STATUS_HEIGHT, max_x, max_y -STATUS_HEIGHT, 0);

  scrollok(app->message_win, TRUE);

  box(app->sidebar_win, 0, 0);
  box(app->input_win, 0, 0);

  wbkgd(app->status_win, COLOR_PAIR(1));
  wbkgd(app->input_win, COLOR_PAIR(2));
  wbkgd(app->message_win, COLOR_PAIR(3));
  wbkgd(app->sidebar_win, COLOR_PAIR(4));

  refresh();
  wrefresh(app->message_win);
  wrefresh(app->input_win);
  wrefresh(app->status_win);
  wrefresh(app->sidebar_win);
}

void draw_ui(App *app) 
{
}
