/* models.h */

#ifndef __models_h
#define __models_h

#include <sqlite3.h>
#include <stddef.h>
#include <time.h>

typedef struct User_ {
  int id;
  char *name;
  char *created_at;
  char *updated_at;
} User;

typedef struct UserArr_ {
  User *users;
  size_t count;
} UserArr;

typedef struct UserFilter_ {
  int *id;
  char **name;
} UserFilter;

typedef struct Message_ {
  int id;
  int sender_id;
  int receiver_id;
  char *message;
  char *created_at;
  char *updated_at;
} Message;

typedef struct MessageFilter_ {
  int *id;
  int *sender_id;
  int *receiver_id;
} MessageFilter;

typedef struct Conversation_ {
  int id;
  char *created_at;
  char *updated_at;
} Conversation;

typedef struct ConversationParticipant_ {
  int user_id;
  int conversation_id;
} ConversationParticipant;

typedef struct ConversationParticipantFilter_ {
  int *user_id;
  int *conversation_id;
} ConversationParticipantFilter;

typedef struct ConversationMessage_ {
  int conversation_id;
  int message_id;
} ConversationMessage;

typedef struct ConversationMessageFilter_ {
  int *conversation_id;
  int *message_id;
} ConversationMessageFilter;

int createUser(sqlite3 *db, User *user);
UserArr *getUsers(sqlite3 *db, UserFilter filter, int *err_code);
void freeUsersArr(UserArr *arr);

int createMessage(sqlite3 *db, Message *message);
Message *getMessages(sqlite3 *db, MessageFilter filter);

int createConversation(sqlite3 *db, Conversation *conv);
Conversation *getConversationByID(sqlite3 *db, int id);

int createConvParticipant(sqlite3 *db, ConversationParticipant *conv_part);
ConversationParticipant *getConvParticipants(sqlite3 *db, ConversationParticipantFilter filter);

#endif
