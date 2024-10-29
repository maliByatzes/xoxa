/* db.h */

#ifndef __db_h
#define __db_h

#include <sqlite3.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct DB_ {
  sqlite3 *db;
  char *filename;
} DB;

DB *newDB(const char */* filename */);
void destroyDB(DB */* db */);

#endif
