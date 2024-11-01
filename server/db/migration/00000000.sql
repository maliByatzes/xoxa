CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE messages (
    id INTEGER PRIMARY KEY,
    sender_id INTEGER NOT NULL REFERENCES users (id),
    receiver_id INTEGER NOT NULL REFERENCES users (id),
    message TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE conversations (
    id INTEGER PRIMARY KEY,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE conversation_participants (
    user_id INTEGER NOT NULL REFERENCES users (id),
    conversation_id INTEGER NOT NULL REFERENCES conversations (id),
    PRIMARY KEY (user_id, conversation_id)
);

CREATE TABLE conversation_messages (
    conversation_id INTEGER NOT NULL REFERENCES conversations (id),
    message_id INTEGER NOT NULL REFERENCES messages (id),
    PRIMARY KEY (conversation_id, message_id)
);

CREATE INDEX messages_sender_id_idx ON messages (sender_id);
CREATE INDEX messages_receiver_id_idx ON messages (receiver_id);
