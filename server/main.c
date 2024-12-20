/* main.c */

#include "db.h"
#include "server.h"

int main(int argc, char **argv)
{

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <database URI>\n", argv[0]);
    return 1;
  }

#if defined(_WIN32)
  WSAData d;
  if (WSAStartup(MAKEWORD(2,2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return 1;
  }
#endif

  sqlite3 *db;
  db = newDB(argv[1]);
  if (db == NULL) {
    return 1;
  }

  SOCKET connfd = create_server();
  run_server(connfd, db);

#if defined(_WIN32)
  WSACleanup();
#endif

  sqlite3_close(db);

  return 0;
}
