/* client.h */

#ifndef __client_h
#define __client_h

#include "app.h"
#include "xoxa.h"

/*
#include "xoxa.h"
#include "config.h"
#include <curses.h>
#include <sys/param.h>

#define MAX_CLIENT_NAME 50

typedef struct {
  char name[MAX_CLIENT_NAME];
  char ip[16];
  char port[6];
} Client;

Application init_application();
void run_application(Application * app);
void free_application(Application *app);

void init_ui(Application *app);
void update_status(Application *app, const char *status);
void refresh_sidebar(Application *app);
void add_message(Application *app, const char *msg, int color_pair);
void parse_client_list(Application *app, const char *list_data);
void handle_key(Application *app, int ch);
char *read_input(Application *app);
*/

SOCKET connect_to_remote(App *app);
void client_loop(App *app);

void fetch_messages_with_select_client(App *app);

#endif
