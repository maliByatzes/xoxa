/* ui.h */

#ifndef __ui_h
#define __ui_h

#include "app.h"

void draw_ui(App *app);
void update_ui(App *app);
void update_status(App *app, const char *status);
char *read_input(App *app);
void display_clients(App *app);
void display_messages(App *app);
void display_exit_message();

#endif
