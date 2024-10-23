/* config.h */

#ifndef __config_h
#define __config_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *server_ip;
  char *server_port;
  char *client_name;
  int debug_mode;
} Config;

char *get_env_or_default(const char */* name */, char */* default_value */);
int get_env_bool(const char */* name */);
Config load_config();
void free_config(Config */* cfg */);
int validate_config(Config */* cfg */);
void print_config(Config */* cfg */);

#endif
