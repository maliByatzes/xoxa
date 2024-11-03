/* models.c */

#include "models.h"
#include "db.h"
#include <time.h>

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

  const char *insert_sql = "INSERT INTO users (name, created_at, updated_at) VALUES (?, ?, ?) RETURNING id";

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

User *getUsers(sqlite3 *db, UserFilter filter) 
{}

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
