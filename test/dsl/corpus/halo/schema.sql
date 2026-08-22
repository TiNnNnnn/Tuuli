CREATE TABLE "attachments" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "file_key" VARCHAR(2047),
  "height" INTEGER,
  "media_type" VARCHAR(50) NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "path" VARCHAR(1023) NOT NULL,
  "size" BIGINT NOT NULL,
  "suffix" VARCHAR(50),
  "thumb_path" VARCHAR(1023),
  "type" INTEGER,
  "width" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "categories" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "description" VARCHAR(100),
  "name" VARCHAR(50) NOT NULL,
  "parent_id" INTEGER,
  "slug_name" VARCHAR(50) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("slug_name")
);

CREATE TABLE "comments" (
  "type" INTEGER NOT NULL,
  "id" BIGINT NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "author" VARCHAR(50) NOT NULL,
  "author_url" VARCHAR(512),
  "content" VARCHAR(1023) NOT NULL,
  "email" VARCHAR(255) NOT NULL,
  "gravatar_md5" VARCHAR(128),
  "ip_address" VARCHAR(127),
  "is_admin" SMALLINT,
  "parent_id" BIGINT,
  "post_id" INTEGER NOT NULL,
  "status" INTEGER,
  "top_priority" INTEGER,
  "user_agent" VARCHAR(512),
  PRIMARY KEY ("id")
);

CREATE TABLE "journals" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "content" VARCHAR(1023) NOT NULL,
  "likes" BIGINT,
  "type" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "links" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "description" VARCHAR(255),
  "logo" VARCHAR(1023),
  "name" VARCHAR(255) NOT NULL,
  "priority" INTEGER,
  "team" VARCHAR(255),
  "url" VARCHAR(1023) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "logs" (
  "id" BIGINT NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "content" VARCHAR(1023) NOT NULL,
  "ip_address" VARCHAR(127),
  "log_key" VARCHAR(1023),
  "type" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "menus" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "icon" VARCHAR(50),
  "name" VARCHAR(50) NOT NULL,
  "parent_id" INTEGER,
  "priority" INTEGER,
  "target" VARCHAR(20),
  "team" VARCHAR(255),
  "url" VARCHAR(1023) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "metas" (
  "type" INTEGER NOT NULL,
  "id" BIGINT NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "meta_key" VARCHAR(100) NOT NULL,
  "post_id" INTEGER NOT NULL,
  "meta_value" VARCHAR(1023) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "options" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "option_key" VARCHAR(100) NOT NULL,
  "option_value" VARCHAR(1023) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "photos" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "description" VARCHAR(255),
  "location" VARCHAR(255),
  "name" VARCHAR(255) NOT NULL,
  "take_time" TIMESTAMP NOT NULL,
  "team" VARCHAR(255),
  "thumbnail" VARCHAR(1023),
  "url" VARCHAR(1023) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "post_categories" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "category_id" INTEGER,
  "post_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "post_tags" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "post_id" INTEGER NOT NULL,
  "tag_id" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "posts" (
  "type" INTEGER NOT NULL,
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "create_from" INTEGER,
  "disallow_comment" INTEGER,
  "edit_time" TIMESTAMP NOT NULL,
  "format_content" TEXT NOT NULL,
  "likes" BIGINT,
  "original_content" TEXT NOT NULL,
  "password" VARCHAR(255),
  "status" INTEGER,
  "summary" VARCHAR(500),
  "template" VARCHAR(255),
  "thumbnail" VARCHAR(1023),
  "title" VARCHAR(100) NOT NULL,
  "top_priority" INTEGER,
  "url" VARCHAR(255) NOT NULL,
  "visits" BIGINT,
  PRIMARY KEY ("id"),
  UNIQUE ("url")
);

CREATE TABLE "tags" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "slug_name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("slug_name")
);

CREATE TABLE "theme_settings" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "setting_key" VARCHAR(255) NOT NULL,
  "theme_id" VARCHAR(255) NOT NULL,
  "setting_value" VARCHAR(10239) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "users" (
  "id" INTEGER NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "deleted" SMALLINT,
  "update_time" TIMESTAMP NOT NULL,
  "avatar" VARCHAR(1023),
  "description" VARCHAR(1023),
  "email" VARCHAR(127),
  "expire_time" TIMESTAMP NOT NULL,
  "nickname" VARCHAR(255) NOT NULL,
  "password" VARCHAR(255) NOT NULL,
  "username" VARCHAR(50) NOT NULL,
  PRIMARY KEY ("id")
);

-- WeTune schema patches
ALTER TABLE "post_categories" ADD CONSTRAINT "wetune_fk_8b879da33bffa376" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "post_tags" ADD CONSTRAINT "wetune_fk_91506f9f1fe72d09" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
