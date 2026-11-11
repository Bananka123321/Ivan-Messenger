-- Метаданные файлов, передаваемых в диалогах.
-- Применить к существующей базе:
--   psql -U <user> -d <db> -h 127.0.0.1 -f ServerPart/sql/migrations/001_files.sql

CREATE TABLE IF NOT EXISTS files (
    id bigserial PRIMARY KEY,
    owner_id integer NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    dialog_id bigint NOT NULL REFERENCES dialogs(id) ON DELETE CASCADE,
    original_name character varying(255) NOT NULL,
    mime_type character varying(127) NOT NULL,
    kind smallint NOT NULL,
    size_bytes bigint NOT NULL CHECK (size_bytes > 0),
    sha256 character(64) NOT NULL,
    storage_name character varying(64) NOT NULL UNIQUE,
    uploaded_bytes bigint NOT NULL DEFAULT 0 CHECK (uploaded_bytes >= 0 AND uploaded_bytes <= size_bytes),
    status character varying(16) NOT NULL DEFAULT 'uploading' CHECK (status IN ('uploading', 'complete', 'failed')),
    created_at timestamp with time zone NOT NULL DEFAULT now(),
    completed_at timestamp with time zone
);

CREATE INDEX IF NOT EXISTS idx_files_dialog ON files USING btree (dialog_id);
CREATE INDEX IF NOT EXISTS idx_files_owner_resume ON files USING btree (owner_id, dialog_id, sha256) WHERE status = 'uploading';
