CREATE TABLE "account_deletions" (
  "id" INTEGER NOT NULL,
  "person_id" INTEGER,
  "completed_at" TIMESTAMP,
  PRIMARY KEY ("id"),
  UNIQUE ("person_id")
);

CREATE TABLE "account_migrations" (
  "id" BIGINT NOT NULL,
  "old_person_id" INTEGER NOT NULL,
  "new_person_id" INTEGER NOT NULL,
  "completed_at" TIMESTAMP,
  PRIMARY KEY ("id"),
  UNIQUE ("old_person_id", "new_person_id"),
  UNIQUE ("old_person_id")
);

CREATE TABLE "ar_internal_metadata" (
  "key" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255),
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("key")
);

CREATE TABLE "aspect_memberships" (
  "id" INTEGER NOT NULL,
  "aspect_id" INTEGER NOT NULL,
  "contact_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("aspect_id", "contact_id")
);

CREATE TABLE "aspect_visibilities" (
  "id" INTEGER NOT NULL,
  "shareable_id" INTEGER NOT NULL,
  "aspect_id" INTEGER NOT NULL,
  "shareable_type" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("shareable_id", "shareable_type", "aspect_id")
);

CREATE TABLE "aspects" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "user_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "order_id" INTEGER,
  "post_default" SMALLINT,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "name")
);

CREATE TABLE "authorizations" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "o_auth_application_id" INTEGER,
  "refresh_token" VARCHAR(255),
  "code" VARCHAR(255),
  "redirect_uri" VARCHAR(255),
  "nonce" VARCHAR(255),
  "scopes" TEXT,
  "code_used" SMALLINT,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "blocks" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "person_id" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "person_id")
);

CREATE TABLE "comment_signatures" (
  "comment_id" INTEGER NOT NULL,
  "author_signature" TEXT NOT NULL,
  "signature_order_id" INTEGER NOT NULL,
  "additional_data" TEXT,
  UNIQUE ("comment_id")
);

CREATE TABLE "comments" (
  "id" INTEGER NOT NULL,
  "text" TEXT NOT NULL,
  "commentable_id" INTEGER NOT NULL,
  "author_id" INTEGER NOT NULL,
  "guid" VARCHAR(255) NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "likes_count" INTEGER NOT NULL,
  "commentable_type" VARCHAR(60) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("guid")
);

CREATE TABLE "contacts" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "person_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "sharing" SMALLINT NOT NULL,
  "receiving" SMALLINT NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "person_id")
);

CREATE TABLE "conversation_visibilities" (
  "id" INTEGER NOT NULL,
  "conversation_id" INTEGER NOT NULL,
  "person_id" INTEGER NOT NULL,
  "unread" INTEGER NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("conversation_id", "person_id")
);

CREATE TABLE "conversations" (
  "id" INTEGER NOT NULL,
  "subject" VARCHAR(255),
  "guid" VARCHAR(255) NOT NULL,
  "author_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("guid")
);

CREATE TABLE "invitation_codes" (
  "id" INTEGER NOT NULL,
  "token" VARCHAR(255),
  "user_id" INTEGER,
  "count" INTEGER,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "like_signatures" (
  "like_id" INTEGER NOT NULL,
  "author_signature" TEXT NOT NULL,
  "signature_order_id" INTEGER NOT NULL,
  "additional_data" TEXT,
  UNIQUE ("like_id")
);

CREATE TABLE "likes" (
  "id" INTEGER NOT NULL,
  "positive" SMALLINT,
  "target_id" INTEGER,
  "author_id" INTEGER,
  "guid" VARCHAR(255),
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "target_type" VARCHAR(60) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("target_id", "author_id", "target_type"),
  UNIQUE ("guid")
);

CREATE TABLE "locations" (
  "id" INTEGER NOT NULL,
  "address" VARCHAR(255),
  "lat" VARCHAR(255),
  "lng" VARCHAR(255),
  "status_message_id" INTEGER,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "mentions" (
  "id" INTEGER NOT NULL,
  "mentions_container_id" INTEGER NOT NULL,
  "person_id" INTEGER NOT NULL,
  "mentions_container_type" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("person_id", "mentions_container_id", "mentions_container_type")
);

CREATE TABLE "messages" (
  "id" INTEGER NOT NULL,
  "conversation_id" INTEGER NOT NULL,
  "author_id" INTEGER NOT NULL,
  "guid" VARCHAR(255) NOT NULL,
  "text" TEXT NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("guid")
);

CREATE TABLE "notification_actors" (
  "id" INTEGER NOT NULL,
  "notification_id" INTEGER,
  "person_id" INTEGER,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("notification_id", "person_id")
);

CREATE TABLE "notifications" (
  "id" INTEGER NOT NULL,
  "target_type" VARCHAR(255),
  "target_id" INTEGER,
  "recipient_id" INTEGER NOT NULL,
  "unread" SMALLINT NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "type" VARCHAR(255),
  "guid" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("guid")
);

CREATE TABLE "o_auth_access_tokens" (
  "id" INTEGER NOT NULL,
  "authorization_id" INTEGER,
  "token" VARCHAR(255),
  "expires_at" TIMESTAMP,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("token")
);

CREATE TABLE "o_auth_applications" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "client_id" VARCHAR(255),
  "client_secret" VARCHAR(255),
  "client_name" VARCHAR(255),
  "redirect_uris" TEXT,
  "response_types" VARCHAR(255),
  "grant_types" VARCHAR(255),
  "application_type" VARCHAR(255),
  "contacts" VARCHAR(255),
  "logo_uri" VARCHAR(255),
  "client_uri" VARCHAR(255),
  "policy_uri" VARCHAR(255),
  "tos_uri" VARCHAR(255),
  "sector_identifier_uri" VARCHAR(255),
  "token_endpoint_auth_method" VARCHAR(255),
  "jwks" TEXT,
  "jwks_uri" VARCHAR(255),
  "ppid" SMALLINT,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("client_id")
);

CREATE TABLE "o_embed_caches" (
  "id" INTEGER NOT NULL,
  "url" VARCHAR(1024) NOT NULL,
  "data" TEXT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "open_graph_caches" (
  "id" INTEGER NOT NULL,
  "title" VARCHAR(255),
  "ob_type" VARCHAR(255),
  "image" TEXT,
  "url" TEXT,
  "description" TEXT,
  "video_url" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "participations" (
  "id" INTEGER NOT NULL,
  "guid" VARCHAR(255),
  "target_id" INTEGER,
  "target_type" VARCHAR(60) NOT NULL,
  "author_id" INTEGER,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "count" INTEGER NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("target_id", "target_type", "author_id")
);

CREATE TABLE "people" (
  "id" INTEGER NOT NULL,
  "guid" VARCHAR(255) NOT NULL,
  "diaspora_handle" VARCHAR(255) NOT NULL,
  "serialized_public_key" TEXT NOT NULL,
  "owner_id" INTEGER,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "closed_account" SMALLINT,
  "fetch_status" INTEGER,
  "pod_id" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("diaspora_handle"),
  UNIQUE ("guid"),
  UNIQUE ("owner_id")
);

CREATE TABLE "photos" (
  "id" INTEGER NOT NULL,
  "author_id" INTEGER NOT NULL,
  "public" SMALLINT NOT NULL,
  "guid" VARCHAR(255) NOT NULL,
  "pending" SMALLINT NOT NULL,
  "text" TEXT,
  "remote_photo_path" TEXT,
  "remote_photo_name" VARCHAR(255),
  "random_string" VARCHAR(255),
  "processed_image" VARCHAR(255),
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "unprocessed_image" VARCHAR(255),
  "status_message_guid" VARCHAR(255),
  "height" INTEGER,
  "width" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("guid")
);

CREATE TABLE "pods" (
  "id" INTEGER NOT NULL,
  "host" VARCHAR(255) NOT NULL,
  "ssl" SMALLINT,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "status" INTEGER,
  "checked_at" TIMESTAMP,
  "offline_since" TIMESTAMP,
  "response_time" INTEGER,
  "software" VARCHAR(255),
  "error" VARCHAR(255),
  "port" INTEGER,
  "blocked" SMALLINT,
  "scheduled_check" SMALLINT NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("host", "port")
);

CREATE TABLE "poll_answers" (
  "id" INTEGER NOT NULL,
  "answer" VARCHAR(255) NOT NULL,
  "poll_id" INTEGER NOT NULL,
  "guid" VARCHAR(255),
  "vote_count" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("guid")
);

CREATE TABLE "poll_participation_signatures" (
  "poll_participation_id" INTEGER NOT NULL,
  "author_signature" TEXT NOT NULL,
  "signature_order_id" INTEGER NOT NULL,
  "additional_data" TEXT,
  UNIQUE ("poll_participation_id")
);

CREATE TABLE "poll_participations" (
  "id" INTEGER NOT NULL,
  "poll_answer_id" INTEGER NOT NULL,
  "author_id" INTEGER NOT NULL,
  "poll_id" INTEGER NOT NULL,
  "guid" VARCHAR(255),
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id"),
  UNIQUE ("poll_id", "author_id"),
  UNIQUE ("guid")
);

CREATE TABLE "polls" (
  "id" INTEGER NOT NULL,
  "question" VARCHAR(255) NOT NULL,
  "status_message_id" INTEGER NOT NULL,
  "status" SMALLINT,
  "guid" VARCHAR(255),
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id"),
  UNIQUE ("guid")
);

CREATE TABLE "posts" (
  "id" INTEGER NOT NULL,
  "author_id" INTEGER NOT NULL,
  "public" SMALLINT NOT NULL,
  "guid" VARCHAR(255) NOT NULL,
  "type" VARCHAR(40) NOT NULL,
  "text" TEXT,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "provider_display_name" VARCHAR(255),
  "root_guid" VARCHAR(255),
  "likes_count" INTEGER,
  "comments_count" INTEGER,
  "o_embed_cache_id" INTEGER,
  "reshares_count" INTEGER,
  "interacted_at" TIMESTAMP,
  "tweet_id" VARCHAR(255),
  "open_graph_cache_id" INTEGER,
  "tumblr_ids" TEXT,
  PRIMARY KEY ("id"),
  UNIQUE ("guid"),
  UNIQUE ("author_id", "root_guid")
);

CREATE TABLE "ppid" (
  "id" INTEGER NOT NULL,
  "o_auth_application_id" INTEGER,
  "user_id" INTEGER,
  "guid" VARCHAR(32),
  "identifier" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "profiles" (
  "id" INTEGER NOT NULL,
  "diaspora_handle" VARCHAR(255),
  "first_name" VARCHAR(127),
  "last_name" VARCHAR(127),
  "image_url" VARCHAR(255),
  "image_url_small" VARCHAR(255),
  "image_url_medium" VARCHAR(255),
  "birthday" DATE,
  "gender" VARCHAR(255),
  "bio" TEXT,
  "searchable" SMALLINT NOT NULL,
  "person_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "location" VARCHAR(255),
  "full_name" VARCHAR(70),
  "nsfw" SMALLINT,
  "public_details" SMALLINT,
  PRIMARY KEY ("id")
);

CREATE TABLE "references" (
  "id" BIGINT NOT NULL,
  "source_id" INTEGER NOT NULL,
  "source_type" VARCHAR(60) NOT NULL,
  "target_id" INTEGER NOT NULL,
  "target_type" VARCHAR(60) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("source_id", "source_type", "target_id", "target_type")
);

CREATE TABLE "reports" (
  "id" INTEGER NOT NULL,
  "item_id" INTEGER NOT NULL,
  "item_type" VARCHAR(255) NOT NULL,
  "reviewed" SMALLINT,
  "text" TEXT,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "user_id" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "roles" (
  "id" INTEGER NOT NULL,
  "person_id" INTEGER,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("person_id", "name")
);

CREATE TABLE "schema_migrations" (
  "version" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("version")
);

CREATE TABLE "services" (
  "id" INTEGER NOT NULL,
  "type" VARCHAR(127) NOT NULL,
  "user_id" INTEGER NOT NULL,
  "uid" VARCHAR(127),
  "access_token" VARCHAR(255),
  "access_secret" VARCHAR(255),
  "nickname" VARCHAR(255),
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "share_visibilities" (
  "id" INTEGER NOT NULL,
  "shareable_id" INTEGER NOT NULL,
  "hidden" SMALLINT NOT NULL,
  "shareable_type" VARCHAR(60) NOT NULL,
  "user_id" INTEGER NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("shareable_id", "shareable_type", "user_id")
);

CREATE TABLE "signature_orders" (
  "id" INTEGER NOT NULL,
  "order" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("order")
);

CREATE TABLE "simple_captcha_data" (
  "id" INTEGER NOT NULL,
  "key" VARCHAR(40),
  "value" VARCHAR(12),
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "tag_followings" (
  "id" INTEGER NOT NULL,
  "tag_id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("tag_id", "user_id")
);

CREATE TABLE "taggings" (
  "id" INTEGER NOT NULL,
  "tag_id" INTEGER,
  "taggable_id" INTEGER,
  "taggable_type" VARCHAR(127),
  "tagger_id" INTEGER,
  "tagger_type" VARCHAR(127),
  "context" VARCHAR(127),
  "created_at" TIMESTAMP,
  PRIMARY KEY ("id"),
  UNIQUE ("taggable_id", "taggable_type", "tag_id")
);

CREATE TABLE "tags" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "taggings_count" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("name")
);

CREATE TABLE "user_preferences" (
  "id" INTEGER NOT NULL,
  "email_type" VARCHAR(255),
  "user_id" INTEGER,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "users" (
  "id" INTEGER NOT NULL,
  "username" VARCHAR(255) NOT NULL,
  "serialized_private_key" TEXT,
  "getting_started" SMALLINT NOT NULL,
  "disable_mail" SMALLINT NOT NULL,
  "language" VARCHAR(255),
  "email" VARCHAR(255) NOT NULL,
  "encrypted_password" VARCHAR(255) NOT NULL,
  "reset_password_token" VARCHAR(255),
  "remember_created_at" TIMESTAMP,
  "sign_in_count" INTEGER,
  "current_sign_in_at" TIMESTAMP,
  "last_sign_in_at" TIMESTAMP,
  "current_sign_in_ip" VARCHAR(255),
  "last_sign_in_ip" VARCHAR(255),
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  "invited_by_id" INTEGER,
  "authentication_token" VARCHAR(30),
  "unconfirmed_email" VARCHAR(255),
  "confirm_email_token" VARCHAR(30),
  "locked_at" TIMESTAMP,
  "show_community_spotlight_in_stream" SMALLINT NOT NULL,
  "auto_follow_back" SMALLINT,
  "auto_follow_back_aspect_id" INTEGER,
  "hidden_shareables" TEXT,
  "reset_password_sent_at" TIMESTAMP,
  "last_seen" TIMESTAMP,
  "remove_after" TIMESTAMP,
  "export" VARCHAR(255),
  "exported_at" TIMESTAMP,
  "exporting" SMALLINT,
  "strip_exif" SMALLINT,
  "exported_photos_file" VARCHAR(255),
  "exported_photos_at" TIMESTAMP,
  "exporting_photos" SMALLINT,
  "color_theme" VARCHAR(255),
  "post_default_public" SMALLINT,
  "consumed_timestep" INTEGER,
  "otp_required_for_login" SMALLINT,
  "otp_backup_codes" TEXT,
  "plain_otp_secret" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("email"),
  UNIQUE ("username"),
  UNIQUE ("authentication_token")
);

ALTER TABLE "account_migrations" ADD FOREIGN KEY ("new_person_id") REFERENCES "people" ("id");

ALTER TABLE "account_migrations" ADD FOREIGN KEY ("old_person_id") REFERENCES "people" ("id");

ALTER TABLE "aspect_memberships" ADD FOREIGN KEY ("aspect_id") REFERENCES "aspects" ("id");

ALTER TABLE "aspect_memberships" ADD FOREIGN KEY ("contact_id") REFERENCES "contacts" ("id");

ALTER TABLE "aspect_visibilities" ADD FOREIGN KEY ("aspect_id") REFERENCES "aspects" ("id");

ALTER TABLE "authorizations" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "authorizations" ADD FOREIGN KEY ("o_auth_application_id") REFERENCES "o_auth_applications" ("id");

ALTER TABLE "comment_signatures" ADD FOREIGN KEY ("comment_id") REFERENCES "comments" ("id");

ALTER TABLE "comment_signatures" ADD FOREIGN KEY ("signature_order_id") REFERENCES "signature_orders" ("id");

ALTER TABLE "comments" ADD FOREIGN KEY ("author_id") REFERENCES "people" ("id");

ALTER TABLE "contacts" ADD FOREIGN KEY ("person_id") REFERENCES "people" ("id");

ALTER TABLE "conversation_visibilities" ADD FOREIGN KEY ("conversation_id") REFERENCES "conversations" ("id");

ALTER TABLE "conversation_visibilities" ADD FOREIGN KEY ("person_id") REFERENCES "people" ("id");

ALTER TABLE "conversations" ADD FOREIGN KEY ("author_id") REFERENCES "people" ("id");

ALTER TABLE "like_signatures" ADD FOREIGN KEY ("like_id") REFERENCES "likes" ("id");

ALTER TABLE "like_signatures" ADD FOREIGN KEY ("signature_order_id") REFERENCES "signature_orders" ("id");

ALTER TABLE "likes" ADD FOREIGN KEY ("author_id") REFERENCES "people" ("id");

ALTER TABLE "messages" ADD FOREIGN KEY ("author_id") REFERENCES "people" ("id");

ALTER TABLE "messages" ADD FOREIGN KEY ("conversation_id") REFERENCES "conversations" ("id");

ALTER TABLE "notification_actors" ADD FOREIGN KEY ("notification_id") REFERENCES "notifications" ("id");

ALTER TABLE "o_auth_access_tokens" ADD FOREIGN KEY ("authorization_id") REFERENCES "authorizations" ("id");

ALTER TABLE "o_auth_applications" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "people" ADD FOREIGN KEY ("pod_id") REFERENCES "pods" ("id");

ALTER TABLE "poll_participation_signatures" ADD FOREIGN KEY ("poll_participation_id") REFERENCES "poll_participations" ("id");

ALTER TABLE "poll_participation_signatures" ADD FOREIGN KEY ("signature_order_id") REFERENCES "signature_orders" ("id");

ALTER TABLE "posts" ADD FOREIGN KEY ("author_id") REFERENCES "people" ("id");

ALTER TABLE "ppid" ADD FOREIGN KEY ("o_auth_application_id") REFERENCES "o_auth_applications" ("id");

ALTER TABLE "ppid" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "profiles" ADD FOREIGN KEY ("person_id") REFERENCES "people" ("id");

ALTER TABLE "services" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

ALTER TABLE "share_visibilities" ADD FOREIGN KEY ("user_id") REFERENCES "users" ("id");

-- WeTune schema patches
ALTER TABLE "notification_actors" ALTER COLUMN "person_id" SET NOT NULL;
ALTER TABLE "notification_actors" ALTER COLUMN "notification_id" SET NOT NULL;
ALTER TABLE "simple_captcha_data" ALTER COLUMN "key" SET NOT NULL;
ALTER TABLE "o_auth_applications" ALTER COLUMN "client_id" SET NOT NULL;
ALTER TABLE "o_auth_applications" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "photos" ALTER COLUMN "status_message_guid" SET NOT NULL;
ALTER TABLE "posts" ALTER COLUMN "root_guid" SET NOT NULL;
ALTER TABLE "likes" ALTER COLUMN "guid" SET NOT NULL;
ALTER TABLE "likes" ALTER COLUMN "target_id" SET NOT NULL;
ALTER TABLE "likes" ALTER COLUMN "author_id" SET NOT NULL;
ALTER TABLE "user_preferences" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "user_preferences" ALTER COLUMN "email_type" SET NOT NULL;
ALTER TABLE "profiles" ALTER COLUMN "full_name" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "authentication_token" SET NOT NULL;
ALTER TABLE "ppid" ALTER COLUMN "o_auth_application_id" SET NOT NULL;
ALTER TABLE "ppid" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "tags" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "tag_id" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "taggable_id" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "taggable_type" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "context" SET NOT NULL;
ALTER TABLE "poll_answers" ALTER COLUMN "guid" SET NOT NULL;
ALTER TABLE "notifications" ALTER COLUMN "guid" SET NOT NULL;
ALTER TABLE "notifications" ALTER COLUMN "target_type" SET NOT NULL;
ALTER TABLE "notifications" ALTER COLUMN "target_id" SET NOT NULL;
ALTER TABLE "roles" ALTER COLUMN "person_id" SET NOT NULL;
ALTER TABLE "roles" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "authorizations" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "authorizations" ALTER COLUMN "o_auth_application_id" SET NOT NULL;
ALTER TABLE "poll_participations" ALTER COLUMN "guid" SET NOT NULL;
ALTER TABLE "polls" ALTER COLUMN "guid" SET NOT NULL;
ALTER TABLE "account_deletions" ALTER COLUMN "person_id" SET NOT NULL;
ALTER TABLE "participations" ALTER COLUMN "target_id" SET NOT NULL;
ALTER TABLE "participations" ALTER COLUMN "author_id" SET NOT NULL;
ALTER TABLE "participations" ALTER COLUMN "guid" SET NOT NULL;
ALTER TABLE "blocks" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "blocks" ALTER COLUMN "person_id" SET NOT NULL;
ALTER TABLE "services" ALTER COLUMN "uid" SET NOT NULL;
ALTER TABLE "people" ALTER COLUMN "owner_id" SET NOT NULL;
ALTER TABLE "people" ALTER COLUMN "pod_id" SET NOT NULL;
ALTER TABLE "locations" ALTER COLUMN "status_message_id" SET NOT NULL;
ALTER TABLE "pods" ALTER COLUMN "port" SET NOT NULL;
ALTER TABLE "pods" ALTER COLUMN "checked_at" SET NOT NULL;
ALTER TABLE "pods" ALTER COLUMN "offline_since" SET NOT NULL;
ALTER TABLE "pods" ALTER COLUMN "status" SET NOT NULL;
ALTER TABLE "o_auth_access_tokens" ALTER COLUMN "token" SET NOT NULL;
ALTER TABLE "o_auth_access_tokens" ALTER COLUMN "authorization_id" SET NOT NULL;
ALTER TABLE "notifications" ADD CONSTRAINT "wetune_u_e71e0fbf2aba97f3" UNIQUE ("target_id", "target_type");
ALTER TABLE "contacts" ADD CONSTRAINT "wetune_fk_227c965cc5cb4344" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "notification_actors" ADD CONSTRAINT "wetune_fk_6ce945cead8da5f6" FOREIGN KEY ("person_id") REFERENCES "people" ("id");
ALTER TABLE "likes" ADD CONSTRAINT "wetune_fk_86cc77de122e0379" FOREIGN KEY ("target_id") REFERENCES "posts" ("id");
ALTER TABLE "poll_participations" ADD CONSTRAINT "wetune_fk_d6e4e05d23d1ee9f" FOREIGN KEY ("poll_id") REFERENCES "polls" ("id");
ALTER TABLE "share_visibilities" ADD CONSTRAINT "wetune_fk_184bac2f77083ffe" FOREIGN KEY ("shareable_id") REFERENCES "photos" ("id");
ALTER TABLE "taggings" ADD CONSTRAINT "wetune_fk_55ab0b1c4e1ba784" FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");
ALTER TABLE "participations" ADD CONSTRAINT "wetune_fk_4bf951e9de092e94" FOREIGN KEY ("target_id") REFERENCES "posts" ("id");
ALTER TABLE "comments" ADD CONSTRAINT "wetune_fk_747dd7fafcaa8168" FOREIGN KEY ("commentable_id") REFERENCES "posts" ("id");
ALTER TABLE "mentions" ADD CONSTRAINT "wetune_fk_cec237a20cf34aa8" FOREIGN KEY ("person_id") REFERENCES "people" ("id");
ALTER TABLE "blocks" ADD CONSTRAINT "wetune_fk_b3413c3be817f319" FOREIGN KEY ("person_id") REFERENCES "people" ("id");
ALTER TABLE "polls" ADD CONSTRAINT "wetune_fk_699bc8c89e761036" FOREIGN KEY ("status_message_id") REFERENCES "posts" ("id");
ALTER TABLE "taggings" ADD CONSTRAINT "wetune_fk_2fcc9ad0259ad683" FOREIGN KEY ("taggable_id") REFERENCES "posts" ("id");
ALTER TABLE "roles" ADD CONSTRAINT "wetune_fk_587f30fffbf30923" FOREIGN KEY ("person_id") REFERENCES "people" ("id");
ALTER TABLE "participations" ADD CONSTRAINT "wetune_fk_39240ed068823348" FOREIGN KEY ("author_id") REFERENCES "people" ("id");
ALTER TABLE "people" ADD CONSTRAINT "wetune_fk_392d261f2bb0e349" FOREIGN KEY ("owner_id") REFERENCES "users" ("id");
ALTER TABLE "mentions" ADD CONSTRAINT "wetune_fk_f95dd47577e31bc6" FOREIGN KEY ("mentions_container_id") REFERENCES "posts" ("id");
ALTER TABLE "tag_followings" ADD CONSTRAINT "wetune_fk_fa4e168544314501" FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");
ALTER TABLE "aspect_visibilities" ADD CONSTRAINT "wetune_fk_0fb014a7cdf976ac" FOREIGN KEY ("shareable_id") REFERENCES "posts" ("id");
