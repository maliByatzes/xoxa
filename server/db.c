/* db.c */

#include "db.h"

DB *newDB(const char *filename)
{
  DB *out = {0};

  int ret = sqlite3_open_v2(
    filename,
    &(out->db),
    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI,
    NULL);

  if (ret != SQLITE_OK) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(out->db));
    sqlite3_close(out->db);
    exit(1);
  }

  if (out->db == NULL) {
    fprintf(stderr, "Cannot allocate database memory for sqlite3 object.");
    sqlite3_close(out->db);
    exit(1);
  }

  return out;
}

void destroyDB(DB *db)
{
  sqlite3_close(db->db);
}
