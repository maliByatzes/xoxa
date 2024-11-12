/* config.c */

#include "config.h"

char *get_env_or_default(const char *name, char *default_value)
{
  char *value = getenv(name);
  return value ? value : default_value;
}

int get_env_bool(const char *name)
{
  char *value = getenv(name);
  if (!value) return 0;

  return (strcmp(value, "1") == 0 ||
          strcmp(value, "true") == 0 ||
          strcmp(value, "yes") == 0 ||
          strcmp(value, "on") == 0);
}

Config *load_config()
{
  Config *cfg = (Config *)malloc(sizeof(Config));

  cfg->server_ip = strdup(get_env_or_default("XOXA_SERVER_IP", "127.0.0.1"));
  cfg->server_port = strdup(get_env_or_default("XOXA_SERVER_PORT", "8080"));
  cfg->client_name = strdup(get_env_or_default("XOXA_CLIENT_NAME", ""));
  cfg->debug_mode = get_env_bool("XOXA_DEBUG_MODE");

  return cfg;
}

void free_config(Config *cfg)
{
  free(cfg->server_ip);
  free(cfg->server_port);
  free(cfg->client_name);
}

int validate_config(Config *cfg)
{
  if (strlen(cfg->client_name) == 0) {
    fprintf(stderr, "error: XOXA_CLIENT_NAME environment variable is required.\n");
    return 0;
  }

  return 1;
}

void print_config(Config *cfg)
{
  printf("Configuration:\n");
  printf("  XOXA_SERVER_IP: %s\n", cfg->server_ip);
  printf("  XOXA_SERVER_PORT: %s\n", cfg->server_port);
  printf("  XOXA_CLIENT_NAME: %s\n", cfg->client_name);
  printf("  XOXA_DEBUG_MODE: %s\n", cfg->debug_mode ? "enabled" : "disabled");
}
