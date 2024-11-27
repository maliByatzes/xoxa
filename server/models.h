/* models.h */

#ifndef __models_h
#define __models_h

#include <sqlite3.h>
#include <stddef.h>
#include <time.h>

#define ERR_INVALID 69
#define ERR_NOMEM 70

typedef struct User_ {
  int id;
  char *name;
} User;

typedef struct UserArr_ {
  User *users;
  size_t count;
} UserArr;

typedef struct UserFilter_ {
  int *id;
  char *name;
} UserFilter;

typedef struct Message_ {
  int id;
  int sender_id;
  int receiver_id;
  char *message;
} Message;

typedef struct MessageArr_ {
  Message *messages;
  size_t count;
} MessageArr;

/*
typedef struct Conversation_ {
  int id;
} Conversation;

typedef struct ConversationParticipant_ {
  int user_id;
  int conversation_id;
} ConversationParticipant;

typedef struct ConversationParticipantArr_ {
  ConversationParticipant *conv_parts;
  size_t count;
} ConversationParticipantArr;

typedef struct ConversationMessage_ {
  int conversation_id;
  int message_id;
} ConversationMessage;

typedef struct ConversationMessageArr_ {
  ConversationMessage *conv_msgs;
  size_t count;
} ConversationMessageArr;

typedef struct ConversationArr_ {
  Conversation *convs;
  ConversationParticipantArr *conv_parts;
  ConversationMessageArr *conv_msgs;
  size_t count;
} ConversationArr;*/

int createUser(sqlite3 *db, User *user);
UserArr *getUsers(sqlite3 *db, UserFilter filter, int *err_code);
void freeUsersArr(UserArr *arr);

int createMessage(sqlite3 *db, Message *msg);
MessageArr *getMessages(sqlite3 *db, int sender_id, int recv_id, int *err_code);
void freeMessagesArr(MessageArr *arr);

/*
int createMessage(sqlite3 *db, Message *message);
MessageArr *getMessages(sqlite3 *db, MessageFilter filter, int *error_code);
void freeMessagesArr(MessageArr *arr);

ConversationArr *getConversationByID(sqlite3 *db, int id, int *error_code);
void freeConversationArr(ConversationArr *arr);

int createConvParticipant(sqlite3 *db, ConversationParticipant *conv_part);

int createConvMessage(sqlite3 *db, ConversationMessage *conv_msg);
*/

#endif
