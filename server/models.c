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

  result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
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
  strncat(select_sql, "SELECT id, name, created_at, updated_at FROM users WHERE", 56);

  if (filter.id != NULL) {
    strncat(select_sql, " id = ?", 7);
  }

  if (filter.name != NULL) {
    strncat(select_sql, " name = ?", 9);
  }

  strncat(select_sql, " ORDER BY id;", 12);
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
        fprintf(stderr, "Memory allocation failed.\n");
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

  const char *select_sql = "SELECT cm.conversation_id FROM conversation_messages AS cm JOIN messages AS m ON cm.message_id = m.id WHERE m.sender_id = ? AND m.receiver_id = ?;";

  result = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return result;
  }

  if ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
    *conv_id = sqlite3_column_int(stmt, 0);
  }

  if (result != SQLITE_DONE) {
    fprintf(stderr, "failed to execute query: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return result;
  }

  sqlite3_finalize(stmt);

  return SQLITE_OK;
}

int createConversation(sqlite3 *db, int *conv_id) 
{
  int result = 0;
  sqlite3_stmt *stmt = NULL;

  const char *insert_sql = "INSERT INTO conversations DEFAULT VALUES;";

  result = sqlite3_exec(db, insert_sql, 0, 0, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to execute query: %s\n", sqlite3_errmsg(db));
    return result;
  }

  *conv_id = (int)sqlite3_last_insert_rowid(db);

  return SQLITE_OK;
}

int createMessage(sqlite3 *db, Message *msg) 
{
  int result = 0;
  sqlite3_stmt *stmt = NULL;
  
  // Create message in the db

  // Do field checking for required values
  if (msg->sender_id <= 0) {
    fprintf(stderr, "sender_id is required.\n");
    return ERR_INVALID;
  }

  if (msg->receiver_id <= 0) {
    fprintf(stderr, "receiver_id is required.\n");
    return ERR_INVALID;
  }

  if (msg->message == NULL) {
    fprintf(stderr, "message is required.\n");
    return ERR_INVALID;
  }

  if ((result = beginTransaction(db)) != SQLITE_OK) {
    return result;
  }

  const char *insert_sql = "INSERT INTO messages (sender_id, receiver_id, message) VALUES (?, ?, ?);";

  result = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_int(stmt, 1, msg->sender_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `sender_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_int(stmt, 2, msg->receiver_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `receiver_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_text(stmt, 3, msg->message, -1, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `message` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
    fprintf(stderr, "failed to insert data: %s\n", sqlite3_errmsg(db));
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

  if (conv_id == 0) {
    result = createConversation(db, &conv_id);
    if (result != SQLITE_OK) {
      rollbackTransaction(db);
      return result;
    }
  }
  
  // If the conversation does exist, use that conversation
  // Add new message to conversations_messages


  if ((result = commitTransaction(db)) != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return result;
  }

  sqlite3_finalize(stmt);

  return SQLITE_OK;
}

/*
int createMessage(sqlite3 *db, Message *message) 
{
  int result = 0;
  sqlite3_stmt *stmt = NULL;

  if (message->sender_id <= 0) {
    fprintf(stderr, "sender_id on message struct is required.");
    return ERR_INVALID;
  }
  if (message->receiver_id <= 0) {
    fprintf(stderr, "receiver_id on message struct is required.");
    return ERR_INVALID;
  }
  if (message->message == NULL) {
    fprintf(stderr, "message on message struct is required.");
    return ERR_INVALID;
  }

  if ((result = beginTransaction(db)) != SQLITE_OK) {
    return result;
  }

  const char *insert_sql = "INSERT INTO messages (sender_id, reciever_id, message, created_at, updated_at) VALUES (?, ?, ?, ?, ?);";

  result = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_int(stmt, 1, message->sender_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `sender_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_int(stmt, 2, message->receiver_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `receiver_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_text(stmt, 3, message->message, -1, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `message` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }
  
  result = sqlite3_bind_text(stmt, 4, message->created_at, -1, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `created_at` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }
  result = sqlite3_bind_text(stmt, 5, message->updated_at, -1, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `updated_at` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
    fprintf(stderr, "failed to insert data: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  message->id = (int)sqlite3_last_insert_rowid(db);

  if ((result = commitTransaction(db)) != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return result;
  }

  sqlite3_finalize(stmt);

  return SQLITE_OK;
}

MessageArr *getMessages(sqlite3 *db, MessageFilter filter, int *error_code) 
{
  int result = 0;
  MessageArr *messages = NULL;
  sqlite3_stmt *stmt = NULL;

  *error_code = 0;

  messages = (MessageArr *)malloc(sizeof(MessageArr));
  if (!messages) {
    *error_code = ERR_NOMEM;
    return NULL;
  }
  messages->messages = NULL;
  messages->count = 0;

  if ((result = beginTransaction(db)) != SQLITE_OK) {
    free(messages);
    *error_code = result;
    return NULL;
  }

  char select_sql[512];
  strncat(select_sql, "SELECT id, sender_id, receiver_id, message, created_at, updated_at FROM messages WHERE", 86);
  
  if (filter.id != NULL) {
    strncat(select_sql, " id = ?", 7);
  }

  if (filter.sender_id != NULL) {
    strncat(select_sql, " sender_id = ?", 14);
  }

  if (filter.receiver_id != NULL) {
    strncat(select_sql, " receiver_id = ?", 16);
  }

  strncat(select_sql, " ORDER BY id;", 12);
  printf("select sql: %s\n", select_sql);

  result = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    free(messages);
    *error_code = result;
    return NULL;
  }

  if (filter.id != NULL && filter.sender_id != NULL && filter.receiver_id != NULL) {
    sqlite3_bind_int(stmt, 1, *filter.id);
    sqlite3_bind_int(stmt, 2, *filter.sender_id);
    sqlite3_bind_int(stmt, 3, *filter.receiver_id);
  } else if (filter.id != NULL && filter.sender_id) {
    sqlite3_bind_int(stmt, 1, *filter.id);
    sqlite3_bind_int(stmt, 2, *filter.sender_id);
  } else if (filter.id != NULL && filter.receiver_id) {
    sqlite3_bind_int(stmt, 1, *filter.id);
    sqlite3_bind_int(stmt, 2, *filter.receiver_id);
  } else if (filter.sender_id != NULL && filter.receiver_id != NULL) {
    sqlite3_bind_int(stmt, 1, *filter.sender_id);
    sqlite3_bind_int(stmt, 2, *filter.receiver_id);
  } else if (filter.id != NULL) {
    sqlite3_bind_int(stmt, 1, *filter.id);
  } else if (filter.sender_id != NULL) {
    sqlite3_bind_int(stmt, 1, *filter.sender_id);
  } else if (filter.receiver_id != NULL) {
    sqlite3_bind_int(stmt, 1, *filter.receiver_id);
  }

  size_t capacity = 10;
  messages->messages = (Message *)malloc(capacity * sizeof(Message));
  if (!messages->messages) {
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    free(messages);
    *error_code = ERR_NOMEM;
    return NULL;
  }

  while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
    if (messages->count >= capacity) {
      capacity *= 2;
      Message *temp = (Message *)realloc(messages->messages, capacity * sizeof(Message));
      if (!temp) {
        fprintf(stderr, "memory allocation failed.\n");
        sqlite3_finalize(stmt);
        rollbackTransaction(db);
        free(messages);
        *error_code = ERR_NOMEM;
        return NULL;
      }
      messages->messages = temp;
    }

    Message *message = &messages->messages[messages->count];

    message->id = sqlite3_column_int(stmt, 0);
    message->sender_id = sqlite3_column_int(stmt, 1);
    message->receiver_id = sqlite3_column_int(stmt, 2);

    const char *created_at = (const char *)sqlite3_column_text(stmt, 3);
    message->created_at = created_at ? strdup(created_at) : NULL;

    const char *updated_at = (const char *)sqlite3_column_text(stmt, 4);
    message->updated_at = updated_at ? strdup(updated_at) : NULL;

    messages->count++;
  }

  if (result != SQLITE_DONE) {
    fprintf(stderr, "failed to execute query: %s\n", sqlite3_errmsg(db));
    freeMessagesArr(messages);
    free(messages);
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    *error_code = result;
    return NULL;
  }

  sqlite3_finalize(stmt);

  if ((result = commitTransaction(db)) != SQLITE_OK) {
    freeMessagesArr(messages);
    free(messages);
    *error_code = result;
    return NULL;
  }

  return messages;
}

void freeMessagesArr(MessageArr *arr) {
  if (arr) {
    for (size_t i = 0; i < arr->count; i++) {
      free(arr->messages[i].message);
      free(arr->messages[i].created_at);
      free(arr->messages[i].updated_at);
    }
    free(arr->messages);
    arr->messages = NULL;
    arr->count = 0;
  }
}

int createConversation(sqlite3 *db, Conversation *conv)
{
  int result = 0;
  sqlite3_stmt *stmt = NULL;

  if ((result = beginTransaction(db)) != SQLITE_OK) {
    return result;
  }

  const char *insert_sql = "INSERT INTO conversations (created_at, updated_at);";

  result = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_text(stmt, 1, conv->created_at, -1, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `created_at` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_text(stmt, 1, conv->updated_at, -1, SQLITE_STATIC);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `updated_at` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
    fprintf(stderr, "failed to insert data: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  conv->id = (int)sqlite3_last_insert_rowid(db);

  if ((result = commitTransaction(db)) != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return result;
  }

  sqlite3_finalize(stmt);

  return result;
}

ConversationArr *getConversationByID(sqlite3 *db, int id, int *error_code) 
{
  int result = 0;
  ConversationArr *convs = NULL;
  sqlite3_stmt *stmt = NULL;

  *error_code = 0;

  convs = (ConversationArr *)malloc(sizeof(ConversationArr));
  if (!convs) {
    *error_code = ERR_NOMEM;
    return NULL;
  }
  convs->convs = NULL;
  convs->count = 0;

  if ((result = beginTransaction(db)) != SQLITE_OK) {
    free(convs);
    *error_code = result;
    return NULL;
  }

  char select_sql[512];
  strncat(select_sql, "SELECT id, created_at, updated_at FROM conversations WHERE", 58);

  if (id > 0) {
    strncat(select_sql, " id = ?", 7);
  }
  
  strncat(select_sql, " ORDER BY id;", 12);
  printf("select sql: %s\n", select_sql);

  result = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    free(convs);
    *error_code = result;
    return NULL;
  }

  if (id > 0) {
    sqlite3_bind_int(stmt, 1, id);
  }

  size_t capacity = 10;
  convs->convs = (Conversation *)malloc(capacity * sizeof(Conversation));
  if (!convs->convs) {
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    free(convs);
    *error_code = ERR_NOMEM;
    return NULL;
  }

  while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
    if (convs->count >= capacity) {
      capacity *= 2;
      Conversation *temp = (Conversation *)realloc(convs->convs, capacity * sizeof(Conversation));
      if (!temp) {
        fprintf(stderr, "memory allocation failed.\n");
        sqlite3_finalize(stmt);
        rollbackTransaction(db);
        free(convs);
        *error_code = ERR_NOMEM;
        return NULL;
      }
      convs->convs = temp;
    }

    Conversation *conv = &convs->convs[convs->count];

    conv->id = sqlite3_column_int(stmt, 0);
    const char *created_at = (const char *)sqlite3_column_text(stmt, 1);
    conv->created_at = created_at ? strdup(created_at) : NULL;
    const char *updated_at = (const char *)sqlite3_column_text(stmt, 2);
    conv->updated_at = updated_at ? strdup(updated_at) : NULL;

    convs->count++;
  }

  if (result != SQLITE_DONE) {
    fprintf(stderr, "failed to execute query: %s\n", sqlite3_errmsg(db));
    freeConversationArr(convs);
    free(convs);
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    *error_code = result;
    return NULL;
  }

  sqlite3_finalize(stmt);

  if ((result = commitTransaction(db)) != SQLITE_OK) {
    freeConversationArr(convs);
    free(convs);
    *error_code = result;
    return NULL;
  }

  return convs;
}

void freeConversationArr(ConversationArr *arr) 
{
  if (arr) {
    for (size_t i = 0; i < arr->count; i++) {
      free(arr->convs[i].created_at);
      free(arr->convs[i].updated_at);
    }
    free(arr->convs);
    arr->convs = NULL;
    arr->count = 0;
  }
}

int createConvParticipant(sqlite3 *db, ConversationParticipant *conv_part) 
{
  int result = 0;
  sqlite3_stmt *stmt;

  if ((result = beginTransaction(db)) != SQLITE_OK) {
    return result;
  }

  if (conv_part->user_id <= 0) {
    fprintf(stderr, "user_id on conversation participant struct is required.\n");
    rollbackTransaction(db);
    return ERR_INVALID;
  }

  if (conv_part->conversation_id <= 0) {
    fprintf(stderr, "conversation_id on conversation participant struct is required.\n");
    rollbackTransaction(db);
    return ERR_INVALID;
  }

  const char *insert_sql = "INSERT INTO conversation_particapants (user_id, conversation_id) VALUES (?, ?);";

  result = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_int(stmt, 1, conv_part->user_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `user_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_bind_int(stmt, 2, conv_part->conversation_id);
  if (result != SQLITE_OK) {
    fprintf(stderr, "failed to bind `conversation_id` param: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    rollbackTransaction(db);
    return result;
  }

  result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
    fprintf(stderr, "failed to insert data: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return result;
  }

  if ((result = commitTransaction(db)) != SQLITE_OK) {
    sqlite3_finalize(stmt);
    return result;
  }

  sqlite3_finalize(stmt);

  return SQLITE_OK;
}
*/
