CREATE TYPE message_type AS ENUM (
    'text',
    'image',
    'voice',
    'file',
    'Sticker'
);

CREATE TABLE connect_tokens (
    token character varying(64) NOT NULL,
    user_id integer NOT NULL,
    created_at timestamp without time zone DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE dialog_last_message (
    dialog_id bigint NOT NULL,
    last_msg_id bigint,
    last_msg_text text,
    last_msg_sender_id integer,
    last_msg_timestamp timestamp with time zone
);

CREATE TABLE dialog_participants (
    dialog_id bigint NOT NULL,
    user_id integer NOT NULL,
    joined_at timestamp with time zone DEFAULT now(),
    last_read_msg_id bigint DEFAULT 0,
    is_muted boolean DEFAULT false,
    is_pinned boolean DEFAULT false
);

CREATE TABLE dialogs (
    id bigint NOT NULL,
    created_at timestamp with time zone DEFAULT now(),
    updated_at timestamp with time zone DEFAULT now()
);

CREATE SEQUENCE dialogs_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
ALTER SEQUENCE dialogs_id_seq OWNED BY dialogs.id;

CREATE TABLE messages (
    id bigint NOT NULL,
    dialog_id bigint NOT NULL,
    sender_id integer NOT NULL,
    type smallint NOT NULL,
    created_at timestamp with time zone DEFAULT now(),
    payload jsonb NOT NULL
);

CREATE SEQUENCE messages_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
ALTER SEQUENCE messages_id_seq OWNED BY messages.id;

CREATE TABLE user_sessions (
    user_id integer NOT NULL,
    token character varying(64) NOT NULL,
    created_at timestamp without time zone DEFAULT CURRENT_TIMESTAMP,
    last_activity timestamp without time zone DEFAULT CURRENT_TIMESTAMP,
    expires_at timestamp without time zone GENERATED ALWAYS AS ((last_activity + '30 days'::interval)) STORED,
    device_info text,
    ip_address character varying(45)
);

CREATE TABLE users (
    id integer NOT NULL,
    username character varying(32) NOT NULL,
    password_hash character varying(128) NOT NULL,
    created_at timestamp without time zone DEFAULT CURRENT_TIMESTAMP
);

CREATE SEQUENCE users_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
ALTER SEQUENCE users_id_seq OWNED BY users.id;
ALTER TABLE ONLY dialogs ALTER COLUMN id SET DEFAULT nextval('dialogs_id_seq'::regclass);
ALTER TABLE ONLY messages ALTER COLUMN id SET DEFAULT nextval('messages_id_seq'::regclass);
ALTER TABLE ONLY users ALTER COLUMN id SET DEFAULT nextval('users_id_seq'::regclass);
ALTER TABLE ONLY connect_tokens
    ADD CONSTRAINT connect_tokens_pkey PRIMARY KEY (token);
ALTER TABLE ONLY dialog_last_message
    ADD CONSTRAINT dialog_last_message_pkey PRIMARY KEY (dialog_id);
ALTER TABLE ONLY dialog_participants
    ADD CONSTRAINT dialog_participants_pkey PRIMARY KEY (dialog_id, user_id);
ALTER TABLE ONLY dialogs
    ADD CONSTRAINT dialogs_pkey PRIMARY KEY (id);
ALTER TABLE ONLY messages
    ADD CONSTRAINT messages_pkey PRIMARY KEY (id);
ALTER TABLE ONLY user_sessions
    ADD CONSTRAINT user_sessions_pkey PRIMARY KEY (token);
ALTER TABLE ONLY users
    ADD CONSTRAINT users_pkey PRIMARY KEY (id);
ALTER TABLE ONLY users
    ADD CONSTRAINT users_username_key UNIQUE (username);
CREATE INDEX idx_connect_tokens_user_id ON connect_tokens USING btree (user_id);
CREATE INDEX idx_participants_user ON dialog_participants USING btree (user_id);
CREATE INDEX idx_sessions_expires ON user_sessions USING btree (expires_at);
CREATE INDEX idx_sessions_token ON user_sessions USING btree (token);
CREATE INDEX idx_sessions_user_id ON user_sessions USING btree (user_id);
ALTER TABLE ONLY connect_tokens
    ADD CONSTRAINT connect_tokens_user_id_fkey FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE;
ALTER TABLE ONLY dialog_last_message
    ADD CONSTRAINT dialog_last_message_dialog_id_fkey FOREIGN KEY (dialog_id) REFERENCES dialogs(id) ON DELETE CASCADE;
ALTER TABLE ONLY dialog_last_message
    ADD CONSTRAINT dialog_last_message_last_msg_sender_id_fkey FOREIGN KEY (last_msg_sender_id) REFERENCES users(id);
ALTER TABLE ONLY dialog_participants
    ADD CONSTRAINT dialog_participants_dialog_id_fkey FOREIGN KEY (dialog_id) REFERENCES dialogs(id) ON DELETE CASCADE;
ALTER TABLE ONLY dialog_participants
    ADD CONSTRAINT dialog_participants_user_id_fkey FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE;
ALTER TABLE ONLY messages
    ADD CONSTRAINT messages_dialog_id_fkey FOREIGN KEY (dialog_id) REFERENCES dialogs(id) ON DELETE CASCADE;
ALTER TABLE ONLY messages
    ADD CONSTRAINT messages_sender_id_fkey FOREIGN KEY (sender_id) REFERENCES users(id);
ALTER TABLE ONLY user_sessions
    ADD CONSTRAINT user_sessions_user_id_fkey FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE;
