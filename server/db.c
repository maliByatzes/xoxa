/* db.c */

#include "db.h"
#include <dirent.h>

sqlite3 *newDB(const char *filename)
{
  sqlite3 *db;
  char *err_msg;
  int ret;

  ret = sqlite3_open_v2(
    filename,
    &db,
    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI,
    NULL);

  if (ret) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }

  if (db == NULL) {
    fprintf(stderr, "Cannot allocate database memory for sqlite3 object.");
    sqlite3_close(db);
    exit(1);
  }

  // Enable WAL
  ret = sqlite3_exec(db, "PRAGMA journal_mode = wal;", NULL, 0, &err_msg);
  if (ret != SQLITE_OK) {
    fprintf(stderr, "enable error: %s\n", err_msg);
    sqlite3_free(err_msg);
    exit(1);
  }

  // Enable foreign keys support
  ret = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, 0, &err_msg);
  if (ret != SQLITE_OK) {
    fprintf(stderr, "enable foreign keys support: %s\n", err_msg);
    sqlite3_free(err_msg);
    exit(1);
  }

  runMigration(db);

  return db;
}

void destroyDB(sqlite3 *db)
{
  sqlite3_close(db);
}

void runMigration(sqlite3 *db)
{
  int result;
  char *err_msg;

  // Create migrations table
  result = sqlite3_exec(
    db,
    "CREATE TABLE IF NOT EXISTS migrations (name TEXT PRIMARY KEY);",
    NULL,
    0,
    &err_msg
  );
  if (result != SQLITE_OK) {
    fprintf(stderr, "create migrations table error: %s\n", err_msg);
    sqlite3_free(err_msg);
    exit(1);
  }

  struct dirent *dp;
  DIR *dir = opendir(MIGRATION_DIR);

  if (dir == NULL) {
    fprintf(stderr, "cannot open migration directory: %s\n", MIGRATION_DIR);
    exit(1);
  }

  while ((dp = readdir(dir)) != NULL) {
    printf("%s\n", dp->d_name);
  }

  closedir(dir);
}

