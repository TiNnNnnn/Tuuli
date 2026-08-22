CREATE TABLE "ar_internal_metadata" (
  "key" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255),
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("key")
);

CREATE TABLE "comments" (
  "id" NUMERIC(20) NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP,
  "short_id" VARCHAR(10) NOT NULL,
  "story_id" NUMERIC(20) NOT NULL,
  "user_id" NUMERIC(20) NOT NULL,
  "parent_comment_id" NUMERIC(20),
  "thread_id" NUMERIC(20),
  "comment" TEXT NOT NULL,
  "upvotes" INTEGER NOT NULL,
  "downvotes" INTEGER NOT NULL,
  "confidence" DECIMAL(20, 19) NOT NULL,
  "markeddown_comment" TEXT,
  "is_deleted" SMALLINT,
  "is_moderated" SMALLINT,
  "is_from_email" SMALLINT,
  "hat_id" NUMERIC(20),
  PRIMARY KEY ("id"),
  UNIQUE ("short_id")
);

CREATE TABLE "domains" (
  "id" BIGINT NOT NULL,
  "domain" VARCHAR(255),
  "is_tracker" SMALLINT NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "banned_at" TIMESTAMP,
  "banned_by_user_id" INTEGER,
  "banned_reason" VARCHAR(200),
  PRIMARY KEY ("id")
);

CREATE TABLE "hat_requests" (
  "id" NUMERIC(20) NOT NULL,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "user_id" NUMERIC(20) NOT NULL,
  "hat" VARCHAR(255) NOT NULL,
  "link" VARCHAR(255) NOT NULL,
  "comment" TEXT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "hats" (
  "id" NUMERIC(20) NOT NULL,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "user_id" NUMERIC(20) NOT NULL,
  "granted_by_user_id" NUMERIC(20) NOT NULL,
  "hat" VARCHAR(255) NOT NULL,
  "link" VARCHAR(255),
  "modlog_use" SMALLINT,
  "doffed_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "hidden_stories" (
  "id" NUMERIC(20) NOT NULL,
  "user_id" NUMERIC(20) NOT NULL,
  "story_id" NUMERIC(20) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "story_id")
);

CREATE TABLE "invitation_requests" (
  "id" NUMERIC(20) NOT NULL,
  "code" VARCHAR(255),
  "is_verified" SMALLINT,
  "email" VARCHAR(255) NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "memo" TEXT,
  "ip_address" VARCHAR(255),
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "invitations" (
  "id" NUMERIC(20) NOT NULL,
  "user_id" NUMERIC(20) NOT NULL,
  "email" VARCHAR(255),
  "code" VARCHAR(255),
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "memo" TEXT,
  "used_at" TIMESTAMP,
  "new_user_id" NUMERIC(20),
  PRIMARY KEY ("id")
);

CREATE TABLE "keystores" (
  "key" VARCHAR(50) NOT NULL,
  "value" BIGINT,
  UNIQUE ("key")
);

CREATE TABLE "messages" (
  "id" NUMERIC(20) NOT NULL,
  "created_at" TIMESTAMP,
  "author_user_id" NUMERIC(20) NOT NULL,
  "recipient_user_id" NUMERIC(20) NOT NULL,
  "has_been_read" SMALLINT,
  "subject" VARCHAR(100),
  "body" TEXT,
  "short_id" VARCHAR(30),
  "deleted_by_author" SMALLINT,
  "deleted_by_recipient" SMALLINT,
  "hat_id" NUMERIC(20),
  PRIMARY KEY ("id"),
  UNIQUE ("short_id")
);

CREATE TABLE "mod_notes" (
  "id" NUMERIC(20) NOT NULL,
  "moderator_user_id" NUMERIC(20) NOT NULL,
  "user_id" NUMERIC(20) NOT NULL,
  "note" TEXT NOT NULL,
  "markeddown_note" TEXT NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "moderations" (
  "id" NUMERIC(20) NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "moderator_user_id" NUMERIC(20),
  "story_id" NUMERIC(20),
  "comment_id" NUMERIC(20),
  "user_id" NUMERIC(20),
  "action" TEXT,
  "reason" TEXT,
  "is_from_suggestions" SMALLINT,
  "tag_id" NUMERIC(20),
  "domain_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "read_ribbons" (
  "id" NUMERIC(20) NOT NULL,
  "is_following" SMALLINT,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "user_id" NUMERIC(20) NOT NULL,
  "story_id" NUMERIC(20) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "saved_stories" (
  "id" NUMERIC(20) NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "user_id" NUMERIC(20) NOT NULL,
  "story_id" NUMERIC(20) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "story_id")
);

CREATE TABLE "schema_migrations" (
  "version" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("version")
);

CREATE TABLE "stories" (
  "id" NUMERIC(20) NOT NULL,
  "created_at" TIMESTAMP,
  "user_id" NUMERIC(20) NOT NULL,
  "url" VARCHAR(250),
  "title" VARCHAR(150) NOT NULL,
  "description" TEXT,
  "short_id" VARCHAR(6) NOT NULL,
  "is_expired" SMALLINT NOT NULL,
  "upvotes" BIGINT NOT NULL,
  "downvotes" BIGINT NOT NULL,
  "is_moderated" SMALLINT NOT NULL,
  "hotness" DECIMAL(20, 10) NOT NULL,
  "markeddown_description" TEXT,
  "story_cache" TEXT,
  "comments_count" INTEGER NOT NULL,
  "merged_story_id" NUMERIC(20),
  "unavailable_at" TIMESTAMP,
  "twitter_id" VARCHAR(20),
  "user_is_author" SMALLINT,
  "user_is_following" SMALLINT NOT NULL,
  "domain_id" BIGINT,
  PRIMARY KEY ("id"),
  UNIQUE ("short_id")
);

CREATE TABLE "suggested_taggings" (
  "id" NUMERIC(20) NOT NULL,
  "story_id" NUMERIC(20) NOT NULL,
  "tag_id" NUMERIC(20) NOT NULL,
  "user_id" NUMERIC(20) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "suggested_titles" (
  "id" NUMERIC(20) NOT NULL,
  "story_id" NUMERIC(20) NOT NULL,
  "user_id" NUMERIC(20) NOT NULL,
  "title" VARCHAR(150) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "tag_filters" (
  "id" NUMERIC(20) NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "user_id" NUMERIC(20) NOT NULL,
  "tag_id" NUMERIC(20) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "taggings" (
  "id" NUMERIC(20) NOT NULL,
  "story_id" NUMERIC(20) NOT NULL,
  "tag_id" NUMERIC(20) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("story_id", "tag_id")
);

CREATE TABLE "tags" (
  "id" NUMERIC(20) NOT NULL,
  "tag" VARCHAR(25) NOT NULL,
  "description" VARCHAR(100),
  "privileged" SMALLINT,
  "is_media" SMALLINT,
  "inactive" SMALLINT,
  "hotness_mod" REAL,
  "permit_by_new_users" SMALLINT NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("tag")
);

CREATE TABLE "users" (
  "id" NUMERIC(20) NOT NULL,
  "username" VARCHAR(50),
  "email" VARCHAR(100),
  "password_digest" VARCHAR(75),
  "created_at" TIMESTAMP,
  "is_admin" SMALLINT,
  "password_reset_token" VARCHAR(75),
  "session_token" VARCHAR(75) NOT NULL,
  "about" TEXT,
  "invited_by_user_id" NUMERIC(20),
  "is_moderator" SMALLINT,
  "pushover_mentions" SMALLINT,
  "rss_token" VARCHAR(75),
  "mailing_list_token" VARCHAR(75),
  "mailing_list_mode" INTEGER,
  "karma" INTEGER NOT NULL,
  "banned_at" TIMESTAMP,
  "banned_by_user_id" NUMERIC(20),
  "banned_reason" VARCHAR(200),
  "deleted_at" TIMESTAMP,
  "disabled_invite_at" TIMESTAMP,
  "disabled_invite_by_user_id" NUMERIC(20),
  "disabled_invite_reason" VARCHAR(200),
  "settings" TEXT,
  PRIMARY KEY ("id"),
  UNIQUE ("session_token"),
  UNIQUE ("mailing_list_token"),
  UNIQUE ("password_reset_token"),
  UNIQUE ("rss_token"),
  UNIQUE ("username")
);

ALTER TABLE "comments" ADD FOREIGN KEY ("hat_id") REFERENCES "hats" ("id");

ALTER TABLE "comments" ADD FOREIGN KEY ("parent_comment_id") REFERENCES "comments" ("id");

ALTER TABLE "comments" ADD FOREIGN KEY ("story_id") REFERENCES "stories" ("id");

ALTER TABLE "comments" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "hat_requests" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "hats" ADD FOREIGN KEY ("granted_by_user_id") REFERENCES "users" ("id");

ALTER TABLE "hats" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "hidden_stories" ADD FOREIGN KEY ("story_id") REFERENCES "stories" ("id");

ALTER TABLE "hidden_stories" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "invitations" ADD FOREIGN KEY ("new_user_id") REFERENCES "users" ("id");

ALTER TABLE "invitations" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "messages" ADD FOREIGN KEY ("hat_id") REFERENCES "hats" ("id");

ALTER TABLE "messages" ADD FOREIGN KEY ("author_user_id") REFERENCES "users" ("id");

ALTER TABLE "messages" ADD FOREIGN KEY ("recipient_user_id") REFERENCES "users" ("id");

ALTER TABLE "mod_notes" ADD FOREIGN KEY ("moderator_user_id") REFERENCES "users" ("id");

ALTER TABLE "mod_notes" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "moderations" ADD FOREIGN KEY ("comment_id") REFERENCES "comments" ("id");

ALTER TABLE "moderations" ADD FOREIGN KEY ("moderator_user_id") REFERENCES "users" ("id");

ALTER TABLE "moderations" ADD FOREIGN KEY ("story_id") REFERENCES "stories" ("id");

ALTER TABLE "moderations" ADD FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");

ALTER TABLE "read_ribbons" ADD FOREIGN KEY ("story_id") REFERENCES "stories" ("id");

ALTER TABLE "read_ribbons" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "saved_stories" ADD FOREIGN KEY ("story_id") REFERENCES "stories" ("id");

ALTER TABLE "saved_stories" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "stories" ADD FOREIGN KEY ("domain_id") REFERENCES "domains" ("id");

ALTER TABLE "stories" ADD FOREIGN KEY ("merged_story_id") REFERENCES "stories" ("id");

ALTER TABLE "stories" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "suggested_taggings" ADD FOREIGN KEY ("story_id") REFERENCES "stories" ("id");

ALTER TABLE "suggested_taggings" ADD FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");

ALTER TABLE "suggested_taggings" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "suggested_titles" ADD FOREIGN KEY ("story_id") REFERENCES "stories" ("id");

ALTER TABLE "suggested_titles" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "tag_filters" ADD FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");

ALTER TABLE "tag_filters" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "taggings" ADD FOREIGN KEY ("story_id") REFERENCES "stories" ("id");

ALTER TABLE "taggings" ADD FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");

ALTER TABLE "users" ADD FOREIGN KEY ("banned_by_user_id") REFERENCES "users" ("id");

ALTER TABLE "users" ADD FOREIGN KEY ("disabled_invite_by_user_id") REFERENCES "users" ("id");

ALTER TABLE "users" ADD FOREIGN KEY ("invited_by_user_id") REFERENCES "users" ("id");

-- WeTune schema patches
ALTER TABLE "comments" ALTER COLUMN "thread_id" SET NOT NULL;
ALTER TABLE "comments" ALTER COLUMN "hat_id" SET NOT NULL;
ALTER TABLE "comments" ALTER COLUMN "parent_comment_id" SET NOT NULL;
ALTER TABLE "stories" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "stories" ALTER COLUMN "twitter_id" SET NOT NULL;
ALTER TABLE "stories" ALTER COLUMN "url" SET NOT NULL;
ALTER TABLE "stories" ALTER COLUMN "description" SET NOT NULL;
ALTER TABLE "stories" ALTER COLUMN "story_cache" SET NOT NULL;
ALTER TABLE "stories" ALTER COLUMN "domain_id" SET NOT NULL;
ALTER TABLE "stories" ALTER COLUMN "merged_story_id" SET NOT NULL;
ALTER TABLE "moderations" ALTER COLUMN "domain_id" SET NOT NULL;
ALTER TABLE "moderations" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "moderations" ALTER COLUMN "comment_id" SET NOT NULL;
ALTER TABLE "moderations" ALTER COLUMN "moderator_user_id" SET NOT NULL;
ALTER TABLE "moderations" ALTER COLUMN "story_id" SET NOT NULL;
ALTER TABLE "moderations" ALTER COLUMN "tag_id" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "mailing_list_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "password_reset_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "rss_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "username" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "mailing_list_mode" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "banned_by_user_id" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "disabled_invite_by_user_id" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "invited_by_user_id" SET NOT NULL;
ALTER TABLE "invitations" ALTER COLUMN "new_user_id" SET NOT NULL;
ALTER TABLE "messages" ALTER COLUMN "short_id" SET NOT NULL;
ALTER TABLE "messages" ALTER COLUMN "hat_id" SET NOT NULL;
