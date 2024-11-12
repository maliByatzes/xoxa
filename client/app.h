/* app.h */

#ifndef __app_h
#define __app_h

//#include "client.h"
#include "config.h"
#include "xoxa.h"
#include <curses.h>

#define INPUT_HEIGHT 3
#define STATUS_HEIGHT 2
#define SIDEBAR_WIDTH 20
#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 1024
#define MAX_CLIENTS 50

typedef enum CurrentScreen_ {
  CS_Main,
  CS_Input,
  CS_Clients,
  CS_Exiting
} CurrentScreen;

typedef enum CurrentActiveWindow_ {
  CAW_Message,
  CAW_Input,
  CAW_Status,
  CAW_Sidebar
} CurrentActiveWindow;

typedef struct App_ {
  WINDOW *message_win;
  WINDOW *input_win;
  WINDOW *status_win;
  WINDOW *sidebar_win;
    
  char message_history[MAX_MESSAGES][MAX_MESSAGE_LENGTH];
  int message_count;

  //Client clients[MAX_CLIENTS];
  int client_count;
  int selected_client;
    
  CurrentScreen current_scrren;
  CurrentActiveWindow current_active_win;
  
  Config *cfg;

  SOCKET socket_peer;
} App;

App *new_app();
void toggle_active_window(App *app);
void free_app(App *app);

#endif
