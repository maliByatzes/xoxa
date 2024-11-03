/* db.c */

#include "db.h"

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
    return NULL;
}

  if (db == NULL) {
    fprintf(stderr, "Cannot allocate database memory for sqlite3 object.");
    sqlite3_close(db);
    return NULL;
  }

  // Enable WAL
  ret = sqlite3_exec(db, "PRAGMA journal_mode = wal;", NULL, 0, &err_msg);
  if (ret != SQLITE_OK) {
    fprintf(stderr, "enable error: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);
    return NULL;
  }

  // Enable foreign keys support
  ret = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, 0, &err_msg);
  if (ret != SQLITE_OK) {
    fprintf(stderr, "enable foreign keys support: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);
    return NULL;
  }

  int s = runMigration(db);
  if (s == 1) {
    sqlite3_close(db);
    return NULL;
  }

  return db;
}

int runMigration(sqlite3 *db)
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
    return result;
  }

  struct dirent *dp;
  DIR *dir = opendir(MIGRATION_DIR);

  if (dir == NULL) {
    fprintf(stderr, "cannot open migration directory: %s\n", MIGRATION_DIR);
    return 1;
  }

  // TODO: sort the filenames in the directory in ascending order
  // NOTE: use one file for now
  while ((dp = readdir(dir)) != NULL) {
    int s = migrateFile(db, dp->d_name);
    if (s == 1) {
      closedir(dir);
      return s;
    }
  }

  closedir(dir);

  return SQLITE_OK;
}

int migrateFile(sqlite3 *db, const char *file) 
{
  int result, n;
  char *err_msg;
  sqlite3_stmt *stmt;

  // Begin a transaction
  if ((result = beginTransaction(db)) != SQLITE_OK) {
    return result;
  }
  
  // Ensure migration has not been 
  const char *select_sql = "SELECT COUNT(*) FROM migrations WHERE name = ?";

  result = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    return result;
  }

  sqlite3_bind_text(stmt, 1, file, -1, SQLITE_STATIC);

  while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
    n = sqlite3_column_int(stmt, 0);
    // printf("n: %d\n", n);
  }

  if (result != SQLITE_DONE) {
    fprintf(stderr, "failed to execute query: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  sqlite3_finalize(stmt);

  // Skip migrating the current file
  if (n != 0) {
    rollbackTransaction(db);
    return SQLITE_OK;    
  }

  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd() error");
    rollbackTransaction(db);
    return 1;
  }
  
  char filepath[2048];
  memset(filepath, 0, sizeof(filepath));
  snprintf(filepath, sizeof(filepath), "%s/server/db/migration/%s", cwd, file);
  
  FILE *fptr;
  fptr = fopen(filepath, "r");

  if (fptr == NULL) {
    rollbackTransaction(db);
    perror("fopen error");
    return 1;
  }

  char buffer[8192];
  memset(buffer, 0, sizeof(buffer));
  char temp[512];
  memset(temp, 0, sizeof(temp));

  while ((fgets(temp, 512, fptr)) != NULL) {
    strncat(buffer, temp, strlen(temp));
    memset(temp, 0, sizeof(temp));
  }

  result = sqlite3_exec(db, buffer, 0, 0, &err_msg);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to run migration buffer query: %s\n", err_msg);
    sqlite3_free(err_msg);
    rollbackTransaction(db);
    return result;
  }

  // Insert a record in migrations table
  const char *insert_sql = "INSERT INTO migrations (name) VALUES (?)";

  result = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    return result;
  }

  sqlite3_bind_text(stmt, 1, file, -1, SQLITE_STATIC);

  result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
    fprintf(stderr, "failed to execute query: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  sqlite3_finalize(stmt);

  // Commit transaction
  if ((result = commitTransaction(db)) != SQLITE_OK) {
    return result;
  }

  return SQLITE_OK;
}

int beginTransaction(sqlite3 *db) 
{
  int result;
  char *err_msg;

  result = sqlite3_exec(db, "BEGIN;", NULL, 0, &err_msg);
  if (result != SQLITE_OK) {
    fprintf(stderr, "begin transaction error: %s\n", err_msg);
    sqlite3_free(err_msg);
    return result;
  }

  return SQLITE_OK;
}

int commitTransaction(sqlite3 *db) 
{
  int result;
  char *err_msg;  

  result = sqlite3_exec(db, "ROLLBACK;", NULL, 0, &err_msg);
  if (result != SQLITE_OK) {
    fprintf(stderr, "rollback transaction error: %s\n", err_msg);
    sqlite3_free(err_msg);
    return result;
  }

  return SQLITE_OK;
}

int rollbackTransaction(sqlite3 *db) 
{
  int result;
  char *err_msg;  

  result = sqlite3_exec(db, "ROLLBACK;", NULL, 0, &err_msg);
  if (result != SQLITE_OK) {
    fprintf(stderr, "rollback transaction error: %s\n", err_msg);
    sqlite3_free(err_msg);
    return result;
  }

  return SQLITE_OK;
}
