/* app.h */

#ifndef __app_h
#define __app_h

#include "config.h"
#include "xoxa.h"
#include <curses.h>

#define INPUT_HEIGHT 3
#define STATUS_HEIGHT 4
#define SIDEBAR_WIDTH 30
#define MAX_MESSAGES 10
#define MAX_MESSAGE_LENGTH 70 // 50 for message and 20 for client name
#define MAX_CLIENTS 20
#define MAX_CLIENT_NAME 20

typedef enum CurrentActiveWindow_ {
  CAW_Message,
  CAW_Input,
  CAW_Status,
  CAW_Sidebar
} CurrentActiveWindow;

typedef struct Client_ {
  char name[MAX_CLIENT_NAME];
  char ip[20];
  char port[20];
  char messages[MAX_MESSAGES][MAX_MESSAGE_LENGTH];
  int message_count;
} Client;

typedef struct App_ {
  WINDOW *message_win;
  WINDOW *input_win;
  WINDOW *status_win;
  WINDOW *sidebar_win;
    
  Client *clients;
  int client_count;
  int selected_client;
  int sidebar_scroll;
    
  Client *current_client;
  
  const char *current_status;
  
  CurrentActiveWindow current_active_win;
  
  Config *cfg;

  SOCKET socket_peer;
} App;

App *new_app();
int run_app(App *app);
int handle_key(App *app, int ch);
void toggle_active_window(App *app);
void free_app(App *app);

#endif
