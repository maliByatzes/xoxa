/* ui.c */

#include "ui.h"
#include "app.h"
#include <curses.h>
#include <sys/param.h>

void init_ui(App *app) 
{
  initscr();
  cbreak();
  noecho();
  raw();
  curs_set(0);
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
}

void draw_ui(App *app) 
{
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  app->sidebar_win = newwin(max_y - STATUS_HEIGHT, SIDEBAR_WIDTH, 0, 0);
  app->message_win = newwin(max_y - INPUT_HEIGHT - STATUS_HEIGHT, max_x - SIDEBAR_WIDTH, 0, SIDEBAR_WIDTH);
  app->input_win = newwin(INPUT_HEIGHT, max_x - SIDEBAR_WIDTH, max_y - INPUT_HEIGHT - STATUS_HEIGHT, SIDEBAR_WIDTH);
  app->status_win = newwin(STATUS_HEIGHT, max_x, max_y - STATUS_HEIGHT, 0);

  scrollok(app->message_win, TRUE);

  box(app->sidebar_win, 0, 0);
  box(app->input_win, 0, 0);

  mvwprintw(app->sidebar_win, 0, 0, " Client List (%d)", app->client_count);
  mvwprintw(app->status_win, 1, 1, "Status: <...>");
  mvwprintw(app->status_win, 2, 1, "<q/Esc> to exit");

  wbkgd(app->status_win, COLOR_PAIR(3));
  wbkgd(app->input_win, COLOR_PAIR(3));
  wbkgd(app->message_win, COLOR_PAIR(3));
  wbkgd(app->sidebar_win, COLOR_PAIR(3));

  switch (app->current_active_win) {
  case CAW_Message:
    wattron(app->message_win, COLOR_PAIR(4));
    box(app->message_win, 0, 0);
    wattroff(app->message_win, COLOR_PAIR(4));
    break;
  case CAW_Input:
    wattron(app->input_win, COLOR_PAIR(4));
    box(app->input_win, 0, 0);
    wattroff(app->input_win, COLOR_PAIR(4));
    break;
  case CAW_Status:
    wattron(app->status_win, COLOR_PAIR(4));
    box(app->status_win, 0, 0);
    wattroff(app->status_win, COLOR_PAIR(4));
    mvwprintw(app->status_win, 1, 1, "Status: <...>");
    mvwprintw(app->status_win, 2, 1, "<q/Esc> to exit");
    break;
  case CAW_Sidebar:
    wattron(app->sidebar_win, COLOR_PAIR(4));
    box(app->sidebar_win, 0, 0);
    wattroff(app->sidebar_win, COLOR_PAIR(4));
    mvwprintw(app->sidebar_win, 0, 0, " Client List (%d)", app->client_count);
    break;
  }
  
  refresh();
  wrefresh(app->message_win);
  wrefresh(app->input_win);
  wrefresh(app->status_win);
  wrefresh(app->sidebar_win);
}

void update_status(App *app, const char *status)
{
  werase(app->status_win);
  wbkgd(app->status_win, COLOR_PAIR(1));
  mvwprintw(app->status_win, 0, 1, "Status: %s", status);
  mvwprintw(app->status_win, 1, 1, "<UP> / <DOWN>: Navigate | Enter: Select");
  wrefresh(app->status_win);
}

void refresh_sidebar(App *app) 
{
  werase(app->sidebar_win);
  box(app->sidebar_win, 0, 0);
  mvwprintw(app->sidebar_win, 0, 2, " Client List (%d)", app->client_count);

  int display_start = app->sidebar_scroll;
  int display_end = MIN(app->client_count, display_start + (getmaxy(app->sidebar_win) - 2));

  const char *placeholder = "user";
  for (int i = display_start; i < display_end; i++) {
    if (i == app->selected_client) {
      wattron(app->sidebar_win, COLOR_PAIR(6));
      mvwprintw(app->sidebar_win, i - display_start + 1, 1, "%18s", placeholder);
      wattroff(app->sidebar_win, COLOR_PAIR(6));
    } else {      
      wattron(app->sidebar_win, COLOR_PAIR(7));
      mvwprintw(app->sidebar_win, i - display_start + 1, 1, "%18s", placeholder);
      wattroff(app->sidebar_win, COLOR_PAIR(7));
    }
  }

  wrefresh(app->sidebar_win);
}
