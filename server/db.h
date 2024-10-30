/* db.h */

#ifndef __db_h
#define __db_h

#include <sqlite3.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#define MIGRATION_DIR "./server/db/migration"

sqlite3 *newDB(const char */* filename */);
void destroyDB(sqlite3 */* db */);

void runMigration(sqlite3 */* db */);
void migrateFile(const char *file);

#endif
