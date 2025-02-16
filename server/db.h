/* db.h */

#ifndef __db_h
#define __db_h

#include <sqlite3.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#define MIGRATION_DIR "./server/db/migration"

sqlite3 *newDB(const char */* filename */);

int runMigration(sqlite3 */* db */);
int migrateFile(sqlite3 */* db */, const char */* file */);

int beginTransaction(sqlite3 */* db */);
int commitTransaction(sqlite3 */* db */);
int rollbackTransaction(sqlite3 */* db */);

#endif
