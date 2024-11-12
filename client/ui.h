/* ui.h */

#ifndef __ui_h
#define __ui_h

#include "app.h"

void init_ui(App *app);
void draw_ui(App *app);
void update_status(App *app, const char *status);
void refresh_sidebar(App *app);

#endif
