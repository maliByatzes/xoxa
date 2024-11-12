/* client.c */

#include "client.h"

/*
Application init_application()
{
  Application app;

  app.message_count = 0;
  app.scroll_position = 0;
  app.client_count = 0;
  app.selected_client = -1;
  app.sidebar_scroll = 0;

  app.cfg = load_config();

  if (!validate_config(&app.cfg)) {
    free_config(&app.cfg);
    exit(1);
  }

  return app;
}
void run_application(Application *app) {

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2,2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return 1;
  }
#endif

  init_ui(app);

  SOCKET socket_peer = connect_to_remote(app);

  app->socket_peer = socket_peer;

  client_loop(app);

  update_status(app, "Closing connection...");
  CLOSESOCKET(socket_peer);

#if defined(_WIN32)
  WSACleanup();
#endif

  mvprintw(LINES - 1, 0, "Press any key to exit...");
  refresh();
  getch();
  endwin();

  free_config(&app->cfg);
  free_application(app);
}

void free_application(Application * app) {}

void init_ui(Application *app)
{
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  // mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

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
  app->messages_win = newwin(max_y - INPUT_HEIGHT - STATUS_HEIGHT,
                        max_x - SIDEBAR_WIDTH, 0, SIDEBAR_WIDTH);
  app->input_win = newwin(INPUT_HEIGHT, max_x - SIDEBAR_WIDTH,
                     max_y - INPUT_HEIGHT - STATUS_HEIGHT, SIDEBAR_WIDTH);
  app->status_win = newwin(STATUS_HEIGHT, max_x, max_y - STATUS_HEIGHT, 0);

  scrollok(app->messages_win, TRUE);

  box(app->sidebar_win, 0, 0);
  box(app->input_win, 0, 0);

  wbkgd(app->status_win, COLOR_PAIR(1));
  wbkgd(app->input_win, COLOR_PAIR(2));
  wbkgd(app->messages_win, COLOR_PAIR(3));
  wbkgd(app->sidebar_win, COLOR_PAIR(4));

  refresh();
  wrefresh(app->messages_win);
  wrefresh(app->input_win);
  wrefresh(app->status_win);
  wrefresh(app->sidebar_win);
}

void update_status(Application *app, const char *status)
{
  werase(app->status_win);
  wbkgd(app->status_win, COLOR_PAIR(1));
  mvwprintw(app->status_win, 0, 1, "Status: %s", status);
  mvwprintw(app->status_win, 1, 1, "<UP> / <DOWN>: Navigate | Enter: Select");
  wrefresh(app->status_win);
}

void refresh_sidebar(Application *app)
{
  werase(app->sidebar_win);
  box(app->sidebar_win, 0, 0);
  mvwprintw(app->sidebar_win, 0, 2, " Clients (%d)", app->client_count);

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

  wrefresh(app->sidebar_win);
}

void add_message(Application *app, const char *msg, int color_pair)
{
  if (app->message_count < MAX_MESSAGES) {
    strncpy(app->message_history[app->message_count], msg, MAX_MESSAGE_LENGTH - 1);
    app->message_count++;
  } else {
    for (int i = 0; i < MAX_MESSAGES - 1; ++i) {
      strcpy(app->message_history[i], app->message_history[i+1]);
    }
    strncpy(app->message_history[MAX_MESSAGES - 1], msg, MAX_MESSAGE_LENGTH - 1);
  }

  wattron(app->messages_win, COLOR_PAIR(color_pair));
  wprintw(app->messages_win, "%s\n", msg);
  wattroff(app->messages_win, COLOR_PAIR(color_pair));
  wrefresh(app->messages_win);
}

void parse_client_list(Application *app, const char *list_data)
{
  app->client_count = 0;
  char *data_copy = strdup(list_data);
  char *line = strtok(data_copy, "\n");

  while (line != NULL && app->client_count < MAX_CLIENTS) {
    // `name ip:port`
    char name[MAX_CLIENT_NAME];
    char ip[16];
    char port[6];

    if (sscanf(line, "%s %[^:]:%s", name, ip, port) == 3) {
      strncpy(app->clients[app->client_count].name, name, MAX_CLIENT_NAME - 1);
      strncpy(app->clients[app->client_count].ip, ip, 15);
      strncpy(app->clients[app->client_count].port, port, 5);
      app->client_count++;
    }

    line = strtok(NULL, "\n");
  }

  free(data_copy);
  refresh_sidebar(app);
}

void handle_key(Application *app, int  ch)
{
  switch (ch) {
  case 'I':
  case 'i':
    if (app->selected_client >= 0) {
      char message[7168];
      memset(message, 0, sizeof(message));

      while (1) {
        char *line = read_input(app);
        if (strlen(line) == 0) break;
        strcat(message, line);
        strcat(message, "\n");
      }

      char complete_request[8192];
      memset(complete_request, 0, sizeof(complete_request));
      snprintf(complete_request, sizeof(complete_request), "sendto\n%s %s\n%s",
                app->clients[app->selected_client].ip,
                app->clients[app->selected_client].port,
                message);

      add_message(app, message, 4);

      send(app->socket_peer, complete_request, strlen(complete_request), 0);
    }
    break;
  case KEY_UP:
    if (app->selected_client > 0) {
      app->selected_client--;
      if (app->selected_client < app->sidebar_scroll) {
        app->sidebar_scroll--;
      }
      refresh_sidebar(app);
    }
    break;
  case KEY_DOWN:
    if (app->selected_client < app->client_count - 1) {
      app->selected_client++;
      if (app->selected_client >= app->sidebar_scroll + getmaxy(app->sidebar_win) - 2) {
        app->sidebar_scroll++;
      }
      refresh_sidebar(app);
    }
    break;
 case 'r':
    send(app->socket_peer, "list\n", 5, 0);
    break;
  }
}

char *read_input(Application *app)
{
  static char input[MAX_MESSAGE_LENGTH];
  memset(input, 0, MAX_MESSAGE_LENGTH);

  werase(app->input_win);
  box(app->input_win, 0, 0);
  if (app->selected_client >= 0) {
    mvwprintw(app->input_win, 0, 2, "To: %s", app->clients[app->selected_client].name);
  }
  mvwprintw(app->input_win, 1, 1, "> ");
  wrefresh(app->input_win);

  echo();
  mvwgetnstr(app->input_win, 1, 3, input, MAX_MESSAGE_LENGTH - 1);
  noecho();

  return input;
}

SOCKET connect_to_remote(Application *app)
{
  update_status(app, "Connecting...");

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *peer_address;
  if (getaddrinfo(app->cfg.server_ip, app->cfg.server_port, &hints, &peer_address)) {
    char status[100];
    snprintf(status, sizeof(status), "getaddrinfo() failed. (%d)",
             GETSOCKETERRNO());
    update_status(app, status);
    getch();
    endwin();
    exit(1);
  }

  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    char status[100];
    snprintf(status, sizeof(status), "socket() failed. (%d)",
             GETSOCKETERRNO());
    update_status(app, status);
    getch();
    endwin();
    exit(1);
  }

  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    char status[100];
    snprintf(status, sizeof(status), "connect() failed. (%d)", GETSOCKETERRNO());
    update_status(app, status);
    getch();
    endwin();
    exit(1);
  }

  freeaddrinfo(peer_address);

  // send client's name
  send(socket_peer, app->cfg.client_name, strlen(app->cfg.client_name), 0);

  update_status(app, "Connected");

  return socket_peer;
}

void client_loop(Application *app)
{
  while (1) {

    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(app->socket_peer, &reads);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    if (select(app->socket_peer+1, &reads, 0, 0, &timeout) < 0) {
      char status[100];
      snprintf(status, sizeof(status), "select() failed. (%d)",
              GETSOCKETERRNO());
      update_status(app, status);
      break;
    }

    // Read data from socket_peer
    if (FD_ISSET(app->socket_peer, &reads)) {
      char read_buffer[8196];
      memset(read_buffer, 0, sizeof(read_buffer));
      int bytes_read = recv(app->socket_peer, read_buffer, 4096, 0);
      if (bytes_read < 1) {
        update_status(app, "Connection closed by peer.");
        break;
      }
      read_buffer[bytes_read] = '\0';

      if (strstr(read_buffer, "Client List:\n") == read_buffer) {
        parse_client_list(app, read_buffer + 13);
      } else {
        add_message(app, read_buffer, 4);
      }
    }

    static time_t last_refresh = 0;
    time_t current_time = time(NULL);
    if (current_time - last_refresh >= 60) { // refresh every 60 seconds
      send(app->socket_peer, "list\n", 5, 0);
      last_refresh = current_time;
    }

    int ch = getch(); // get from main window
    if (ch != ERR) {
      handle_key(app, ch);
    }

  }
}*/
