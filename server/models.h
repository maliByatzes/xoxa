/* models.h */

#ifndef __models_h
#define __models_h

typedef struct User_ {
  int id;
  char *name;
  char *created_at;
  char *updated_at;
} User;

typedef struct Message_ {
  int id;
  int sender_id;
  int receiver_id;
  char *message;
  char *created_at;
  char *updated_at;
} Message;

typedef struct Conversation_ {
  int id;
  char *created_at;
  char *updated_at;
} Conversation;

typedef struct ConversationParticipant_ {
  int user_id;
  int conversation_id;
} ConversationParticipant;

typedef struct ConversationMessage_ {
  int conversation_id;
  int message_id;
} ConversationMessage;

#endif
