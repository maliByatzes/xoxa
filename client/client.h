/* client.h */

#ifndef __client_h
#define __client_h

#include "app.h"
#include "xoxa.h"
#include "ui.h"

int connect_to_remote(App *app);
int read_from_socket(App *app);
void send_list_cmd(App *app);

void get_clients(App *app, const char *list_data);
void get_messages_for_client(App *app, Client *client, const char *data);

void add_message(App *app, const char *message);

#endif
