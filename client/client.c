/* client.c */

#include "xoxa.h"
#include <curses.h>

#define INPUT_HEIGHT 3
#define STATUS_HEIGHT 2
#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 1024

WINDOW *messages_win;
WINDOW *input_win;
WINDOW *status_win;

char message_history[MAX_MESSAGES][MAX_MESSAGE_LENGTH];
int message_count = 0;
int scroll_position = 0;

void init_ui()
{
  initscr();
  cbreak();
  noecho();
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

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  messages_win = newwin(max_y - INPUT_HEIGHT - STATUS_HEIGHT, max_x, 0, 0);
  input_win = newwin(INPUT_HEIGHT, max_x, max_y - INPUT_HEIGHT - STATUS_HEIGHT, 0);
  status_win = newwin(STATUS_HEIGHT, max_x, max_y - STATUS_HEIGHT, 0);

  scrollok(messages_win, TRUE);

  box(input_win, 0, 0);

  wbkgd(status_win, COLOR_PAIR(1));
  wbkgd(input_win, COLOR_PAIR(2));
  wbkgd(status_win, COLOR_PAIR(3));

  refresh();
  wrefresh(messages_win);
  wrefresh(input_win);
  wrefresh(status_win);
}

void update_status(const char *status)
{
  werase(status_win);
  wbkgd(status_win, COLOR_PAIR(1));
  mvwprintw(status_win, 0, 1, "Status: %s", status);
  mvwprintw(status_win, 1, 1, "Commands: 'sendto', 'list', PAGE UP/DOWN to scroll");
  wrefresh(status_win);
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

char *read_input()
{
  static char input[MAX_MESSAGE_LENGTH];
  memset(input, 0, MAX_MESSAGE_LENGTH);

  werase(input_win);
  box(input_win, 0, 0);
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
  add_message("Connected to server. Type 'sendto' or 'list' to begin", 4);

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
      add_message(read, 3);
    }

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
    }

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
