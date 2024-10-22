/* client.c */

#include "xoxa.h"
#include <curses.h>
#include <sys/param.h>

#define INPUT_HEIGHT 3
#define STATUS_HEIGHT 2
#define SIDEBAR_WIDTH 20
#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 1024
#define MAX_CLIENTS 50
#define MAX_CLIENT_NAME 50

WINDOW *messages_win;
WINDOW *input_win;
WINDOW *status_win;
WINDOW *sidebar_win;

char message_history[MAX_MESSAGES][MAX_MESSAGE_LENGTH];
int message_count = 0;
int scroll_position = 0;

typedef struct {
  char name[MAX_CLIENT_NAME];
  char ip[16];
  char port[6];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
int selected_client = -1;
int sidebar_scroll = 0;

void init_ui()
{
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

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

  sidebar_win = newwin(max_y - STATUS_HEIGHT, SIDEBAR_WIDTH, 0, 0);
  messages_win = newwin(max_y - INPUT_HEIGHT - STATUS_HEIGHT,
                        max_x - SIDEBAR_WIDTH, 0, SIDEBAR_WIDTH);
  input_win = newwin(INPUT_HEIGHT, max_x - SIDEBAR_WIDTH,
                     max_y - INPUT_HEIGHT - STATUS_HEIGHT, SIDEBAR_WIDTH);
  status_win = newwin(STATUS_HEIGHT, max_x, max_y - STATUS_HEIGHT, 0);

  scrollok(messages_win, TRUE);

  box(sidebar_win, 0, 0);
  box(input_win, 0, 0);

  wbkgd(status_win, COLOR_PAIR(1));
  wbkgd(input_win, COLOR_PAIR(2));
  wbkgd(messages_win, COLOR_PAIR(3));
  wbkgd(sidebar_win, COLOR_PAIR(4));

  refresh();
  wrefresh(messages_win);
  wrefresh(input_win);
  wrefresh(status_win);
  wrefresh(sidebar_win);
}

void update_status(const char *status)
{
  werase(status_win);
  wbkgd(status_win, COLOR_PAIR(1));
  mvwprintw(status_win, 0, 1, "Status: %s", status);
  mvwprintw(status_win, 1, 1, "↑ /↓: Navigate | Enter: Select | Mouse: Click to select");
  wrefresh(status_win);
}

void refresh_sidebar()
{
  werase(sidebar_win);
  box(sidebar_win, 0, 0);
  mvwprintw(sidebar_win, 0, 2, " Clients (%d)", client_count);

  int display_start = sidebar_scroll;
  int display_end = MIN(client_count, display_start + (getmaxy(sidebar_win) - 2));

  for (int i = display_start; i < display_end; i++) {
    if (i == selected_client) {
      wattron(sidebar_win, COLOR_PAIR(6));
      mvwprintw(sidebar_win, i - display_start + 1, 1, "%-18s", clients[i].name);
      wattroff(sidebar_win, COLOR_PAIR(6));
    } else {
      wattron(sidebar_win, COLOR_PAIR(7));
      mvwprintw(sidebar_win, i - display_start + 1, 1, "%-18s", clients[i].name);
      wattroff(sidebar_win, COLOR_PAIR(7));
    }
  }

  wrefresh(sidebar_win);
}

void add_message(const char *msg, int color_pair)
{
  if (message_count < MAX_MESSAGES) {
    strncpy(message_history[message_count], msg, MAX_MESSAGE_LENGTH - 1);
    message_count++;
  } else {
    for (int i = 0; i < MAX_MESSAGES - 1; ++i) {
      strcpy(message_history[i], message_history[i+1]);
    }
    strncpy(message_history[MAX_MESSAGES - 1], msg, MAX_MESSAGE_LENGTH - 1);
  }

  wattron(messages_win, COLOR_PAIR(color_pair));
  wprintw(messages_win, "%s\n", msg);
  wattroff(messages_win, COLOR_PAIR(color_pair));
  wrefresh(messages_win);
}

void parse_client_list(const char *list_data)
{
  client_count = 0;
  char *data_copy = strdup(list_data);
  char *line = strtok(data_copy, "\n");

  while (line != NULL && client_count < MAX_CLIENTS) {
    // `name ip:port`
    char name[MAX_CLIENT_NAME];
    char ip[16];
    char port[6];

    if (sscanf(line, "%s %[^:]:%s", name, ip, port) == 3) {
      strncpy(clients[client_count].name, name, MAX_CLIENT_NAME - 1);
      strncpy(clients[client_count].ip, ip, 15);
      strncpy(clients[client_count].port, port, 5);
      client_count++;
    }

    line = strtok(NULL, "\n");
  }

  free(data_copy);
  refresh_sidebar();
}

void handle_mouse_click(int y, int x) {
  if (x < SIDEBAR_WIDTH) {
    int clicked_index = sidebar_scroll + (y - 1);
    if (clicked_index >= 0 && clicked_index < client_count) {
      selected_client = clicked_index;
      refresh_sidebar();

      char status[100];
      snprintf(status, sizeof(status), "Selected: %s", clients[selected_client].name);
      update_status(status);
    }
  }
}

void handle_key(int ch)
{
  switch (ch) {
  case KEY_UP:
    if (selected_client > 0) {
      selected_client--;
      if (selected_client < sidebar_scroll) {
        sidebar_scroll--;
      }
      refresh_sidebar();
    }
    break;
  case KEY_DOWN:
    if (selected_client < client_count - 1) {
      selected_client++;
      if (selected_client >= sidebar_scroll + getmaxy(sidebar_win) - 2) {
        sidebar_scroll++;
      }
      refresh_sidebar();
    }
    break;

  case '\n':
    if (selected_client >= 0) {
      char status[100];
      snprintf(status, sizeof(status), "Chatting with: %s",
               clients[selected_client].name);
      update_status(status);

      // Prepare sendto command automatically
      char cmd[8192];
      snprintf(cmd, sizeof(cmd), "sendto\n%s %s\n",
               clients[selected_client].ip,
               clients[selected_client].port);

      // TODO: Handle sending command with message
    }
    break;
  }
}

char *read_input()
{
  static char input[MAX_MESSAGE_LENGTH];
  memset(input, 0, MAX_MESSAGE_LENGTH);

  werase(input_win);
  box(input_win, 0, 0);
  if (selected_client >= 0) {
    mvwprintw(input_win, 0, 2, "To: %s", clients[selected_client].name);
  }
  mvwprintw(input_win, 1, 1, "> ");
  wrefresh(input_win);

  echo();
  mvwgetnstr(input_win, 1, 3, input, MAX_MESSAGE_LENGTH - 1);
  noecho();

  return input;
}

int main(int argc, char ** argv)
{

  if (argc != 3) {
    fprintf(stderr, "usage: %s <hostname> <port>\n", argv[0]);
    return 1;
  }

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2,2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return 1;
  }
#endif

  init_ui();
  update_status("Connecting...");

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *peer_address;
  if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
    add_message("get addrinfo() failed.", 5);
    getch();
    endwin();
    return 1;
  }

  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    add_message("socket() failed.", 5);
    getch();
    endwin();
    return 1;
  }

  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    add_message("connect() failed.", 5);
    getch();
    endwin();
    return 1;
  }
  freeaddrinfo(peer_address);

  update_status("Connected");
  add_message("Connected to server. Use arrow keys or mouse to select clients.", 4);

  MEVENT event;
  while (1) {

    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(socket_peer, &reads);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    if (select(socket_peer+1, &reads, 0, 0, &timeout) < 0) {
      add_message("select() failed.", 5);
      break;
    }

    if (FD_ISSET(socket_peer, &reads)) {
      char read[4096];
      int bytes_read = recv(socket_peer, read, 4096, 0);
      if (bytes_read < 1) {
        add_message("Connection closed by peer.", 5);
        break;
      }
      read[bytes_read] = '\0';

      if (strstr(read, "Client List:") == read) {
        parse_client_list(read + 12);
      } else {
        add_message(read, 3);
      }
    }

    int ch = wgetch(input_win);
    if (ch == KEY_MOUSE && getmouse(&event) == OK) {
      handle_mouse_click(event.y, event.x);
    } else if (ch != ERR) {
      handle_key(ch);
    }

    static time_t last_refresh = 0;
    time_t current_time = time(NULL);
    if (current_time - last_refresh >= 5) {
      send(socket_peer, "list\n", 5, 0);
      last_refresh = current_time;
    }

    /*
    char *input = read_input();
    if (strlen(input) > 0) {
      if (strcmp(input, "sendto") == 0) {
        update_status("Enter destination details...");
        add_message("Enter destination IP:", 4);
        char dest_ip[200] = {0};
        strcpy(dest_ip, read_input());

        add_message("Enter destination port:", 4);
        char dest_port[50] = {0};
        strcpy(dest_port, read_input());

        add_message("Enter message (empty line to read):", 4);
        char message[8192] = {0};
        char buffer[512];

        while (1) {
          char *line = read_input();
          if (strlen(line) == 0) break;
          strcat(message, line);
          strcat(message, "\n");
        }

        char complete_msg[8192];
        snprintf(complete_msg, sizeof(complete_msg), "sendto\n%s %s\n%s",
                 dest_ip, dest_port, message);

        int bytes_sent = send(socket_peer, complete_msg, strlen(complete_msg), 0);
        char status[100];
        snprintf(status, sizeof(status), "Sent %d bytes", bytes_sent);
        update_status(status);

      } else if (strcmp(input, "list") == 0) {
        char list_cmd[] = "list\n";
        int bytes_sent = send(socket_peer, list_cmd, strlen(list_cmd), 0);
        char status[100];
        snprintf(status, sizeof(status), "Sent %d bytes", bytes_sent);
        update_status(status);

      } else {
        add_message("Invalid command. Use 'sendto' or 'list'", 5);
      }
    }*/

  }

  update_status("Closing connection...");
  CLOSESOCKET(socket_peer);

#if defined(_WIN32)
  WSACleanup();
#endif

  mvprintw(LINES - 1, 0, "Press any key to exit...");
  refresh();
  getch();
  endwin();

  return 0;
}
