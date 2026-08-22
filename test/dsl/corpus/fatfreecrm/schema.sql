CREATE TABLE "account_contacts" (
  "id" INTEGER NOT NULL,
  "account_id" INTEGER,
  "contact_id" INTEGER,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "account_opportunities" (
  "id" INTEGER NOT NULL,
  "account_id" INTEGER,
  "opportunity_id" INTEGER,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "accounts" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "assigned_to" INTEGER,
  "name" VARCHAR(64) NOT NULL,
  "access" VARCHAR(8),
  "website" VARCHAR(64),
  "toll_free_phone" VARCHAR(32),
  "phone" VARCHAR(32),
  "fax" VARCHAR(32),
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "email" VARCHAR(254),
  "background_info" VARCHAR(255),
  "rating" INTEGER NOT NULL,
  "category" VARCHAR(32),
  "subscribed_users" TEXT,
  "contacts_count" INTEGER,
  "opportunities_count" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "name", "deleted_at")
);

CREATE TABLE "activities" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "subject_type" VARCHAR(255),
  "subject_id" INTEGER,
  "action" VARCHAR(32),
  "info" VARCHAR(255),
  "private" SMALLINT,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "addresses" (
  "id" INTEGER NOT NULL,
  "street1" VARCHAR(255),
  "street2" VARCHAR(255),
  "city" VARCHAR(64),
  "state" VARCHAR(64),
  "zipcode" VARCHAR(16),
  "country" VARCHAR(64),
  "full_address" VARCHAR(255),
  "address_type" VARCHAR(16),
  "addressable_type" VARCHAR(255),
  "addressable_id" INTEGER,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "deleted_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "ar_internal_metadata" (
  "key" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255),
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("key")
);

CREATE TABLE "avatars" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "entity_type" VARCHAR(255),
  "entity_id" INTEGER,
  "image_file_size" INTEGER,
  "image_file_name" VARCHAR(255),
  "image_content_type" VARCHAR(255),
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "campaigns" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "assigned_to" INTEGER,
  "name" VARCHAR(64) NOT NULL,
  "access" VARCHAR(8),
  "status" VARCHAR(64),
  "budget" DECIMAL(12, 2),
  "target_leads" INTEGER,
  "target_conversion" REAL,
  "target_revenue" DECIMAL(12, 2),
  "leads_count" INTEGER,
  "opportunities_count" INTEGER,
  "revenue" DECIMAL(12, 2),
  "starts_on" DATE,
  "ends_on" DATE,
  "objectives" TEXT,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "background_info" VARCHAR(255),
  "subscribed_users" TEXT,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "name", "deleted_at")
);

CREATE TABLE "comments" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "commentable_type" VARCHAR(255),
  "commentable_id" INTEGER,
  "private" SMALLINT,
  "title" VARCHAR(255),
  "comment" TEXT,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "state" VARCHAR(16) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "contact_opportunities" (
  "id" INTEGER NOT NULL,
  "contact_id" INTEGER,
  "opportunity_id" INTEGER,
  "role" VARCHAR(32),
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "contacts" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "lead_id" INTEGER,
  "assigned_to" INTEGER,
  "reports_to" INTEGER,
  "first_name" VARCHAR(64) NOT NULL,
  "last_name" VARCHAR(64) NOT NULL,
  "access" VARCHAR(8),
  "title" VARCHAR(64),
  "department" VARCHAR(64),
  "source" VARCHAR(32),
  "email" VARCHAR(254),
  "alt_email" VARCHAR(254),
  "phone" VARCHAR(32),
  "mobile" VARCHAR(32),
  "fax" VARCHAR(32),
  "blog" VARCHAR(128),
  "linkedin" VARCHAR(128),
  "facebook" VARCHAR(128),
  "twitter" VARCHAR(128),
  "born_on" DATE,
  "do_not_call" SMALLINT NOT NULL,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "background_info" VARCHAR(255),
  "skype" VARCHAR(128),
  "subscribed_users" TEXT,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "last_name", "deleted_at")
);

CREATE TABLE "emails" (
  "id" INTEGER NOT NULL,
  "imap_message_id" VARCHAR(255) NOT NULL,
  "user_id" INTEGER,
  "mediator_type" VARCHAR(255),
  "mediator_id" INTEGER,
  "sent_from" VARCHAR(255) NOT NULL,
  "sent_to" VARCHAR(255) NOT NULL,
  "cc" VARCHAR(255),
  "bcc" VARCHAR(255),
  "subject" VARCHAR(255),
  "body" TEXT,
  "header" TEXT,
  "sent_at" TIMESTAMP,
  "received_at" TIMESTAMP,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "state" VARCHAR(16) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "field_groups" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(64),
  "label" VARCHAR(128),
  "position" INTEGER,
  "hint" VARCHAR(255),
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "tag_id" INTEGER,
  "klass_name" VARCHAR(32),
  PRIMARY KEY ("id")
);

CREATE TABLE "fields" (
  "id" INTEGER NOT NULL,
  "type" VARCHAR(255),
  "field_group_id" INTEGER,
  "position" INTEGER,
  "name" VARCHAR(64),
  "label" VARCHAR(128),
  "hint" VARCHAR(255),
  "placeholder" VARCHAR(255),
  "as" VARCHAR(32),
  "collection" TEXT,
  "disabled" SMALLINT,
  "required" SMALLINT,
  "maxlength" INTEGER,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "pair_id" INTEGER,
  "settings" TEXT,
  "minlength" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "groups" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "groups_users" (
  "group_id" INTEGER,
  "user_id" INTEGER
);

CREATE TABLE "leads" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "campaign_id" INTEGER,
  "assigned_to" INTEGER,
  "first_name" VARCHAR(64) NOT NULL,
  "last_name" VARCHAR(64) NOT NULL,
  "access" VARCHAR(8),
  "title" VARCHAR(64),
  "company" VARCHAR(64),
  "source" VARCHAR(32),
  "status" VARCHAR(32),
  "referred_by" VARCHAR(64),
  "email" VARCHAR(254),
  "alt_email" VARCHAR(254),
  "phone" VARCHAR(32),
  "mobile" VARCHAR(32),
  "blog" VARCHAR(128),
  "linkedin" VARCHAR(128),
  "facebook" VARCHAR(128),
  "twitter" VARCHAR(128),
  "rating" INTEGER NOT NULL,
  "do_not_call" SMALLINT NOT NULL,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "background_info" VARCHAR(255),
  "skype" VARCHAR(128),
  "subscribed_users" TEXT,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "last_name", "deleted_at")
);

CREATE TABLE "lists" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "url" TEXT,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "user_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "opportunities" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "campaign_id" INTEGER,
  "assigned_to" INTEGER,
  "name" VARCHAR(64) NOT NULL,
  "access" VARCHAR(8),
  "source" VARCHAR(32),
  "stage" VARCHAR(32),
  "probability" INTEGER,
  "amount" DECIMAL(12, 2),
  "discount" DECIMAL(12, 2),
  "closes_on" DATE,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "background_info" VARCHAR(255),
  "subscribed_users" TEXT,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "name", "deleted_at")
);

CREATE TABLE "permissions" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "asset_type" VARCHAR(255),
  "asset_id" INTEGER,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "group_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "preferences" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "name" VARCHAR(32) NOT NULL,
  "value" TEXT,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "schema_migrations" (
  "version" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("version")
);

CREATE TABLE "sessions" (
  "id" INTEGER NOT NULL,
  "session_id" VARCHAR(255) NOT NULL,
  "data" TEXT,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "settings" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(32) NOT NULL,
  "value" TEXT,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "taggings" (
  "id" INTEGER NOT NULL,
  "tag_id" INTEGER,
  "taggable_id" INTEGER,
  "tagger_id" INTEGER,
  "tagger_type" VARCHAR(255),
  "taggable_type" VARCHAR(50),
  "context" VARCHAR(50),
  "created_at" TIMESTAMP,
  PRIMARY KEY ("id"),
  UNIQUE ("tag_id", "taggable_id", "taggable_type", "context")
);

CREATE TABLE "tags" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "taggings_count" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("name")
);

CREATE TABLE "tasks" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "assigned_to" INTEGER,
  "completed_by" INTEGER,
  "name" VARCHAR(255) NOT NULL,
  "asset_type" VARCHAR(255),
  "asset_id" INTEGER,
  "priority" VARCHAR(32),
  "category" VARCHAR(32),
  "bucket" VARCHAR(32),
  "due_at" TIMESTAMP,
  "completed_at" TIMESTAMP,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "background_info" VARCHAR(255),
  "subscribed_users" TEXT,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "name", "deleted_at")
);

CREATE TABLE "users" (
  "id" INTEGER NOT NULL,
  "username" VARCHAR(32) NOT NULL,
  "email" VARCHAR(254) NOT NULL,
  "first_name" VARCHAR(32),
  "last_name" VARCHAR(32),
  "title" VARCHAR(64),
  "company" VARCHAR(64),
  "alt_email" VARCHAR(254),
  "phone" VARCHAR(32),
  "mobile" VARCHAR(32),
  "aim" VARCHAR(32),
  "yahoo" VARCHAR(32),
  "google" VARCHAR(32),
  "skype" VARCHAR(32),
  "encrypted_password" VARCHAR(255) NOT NULL,
  "password_salt" VARCHAR(255) NOT NULL,
  "last_sign_in_at" TIMESTAMP,
  "current_sign_in_at" TIMESTAMP,
  "last_sign_in_ip" VARCHAR(255),
  "current_sign_in_ip" VARCHAR(255),
  "sign_in_count" INTEGER NOT NULL,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "admin" SMALLINT NOT NULL,
  "suspended_at" TIMESTAMP,
  "unconfirmed_email" VARCHAR(254),
  "reset_password_token" VARCHAR(255),
  "reset_password_sent_at" TIMESTAMP,
  "remember_token" VARCHAR(255),
  "remember_created_at" TIMESTAMP,
  "authentication_token" VARCHAR(255),
  "confirmation_token" VARCHAR(255),
  "confirmed_at" TIMESTAMP NOT NULL,
  "confirmation_sent_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("username", "deleted_at"),
  UNIQUE ("reset_password_token"),
  UNIQUE ("remember_token"),
  UNIQUE ("confirmation_token"),
  UNIQUE ("authentication_token")
);

CREATE TABLE "versions" (
  "id" INTEGER NOT NULL,
  "item_type" VARCHAR(255) NOT NULL,
  "item_id" INTEGER NOT NULL,
  "event" VARCHAR(512) NOT NULL,
  "whodunnit" VARCHAR(255),
  "object" TEXT,
  "created_at" TIMESTAMP,
  "object_changes" TEXT,
  "related_id" INTEGER,
  "related_type" VARCHAR(255),
  "transaction_id" INTEGER,
  PRIMARY KEY ("id")
);

-- WeTune schema patches
ALTER TABLE "taggings" ALTER COLUMN "tag_id" SET NOT NULL;
ALTER TABLE "addresses" ALTER COLUMN "addressable_id" SET NOT NULL;
ALTER TABLE "addresses" ALTER COLUMN "addressable_type" SET NOT NULL;
ALTER TABLE "account_opportunities" ALTER COLUMN "account_id" SET NOT NULL;
ALTER TABLE "account_opportunities" ALTER COLUMN "opportunity_id" SET NOT NULL;
ALTER TABLE "emails" ALTER COLUMN "mediator_id" SET NOT NULL;
ALTER TABLE "emails" ALTER COLUMN "mediator_type" SET NOT NULL;
ALTER TABLE "campaigns" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "campaigns" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "campaigns" ALTER COLUMN "assigned_to" SET NOT NULL;
ALTER TABLE "permissions" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "permissions" ALTER COLUMN "asset_id" SET NOT NULL;
ALTER TABLE "permissions" ALTER COLUMN "asset_type" SET NOT NULL;
ALTER TABLE "permissions" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "tasks" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "tasks" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "tasks" ALTER COLUMN "assigned_to" SET NOT NULL;
ALTER TABLE "preferences" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "sessions" ALTER COLUMN "updated_at" SET NOT NULL;
ALTER TABLE "account_contacts" ALTER COLUMN "account_id" SET NOT NULL;
ALTER TABLE "account_contacts" ALTER COLUMN "contact_id" SET NOT NULL;
ALTER TABLE "contact_opportunities" ALTER COLUMN "contact_id" SET NOT NULL;
ALTER TABLE "contact_opportunities" ALTER COLUMN "opportunity_id" SET NOT NULL;
ALTER TABLE "opportunities" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "opportunities" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "opportunities" ALTER COLUMN "assigned_to" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "reset_password_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "remember_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "confirmation_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "authentication_token" SET NOT NULL;
ALTER TABLE "tags" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "versions" ALTER COLUMN "whodunnit" SET NOT NULL;
ALTER TABLE "versions" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "versions" ALTER COLUMN "transaction_id" SET NOT NULL;
ALTER TABLE "versions" ALTER COLUMN "related_id" SET NOT NULL;
ALTER TABLE "versions" ALTER COLUMN "related_type" SET NOT NULL;
ALTER TABLE "activities" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "activities" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "leads" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "leads" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "leads" ALTER COLUMN "assigned_to" SET NOT NULL;
ALTER TABLE "lists" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "taggable_id" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "taggable_type" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "context" SET NOT NULL;
ALTER TABLE "accounts" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "accounts" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "accounts" ALTER COLUMN "assigned_to" SET NOT NULL;
ALTER TABLE "fields" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "fields" ALTER COLUMN "field_group_id" SET NOT NULL;
ALTER TABLE "contacts" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "contacts" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "contacts" ALTER COLUMN "assigned_to" SET NOT NULL;
ALTER TABLE "groups_users" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "groups_users" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "groups_users" ADD CONSTRAINT "wetune_u_8cf6480d21c4b013" UNIQUE ("group_id", "user_id");
ALTER TABLE "account_opportunities" ADD CONSTRAINT "wetune_fk_3aa83ffe8cb910a6" FOREIGN KEY ("opportunity_id") REFERENCES "opportunities" ("id");
ALTER TABLE "contact_opportunities" ADD CONSTRAINT "wetune_fk_65d0c8ed021031e6" FOREIGN KEY ("opportunity_id") REFERENCES "opportunities" ("id");
ALTER TABLE "groups_users" ADD CONSTRAINT "wetune_fk_0366d79237c54f93" FOREIGN KEY ("group_id") REFERENCES "groups" ("id");
ALTER TABLE "contact_opportunities" ADD CONSTRAINT "wetune_fk_2c0a5c1fc70147d9" FOREIGN KEY ("contact_id") REFERENCES "contacts" ("id");
ALTER TABLE "account_contacts" ADD CONSTRAINT "wetune_fk_9727f87a44782c3e" FOREIGN KEY ("account_id") REFERENCES "accounts" ("id");
ALTER TABLE "account_opportunities" ADD CONSTRAINT "wetune_fk_9baf464982cc5abe" FOREIGN KEY ("account_id") REFERENCES "accounts" ("id");
ALTER TABLE "taggings" ADD CONSTRAINT "wetune_fk_0fce70899dd8a225" FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");
ALTER TABLE "account_contacts" ADD CONSTRAINT "wetune_fk_af1091ba39175f8f" FOREIGN KEY ("contact_id") REFERENCES "contacts" ("id");
