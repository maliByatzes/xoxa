/* models.h */

#ifndef __models_h
#define __models_h

#include <sqlite3.h>
#include <stddef.h>
#include <time.h>

#define ERR_INVALID 69
#define ERR_NOMEM 70
#define ERR_REQUIRED 71

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

int createUser(sqlite3 *db, User *user);
UserArr *getUsers(sqlite3 *db, UserFilter filter, int *err_code);
void freeUsersArr(UserArr *arr);

int createMessage(sqlite3 *db, Message *msg);
int validateMessageStruct(Message *msg);
MessageArr *getMessages(sqlite3 *db, int sender_id, int recv_id, int *err_code);
void freeMessagesArr(MessageArr *arr);

#endif
