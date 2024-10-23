/* client.h */

#ifndef __client_h
#define __client_h

#include "xoxa.h"
#include "config.h"
#include <curses.h>
#include <sys/param.h>

#define INPUT_HEIGHT 3
#define STATUS_HEIGHT 2
#define SIDEBAR_WIDTH 20
#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 1024
#define MAX_CLIENTS 50
#define MAX_CLIENT_NAME 50

typedef struct {
  char name[MAX_CLIENT_NAME];
  char ip[16];
  char port[6];
} Client;

typedef struct {
  WINDOW *messages_win;
  WINDOW *input_win;
  WINDOW *status_win;
  WINDOW *sidebar_win;

  char message_history[MAX_MESSAGES][MAX_MESSAGE_LENGTH];
  int message_count;
  int scroll_position;

  Client clients[MAX_CLIENTS];
  int client_count;
  int selected_client;
  int sidebar_scroll;

  Config cfg;
} Application;

Application init_application();
void run_application(Application */* app */);
void free_application(Application */* app */);

void init_ui(Application */* app */);
void update_status(Application */* app */, const char */* status */);
void refresh_sidebar(Application */* app */);
void add_message(Application */* app */, const char */* msg */, int /* color_pair */);
void parse_client_list(Application */* app */, const char */* list_data */);
void handle_key(Application */* app */, int /* ch */);
char *read_input(Application */* app */);

SOCKET connect_to_remote(Application */* app */);
void client_loop(Application */* app */, SOCKET /* socket_peer */);

#endif
