CREATE TABLE public.actions (id INT NOT NULL, action_type VARCHAR NOT NULL, action_option VARCHAR, target_type VARCHAR, target_id INT, user_type VARCHAR, user_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.ar_internal_metadata (key VARCHAR NOT NULL, value VARCHAR, created_at TIMESTAMP(6) NOT NULL, updated_at TIMESTAMP(6) NOT NULL);

CREATE TABLE public.authorizations (id INT NOT NULL, provider VARCHAR NOT NULL, uid VARCHAR(1000) NOT NULL, user_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.commentable_pages (id BIGINT NOT NULL, name VARCHAR, user_id INT, comments_count INT DEFAULT 0 NOT NULL, created_at TIMESTAMP(6) NOT NULL, updated_at TIMESTAMP(6) NOT NULL);

CREATE TABLE public.comments (id INT NOT NULL, body TEXT NOT NULL, user_id INT NOT NULL, commentable_type VARCHAR, commentable_id INT, deleted_at TIMESTAMP, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.devices (id INT NOT NULL, platform INT NOT NULL, user_id INT NOT NULL, token VARCHAR NOT NULL, last_actived_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.exception_tracks (id INT NOT NULL, title VARCHAR, body TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.locations (id INT NOT NULL, name VARCHAR NOT NULL, users_count INT DEFAULT 0 NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.monkeys (id BIGINT NOT NULL, name VARCHAR, user_id INT, comments_count INT, created_at TIMESTAMP(6) NOT NULL, updated_at TIMESTAMP(6) NOT NULL);

CREATE TABLE public.nodes (id INT NOT NULL, name VARCHAR NOT NULL, summary VARCHAR, section_id INT NOT NULL, sort INT DEFAULT 0 NOT NULL, topics_count INT DEFAULT 0 NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.notes (id INT NOT NULL, title VARCHAR NOT NULL, body TEXT NOT NULL, user_id INT NOT NULL, word_count INT DEFAULT 0 NOT NULL, changes_count INT DEFAULT 0 NOT NULL, publish BOOLEAN DEFAULT FALSE, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.notifications (id INT NOT NULL, user_id INT NOT NULL, actor_id INT, notify_type VARCHAR NOT NULL, target_type VARCHAR, target_id INT, second_target_type VARCHAR, second_target_id INT, third_target_type VARCHAR, third_target_id INT, read_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.oauth_access_grants (id INT NOT NULL, resource_owner_id INT NOT NULL, application_id INT NOT NULL, token VARCHAR NOT NULL, expires_in BIGINT, redirect_uri TEXT NOT NULL, created_at TIMESTAMP NOT NULL, revoked_at TIMESTAMP, scopes VARCHAR);

CREATE TABLE public.oauth_access_tokens (id INT NOT NULL, resource_owner_id INT, application_id INT, token VARCHAR NOT NULL, refresh_token VARCHAR, expires_in BIGINT, revoked_at TIMESTAMP, created_at TIMESTAMP NOT NULL, scopes VARCHAR);

CREATE TABLE public.oauth_applications (id INT NOT NULL, name VARCHAR NOT NULL, uid VARCHAR NOT NULL, secret VARCHAR NOT NULL, redirect_uri TEXT NOT NULL, scopes VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, owner_id INT, owner_type VARCHAR, level INT DEFAULT 0 NOT NULL, confidential BOOLEAN DEFAULT TRUE NOT NULL);

CREATE TABLE public.page_versions (id INT NOT NULL, user_id INT NOT NULL, page_id INT NOT NULL, version INT DEFAULT 0 NOT NULL, slug VARCHAR NOT NULL, title VARCHAR NOT NULL, "desc" TEXT NOT NULL, body TEXT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.photos (id INT NOT NULL, user_id INT, image VARCHAR NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.replies (id INT NOT NULL, user_id INT NOT NULL, topic_id INT NOT NULL, body TEXT NOT NULL, state INT DEFAULT 1 NOT NULL, likes_count INT DEFAULT 0, mentioned_user_ids INT[] DEFAULT CAST('{}' AS INT[]), deleted_at TIMESTAMP, created_at TIMESTAMP, updated_at TIMESTAMP, action VARCHAR, target_type VARCHAR, target_id VARCHAR, reply_to_id INT);

CREATE TABLE public.schema_migrations (version VARCHAR NOT NULL);

CREATE TABLE public.sections (id INT NOT NULL, name VARCHAR NOT NULL, sort INT DEFAULT 0 NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.settings (id INT NOT NULL, var VARCHAR NOT NULL, value TEXT, thing_id INT, thing_type VARCHAR(30), created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.team_users (id INT NOT NULL, team_id INT NOT NULL, user_id INT NOT NULL, role INT, status INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.test_documents (id BIGINT NOT NULL, user_id INT, reply_to_id INT, mentioned_user_ids INT[] DEFAULT CAST('{}' AS INT[]), body TEXT, created_at TIMESTAMP(6) NOT NULL, updated_at TIMESTAMP(6) NOT NULL);

CREATE TABLE public.topics (id INT NOT NULL, user_id INT NOT NULL, node_id INT NOT NULL, title VARCHAR NOT NULL, body TEXT NOT NULL, last_reply_id INT, last_reply_user_id INT, last_reply_user_login VARCHAR, node_name VARCHAR, who_deleted VARCHAR, last_active_mark INT, lock_node BOOLEAN DEFAULT FALSE, suggested_at TIMESTAMP, grade INT DEFAULT 0, replied_at TIMESTAMP, replies_count INT DEFAULT 0 NOT NULL, likes_count INT DEFAULT 0, mentioned_user_ids INT[] DEFAULT CAST('{}' AS INT[]), deleted_at TIMESTAMP, created_at TIMESTAMP, updated_at TIMESTAMP, closed_at TIMESTAMP, team_id INT);

CREATE TABLE public.user_ssos (id INT NOT NULL, user_id INT NOT NULL, uid VARCHAR NOT NULL, username VARCHAR, email VARCHAR, name VARCHAR, avatar_url VARCHAR, last_payload TEXT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.users (id INT NOT NULL, login VARCHAR(100) NOT NULL, name VARCHAR(100), email VARCHAR NOT NULL, email_md5 VARCHAR NOT NULL, email_public BOOLEAN DEFAULT FALSE NOT NULL, location VARCHAR, location_id INT, bio VARCHAR, website VARCHAR, company VARCHAR, github VARCHAR, twitter VARCHAR, avatar VARCHAR, state INT DEFAULT 1 NOT NULL, tagline VARCHAR, created_at TIMESTAMP, updated_at TIMESTAMP, encrypted_password VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, reset_password_token VARCHAR, reset_password_sent_at TIMESTAMP, remember_created_at TIMESTAMP, sign_in_count INT DEFAULT 0 NOT NULL, current_sign_in_at TIMESTAMP, last_sign_in_at TIMESTAMP, current_sign_in_ip VARCHAR, last_sign_in_ip VARCHAR, password_salt VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, persistence_token VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, single_access_token VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, perishable_token VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, topics_count INT DEFAULT 0 NOT NULL, replies_count INT DEFAULT 0 NOT NULL, follower_ids INT[] DEFAULT CAST('{}' AS INT[]), type VARCHAR(20), failed_attempts INT DEFAULT 0 NOT NULL, unlock_token VARCHAR, locked_at TIMESTAMP, team_users_count INT, followers_count INT DEFAULT 0, following_count INT DEFAULT 0);

CREATE TABLE public.walking_deads (id BIGINT NOT NULL, name VARCHAR, tag VARCHAR, deleted_at TIMESTAMP, created_at TIMESTAMP(6) NOT NULL, updated_at TIMESTAMP(6) NOT NULL);

ALTER TABLE ONLY public.actions ADD CONSTRAINT actions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ar_internal_metadata ADD CONSTRAINT ar_internal_metadata_pkey PRIMARY KEY (key);

ALTER TABLE ONLY public.authorizations ADD CONSTRAINT authorizations_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.commentable_pages ADD CONSTRAINT commentable_pages_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.comments ADD CONSTRAINT comments_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.devices ADD CONSTRAINT devices_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.exception_tracks ADD CONSTRAINT exception_tracks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.locations ADD CONSTRAINT locations_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.monkeys ADD CONSTRAINT monkeys_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.nodes ADD CONSTRAINT nodes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.notes ADD CONSTRAINT notes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.notifications ADD CONSTRAINT notifications_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.oauth_access_grants ADD CONSTRAINT oauth_access_grants_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.oauth_access_tokens ADD CONSTRAINT oauth_access_tokens_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.oauth_applications ADD CONSTRAINT oauth_applications_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.page_versions ADD CONSTRAINT page_versions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.photos ADD CONSTRAINT photos_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.replies ADD CONSTRAINT replies_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.schema_migrations ADD CONSTRAINT schema_migrations_pkey PRIMARY KEY (version);

ALTER TABLE ONLY public.sections ADD CONSTRAINT sections_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.settings ADD CONSTRAINT settings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.team_users ADD CONSTRAINT team_users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.test_documents ADD CONSTRAINT test_documents_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topics ADD CONSTRAINT topics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_ssos ADD CONSTRAINT user_ssos_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.users ADD CONSTRAINT users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.walking_deads ADD CONSTRAINT walking_deads_pkey PRIMARY KEY (id);

-- WeTune schema patches
ALTER TABLE "oauth_applications" ALTER COLUMN "owner_id" SET NOT NULL;
ALTER TABLE "oauth_applications" ALTER COLUMN "owner_type" SET NOT NULL;
ALTER TABLE "oauth_access_tokens" ALTER COLUMN "refresh_token" SET NOT NULL;
ALTER TABLE "oauth_access_tokens" ALTER COLUMN "resource_owner_id" SET NOT NULL;
ALTER TABLE "photos" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "settings" ALTER COLUMN "thing_type" SET NOT NULL;
ALTER TABLE "settings" ALTER COLUMN "thing_id" SET NOT NULL;
ALTER TABLE "comments" ALTER COLUMN "commentable_id" SET NOT NULL;
ALTER TABLE "comments" ALTER COLUMN "commentable_type" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "grade" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "last_active_mark" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "last_reply_id" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "likes_count" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "suggested_at" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "team_id" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "location" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "unlock_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "replies" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "actions" ALTER COLUMN "user_type" SET NOT NULL;
ALTER TABLE "actions" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "actions" ALTER COLUMN "target_type" SET NOT NULL;
ALTER TABLE "actions" ALTER COLUMN "target_id" SET NOT NULL;
ALTER TABLE "users" ADD CONSTRAINT "wetune_u_4b0a93cc535c91a2" UNIQUE ("login");
ALTER TABLE "users" ADD CONSTRAINT "wetune_u_33ad5f660a4b9411" UNIQUE ("name");
ALTER TABLE "actions" ADD CONSTRAINT "wetune_u_cb2bfb73b45770a4" UNIQUE ("action_type", "user_type", "user_id", "target_type", "target_id");
ALTER TABLE "actions" ADD CONSTRAINT "wetune_fk_9a8608fa0be4f43c" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "actions" ADD CONSTRAINT "wetune_fk_216339e9eee2fb6a" FOREIGN KEY ("target_id") REFERENCES "users" ("id");
ALTER TABLE "team_users" ADD CONSTRAINT "wetune_fk_6d603375043aa7d7" FOREIGN KEY ("team_id") REFERENCES "users" ("id");
ALTER TABLE "topics" ADD CONSTRAINT "wetune_fk_e246437e4e4b3b7a" FOREIGN KEY ("last_reply_user_id") REFERENCES "users" ("id");
ALTER TABLE "team_users" ADD CONSTRAINT "wetune_fk_faf0f14d45960566" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "topics" ADD CONSTRAINT "wetune_fk_a17ac05086272c3e" FOREIGN KEY ("node_id") REFERENCES "nodes" ("id");
ALTER TABLE "topics" ADD CONSTRAINT "wetune_fk_7317f3462acd73dd" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "actions" ADD CONSTRAINT "wetune_fk_d741dda8ed3a71f9" FOREIGN KEY ("target_id") REFERENCES "nodes" ("id");
