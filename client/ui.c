/* ui.c */

#include "ui.h"
#include "app.h"
#include <curses.h>
#include <sys/param.h>
#include <sys/time.h>
#include <unistd.h>

void draw_ui(App *app) 
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

  gettimeofday(&app->last_update, NULL);

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  app->sidebar_win = newwin(max_y - STATUS_HEIGHT, SIDEBAR_WIDTH, 0, 0);
  app->message_win = newwin(max_y - INPUT_HEIGHT - STATUS_HEIGHT, max_x - SIDEBAR_WIDTH, 0, SIDEBAR_WIDTH);
  app->input_win = newwin(INPUT_HEIGHT, max_x - SIDEBAR_WIDTH, max_y - INPUT_HEIGHT - STATUS_HEIGHT, SIDEBAR_WIDTH);
  app->status_win = newwin(STATUS_HEIGHT, max_x, max_y - STATUS_HEIGHT, 0);

  scrollok(app->message_win, TRUE);

  // Draw sidebar window
  box(app->sidebar_win, 0, 0);
  mvwprintw(app->sidebar_win, 0, 0, " Client List (%d)", app->client_count);
  wbkgd(app->sidebar_win, COLOR_PAIR(3));

  // Draw message window
  box(app->message_win, 0, 0);
  wbkgd(app->message_win, COLOR_PAIR(3));

  // Draw input window
  box(app->input_win, 0, 0);
  if (app->selected_client >= 0) {
    mvwprintw(app->input_win, 0, 2, "to: %s", app->clients[app->selected_client].name);
  } else {
    mvwprintw(app->input_win, 0, 2, "to: fuckn somebody");
  }

  mvwprintw(app->input_win, 1, 1, "> ");
  wbkgd(app->input_win, COLOR_PAIR(3));
  
  // Draw status window
  box(app->status_win, 0, 0);

  if (app->current_status != NULL) {
    mvwprintw(app->status_win, 1, 1, "Status: %s", app->current_status);
  } else {
    mvwprintw(app->status_win, 1, 1, "Status: <...>");
  }
  mvwprintw(app->status_win, 2, 1, "(q) to exit");
  wbkgd(app->status_win, COLOR_PAIR(3));

  wnoutrefresh(app->message_win);
  wnoutrefresh(app->input_win);
  wnoutrefresh(app->status_win);
  wnoutrefresh(app->sidebar_win);

  doupdate();
}

void update_ui(App *app) 
{
  gettimeofday(&app->current_time, NULL);

  long elasped_time = (app->current_time.tv_sec - app->last_update.tv_sec) * 1000000 +
                      (app->current_time.tv_usec - app->last_update.tv_usec);

  int needs_update = (
    app->last_selected_client != app->selected_client ||
    app->last_active_win != app->current_active_win ||
    elasped_time >= 100000 
  );
  
  if (needs_update) {
    
    werase(app->sidebar_win);
    werase(app->message_win);
    werase(app->input_win);
    werase(app->status_win);
    
    // Draw more stuff depending on which window is active
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
      if (app->current_status != NULL) {
        mvwprintw(app->status_win, 1, 1, "Status: %s", app->current_status);
      } else {
        mvwprintw(app->status_win, 1, 1, "Status: <...>");
      }
      mvwprintw(app->status_win, 2, 1, "(q) to exit");
      break;
    case CAW_Sidebar:
      wattron(app->sidebar_win, COLOR_PAIR(4));
      box(app->sidebar_win, 0, 0);
      wattroff(app->sidebar_win, COLOR_PAIR(4));
      mvwprintw(app->sidebar_win, 0, 0, " Client List (%d)", app->client_count);
      break;
    }
  
    // Display clients on sidebar window
    display_clients(app);
    // Display messages on message window
    display_messages(app);
  
    wnoutrefresh(app->message_win);
    wnoutrefresh(app->input_win);
    wnoutrefresh(app->status_win);
    wnoutrefresh(app->sidebar_win);

    doupdate();

    app->last_selected_client = app->selected_client;
    app->last_active_win = app->current_active_win;
    app->last_update = app->current_time;
  }
}

void update_status(App *app, const char *status)
{
  werase(app->status_win);
  box(app->status_win, 0, 0);
  mvwprintw(app->status_win, 1, 1, "Status: %s", status);
  mvwprintw(app->status_win, 2, 1, "(q) to exit");
  wrefresh(app->status_win);
  app->current_status = status;
}

char *read_input(App *app) 
{
  static char msg_input[MAX_MESSAGE_LENGTH];
  memset(msg_input, 0, MAX_MESSAGE_LENGTH);

  werase(app->input_win);
  wattron(app->input_win, COLOR_PAIR(4));
  box(app->input_win, 0, 0);
  wattroff(app->input_win, COLOR_PAIR(4));

  if (app->selected_client >= 0) {
    mvwprintw(app->input_win, 0, 2, "to: %s", app->clients[app->selected_client].name);
  } else {
    mvwprintw(app->input_win, 0, 2, "to: fuckn somebody");
  }

  mvwprintw(app->input_win, 1, 1, "> ");
  wrefresh(app->input_win);

  echo();
  mvwgetnstr(app->input_win, 1, 3, msg_input, MAX_MESSAGE_LENGTH - 1);
  noecho();

  return msg_input;
}

void display_clients(App *app) 
{
  if (app->client_count > 0) {
    int display_start = app->sidebar_scroll;
    int display_end = MIN(app->client_count, display_start + (getmaxy(app->sidebar_win) - 2));
    
    for (int i = display_start; i < display_end; i++) {
      if (i == app->selected_client) {
        wattron(app->sidebar_win, COLOR_PAIR(6));
        mvwprintw(app->sidebar_win, i - display_start + 1, 1, "%-18s", app->clients[i].name);
        wattroff(app->sidebar_win, COLOR_PAIR(6));
      } else {
        wattron(app->sidebar_win, COLOR_PAIR(7));
        mvwprintw(app->sidebar_win, i - display_start + 1, 1, "%-18s", app->clients[i].name);
        wattroff(app->sidebar_win, COLOR_PAIR(7));
      }
    }
  } else {
    
    int display_start = app->sidebar_scroll;
    int display_end = MIN(3, display_start + (getmaxy(app->sidebar_win) - 2));
    
    char *placeholder_name = "user...";
    for (int i = display_start; i < display_end; i++) {
      if (i == app->selected_client) {
        wattron(app->sidebar_win, COLOR_PAIR(6));
        mvwprintw(app->sidebar_win, i - display_start + 1, 1, "%-18s", placeholder_name);
        wattroff(app->sidebar_win, COLOR_PAIR(6));
      } else {
        wattron(app->sidebar_win, COLOR_PAIR(7));
        mvwprintw(app->sidebar_win, i - display_start + 1, 1, "%-18s", placeholder_name);
        wattroff(app->sidebar_win, COLOR_PAIR(7));
      }
    }
  }
}

void display_messages(App *app) 
{
  // TODO: display a message in diff color depending on the sender...

  wclear(app->message_win);

  if (app->selected_client >= 0) {    
    int y = 1;
    for (int i = 0; i < app->clients[app->selected_client].message_count; i++) {
      mvwprintw(app->message_win, y, 2, "%s", app->clients[app->selected_client].messages[i]);
      y++;
    }
  }
  
  box(app->message_win, 0, 0);
  wrefresh(app->message_win);
}

void display_exit_message() 
{
  mvprintw(LINES - 1, 0, "Press any key to exit...");
  refresh();
  getch();
  endwin();
}
