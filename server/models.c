/* models.c */

#include "models.h"
#include "db.h"
#include <unistd.h>

int createUser(sqlite3 *db, User *user) 
{
  int result;
  sqlite3_stmt *stmt;
  
  if ((result = beginTransaction(db)) != SQLITE_OK) {
    return result;    
  }

  const char *insert_sql = "INSERT INTO users (name) VALUES (?);";

  result = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createUser => failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_text(stmt, 1, user->name, -1, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createUser => failed to bind `name` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
    fprintf(stderr, "createUser => failed to insert data: %s\n", sqlite3_errmsg(db));
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
  int result = 0;
  UserArr *users = NULL;
  sqlite3_stmt *stmt = NULL;

  *err_code = 0;

  users = (UserArr *)malloc(sizeof(UserArr));
  if (!users) {
    *err_code = ERR_NOMEM;
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
  memset(select_sql, 0, sizeof(select_sql));
  
  strncat(select_sql, "SELECT id, name, created_at, updated_at FROM users WHERE", 56);

  if (filter.id != NULL) {
    strncat(select_sql, " id = ?", 7);
  }

  if (filter.name != NULL) {
    strncat(select_sql, " name = ?", 9);
  }

  strncat(select_sql, " ORDER BY id;", 13);
  printf("select sql: %s\n", select_sql);

  result = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "getUsers => failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    free(users);
    *err_code = result;
    return NULL;
  }

  // NOTE: Could or Could not work
  if (filter.id != NULL && filter.name != NULL) {
    sqlite3_bind_int(stmt, 1, *filter.id);
    sqlite3_bind_text(stmt, 2, filter.name, -1, SQLITE_STATIC);
  } else if (filter.id != NULL) {
    sqlite3_bind_int(stmt, 1, *filter.id);
  } else if (filter.name != NULL) {
    sqlite3_bind_text(stmt, 1, filter.name, -1, SQLITE_STATIC);
  }

  size_t capacity = 10;
  users->users = (User *)malloc(capacity * sizeof(User));
  if (!users->users) {
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    free(users);
    *err_code = ERR_NOMEM;
    return NULL;
  }
  
  while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
    if (users->count >= capacity) {
      capacity *= 2;
      User *temp = (User *)realloc(users->users, capacity * sizeof(User));
      if (!temp) {
        fprintf(stderr, "getUsers => Memory allocation failed.\n");
        freeUsersArr(users);
        free(users);
        sqlite3_finalize(stmt);
        rollbackTransaction(db);
        *err_code = ERR_NOMEM;
        return NULL;
      }
      users->users = temp;
    }

    User *user = &users->users[users->count];
    
    user->id = sqlite3_column_int(stmt, 0);

    const char *name = (const char *)sqlite3_column_text(stmt, 1);
    user->name = name ? strdup(name) : NULL;

    users->count++;
  }

  if (result != SQLITE_DONE) {
    fprintf(stderr, "getUsers => failed to execute query: %s\n", sqlite3_errmsg(db));
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

  *err_code = SQLITE_OK;
  return users;
}

void freeUsersArr(UserArr *arr) 
{
  if (arr) {
    for (size_t i = 0; i < arr->count; i++) {
      free(arr->users[i].name);
    }
    free(arr->users);
    arr->users = NULL;
    arr->count = 0;
  }
}

int getConversationBySenderIDRecvID(sqlite3 *db, int *conv_id, int sender_id, int recv_id)
{
  int result = 0;
  *conv_id = 0;
  sqlite3_stmt *stmt;

  const char *select_sql = "SELECT cm.conversation_id FROM conversation_messages AS cm JOIN messages AS m ON cm.message_id = m.id WHERE m.sender_id = ? AND m.receiver_id = ? ORDER BY cm.conversation_id LIMIT 1;";

  result = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "getConversationBySenderRecvID => failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return result;
  }

  result = sqlite3_bind_int(stmt, 1, sender_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "getConversationBySenderRecvID => failed to bind `sender_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return result;
  }

  result = sqlite3_bind_int(stmt, 2, recv_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "getConversationBySenderRecvID => failed to bind `receiver_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return result;
  }

  while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
    *conv_id = sqlite3_column_int(stmt, 0);
  }

  if (result != SQLITE_DONE) {
    fprintf(stderr, "getConversationBySenderRecvID => failed to execute query: %d %s\n", result, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return result;
  }

  sqlite3_finalize(stmt);

  return SQLITE_OK;
}

int createConversation(sqlite3 *db, int *conv_id) 
{
  int result = 0;

  const char *insert_sql = "INSERT INTO conversations DEFAULT VALUES;";

  result = sqlite3_exec(db, insert_sql, 0, 0, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createConversation => failed to execute query: %s\n", sqlite3_errmsg(db));
    return result;
  }

  *conv_id = (int)sqlite3_last_insert_rowid(db);

  return SQLITE_OK;
}

int createConvParticipant(sqlite3 *db, int conv_id, int user_id) 
{
  int result = 0;
  sqlite3_stmt *stmt = NULL;

  const char *insert_sql = "INSERT INTO conversation_participants (user_id, conversation_id) VALUES (?, ?);";

  result = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createConvPartipant => failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return result;
  }

  result = sqlite3_bind_int(stmt, 1, user_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createConvParticipant => failed to bind `user_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return result;
  }

  result = sqlite3_bind_int(stmt, 2, conv_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createConvParticipant => failed to bind `conv_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return result;
  }

  result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
    fprintf(stderr, "createConvParticipant => failed to insert data: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return result;
  }

  sqlite3_finalize(stmt);

  return SQLITE_OK;
}

int createConvMessages(sqlite3 *db, int conv_id, int message_id) 
{
  int result = 0;
  sqlite3_stmt *stmt = NULL;

  const char *insert_sql = "INSERT INTO conversation_messages (conversation_id, message_id) VALUES (?, ?);";

  result = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createConvMessages => failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return result;
  }

  result = sqlite3_bind_int(stmt, 1, conv_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createConvMessages => failed to bind `conversation_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return result;
  }

  result = sqlite3_bind_int(stmt, 2, message_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createConvMessages => failed to bind `message_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return result;
  }

  result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
    fprintf(stderr, "createConvMessages => failed to insert data: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return result;
  }

  sqlite3_finalize(stmt);

  return SQLITE_OK;
}

int createMessage(sqlite3 *db, Message *msg) 
{
  int result = 0;
  sqlite3_stmt *stmt = NULL;
  
  // Create message in the db

  // Do field checking for required values
  if (msg->sender_id <= 0) {
    fprintf(stderr, "createMessage => sender_id is required.\n");
    return ERR_INVALID;
  }

  if (msg->receiver_id <= 0) {
    fprintf(stderr, "createMessage => receiver_id is required.\n");
    return ERR_INVALID;
  }

  if (msg->message == NULL) {
    fprintf(stderr, "createMessage => message is required.\n");
    return ERR_INVALID;
  }

  if ((result = beginTransaction(db)) != SQLITE_OK) {
    return result;
  }

  const char *insert_sql = "INSERT INTO messages (sender_id, receiver_id, message) VALUES (?, ?, ?);";

  result = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createMessage => failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_int(stmt, 1, msg->sender_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createMessage => failed to bind `sender_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_int(stmt, 2, msg->receiver_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createMessage => failed to bind `receiver_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_text(stmt, 3, msg->message, -1, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    fprintf(stderr, "createMessage => failed to bind `message` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
    fprintf(stderr, "createMessage => failed to insert data: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }
  sqlite3_finalize(stmt);

  msg->id = (int)sqlite3_last_insert_rowid(db);
  
  // Check for existing conversations between sender_id and recv_id

  int conv_id = 0;
  if ((result = getConversationBySenderIDRecvID(db, &conv_id, msg->sender_id, msg->receiver_id)) != SQLITE_OK) {
    rollbackTransaction(db);
    return result;
  }
  
  // If the conversation doesnt exist, create a new one
  // Create a new conversations_participants
  // Create a new conversations_messages

  printf("conv_id: %d\n", conv_id);
  if (conv_id == 0) {

    result = createConversation(db, &conv_id);
    if (result != SQLITE_OK) {
      rollbackTransaction(db);
      return result;
    }

    result = createConvParticipant(db, conv_id, msg->sender_id);
    if (result != SQLITE_OK) {
      rollbackTransaction(db);
      return result;
    }

    result = createConvParticipant(db, conv_id, msg->receiver_id);
    if (result != SQLITE_OK) {
      rollbackTransaction(db);
      return result;
    }

    result = createConvMessages(db, conv_id, msg->id);
    if (result != SQLITE_OK) {
      rollbackTransaction(db);
      return result;
    }

  } else if (conv_id > 0) {

    // If the conversation does exist, use that conversation
    // Add new message to conversations_messages
    result = createConvMessages(db, conv_id, msg->id);
    if (result != SQLITE_OK) {
      rollbackTransaction(db);
      return result;
    }
  }

  if ((result = commitTransaction(db)) != SQLITE_OK) {
    return result;
  }

  return SQLITE_OK;
}
