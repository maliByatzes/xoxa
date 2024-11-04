/* models.c */

#include "models.h"
#include "db.h"

int createUser(sqlite3 *db, User *user) 
{
  int result;
  sqlite3_stmt *stmt;
  
  if ((result = beginTransaction(db)) != SQLITE_OK) {
    return result;    
  }

  time_t current_time;
  current_time = time(NULL);
  user->created_at = asctime(gmtime(&current_time));
  user->updated_at = strdup(user->created_at);

  const char *insert_sql = "INSERT INTO users (name, created_at, updated_at) VALUES (?, ?, ?);";

  result = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_text(stmt, 1, user->name, -1, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `name` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_text(stmt, 2, user->created_at, -1, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `created_at` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_text(stmt, 2, user->updated_at, -1, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `updated_at` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_step(stmt);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to insert data: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  user->id = (int)sqlite3_last_insert_rowid(db);

  if ((result = commitTransaction(db)) != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return result;
  }

  sqlite3_finalize(stmt);

  return SQLITE_OK;
}

UserArr *getUsers(sqlite3 *db, UserFilter filter, int *err_code) 
{
  int result;
  UserArr *users = NULL;
  sqlite3_stmt *stmt;

  *err_code = 0;

  users = (UserArr *)malloc(sizeof(UserArr));
  if (!users) {
    *err_code = SQLITE_NOMEM;
    return NULL;
  }
  users->users = NULL;
  users->count = 0;

  if ((result = beginTransaction(db)) != SQLITE_OK) {
    free(users);
    *err_code = result;
    return NULL;
  }
  
  char select_sql[512];
  strncat(select_sql, "SELECT id, name, created_at, updated_at FROM users WHERE", 56);

  if (filter.id != NULL) {
    strncat(select_sql, " id = ?", 7);
  }

  if (filter.name != NULL) {
    strncat(select_sql, " name = ?", 9);
  }

  strncat(select_sql, "ORDER BY id;", 12);
  printf("select sql: %s\n", select_sql);

  result = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    free(users);
    *err_code = result;
    return NULL;
  }

  // NOTE: Could or Could not work
  if (filter.id != NULL && filter.name != NULL) {
    sqlite3_bind_int(stmt, 1, *filter.id);
    sqlite3_bind_text(stmt, 2, *filter.name, -1, SQLITE_STATIC);
  } else if (filter.id != NULL) {
    sqlite3_bind_int(stmt, 1, *filter.id);
  } else if (filter.name != NULL) {
    sqlite3_bind_text(stmt, 1, *filter.name, -1, SQLITE_STATIC);
  }

  size_t capacity = 10;
  users->users = (User *)malloc(capacity * sizeof(User));
  if (!users->users) {
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    free(users);
    *err_code = SQLITE_NOMEM;
    return NULL;
  }
  
  while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
    if (users->count >= capacity) {
      capacity *= 2;
      User *temp = (User *)realloc(users->users, capacity * sizeof(User));
      if (!temp) {
        fprintf(stderr, "Memory allocation failed.\n");
        freeUsersArr(users);
        free(users);
        sqlite3_finalize(stmt);
        rollbackTransaction(db);
        *err_code = SQLITE_NOMEM;
        return NULL;
      }
      users->users = temp;
    }

    User *user = &users->users[users->count];
    
    user->id = sqlite3_column_int(stmt, 0);

    const char *name = (const char *)sqlite3_column_text(stmt, 1);
    user->name = name ? strdup(name) : NULL;

    const char *created_at = (const char *)sqlite3_column_text(stmt, 2);
    user->created_at = created_at ? strdup(created_at) : NULL;

    const char *updated_at = (const char *)sqlite3_column_text(stmt, 3);
    user->updated_at = updated_at ? strdup(updated_at) : NULL;

    users->count++;
  }

  if (result != SQLITE_DONE) {
    fprintf(stderr, "failed to execute query: %s\n", sqlite3_errmsg(db));
    freeUsersArr(users);
    free(users);
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    *err_code = result;
    return NULL;
  }

  sqlite3_finalize(stmt);

  if ((result = commitTransaction(db))) {
    freeUsersArr(users);
    free(users);
    *err_code = result;
    return NULL;
  }

  return users;
}

void freeUsersArr(UserArr *arr) 
{
  if (arr) {
    for (size_t i = 0; i < arr->count; i++) {
      free(arr->users[i].name);
      free(arr->users[i].created_at);
      free(arr->users[i].updated_at);
    }
    free(arr->users);
    arr->users = NULL;
    arr->count = 0;
  }
}

int createMessage(sqlite3 *db, Message *message) 
{}

Message *getMessages(sqlite3 *db, MessageFilter filter) 
{}

int createConversation(sqlite3 *db, Conversation *conv)
{}

Conversation *getConversationByID(sqlite3 *db, int id) 
{}

int createConvParticipant(sqlite3 *db, ConversationParticipant *conv_part) 
{}

ConversationParticipant *getConvParticipants(sqlite3 *db, ConversationParticipantFilter filter) 
{}
