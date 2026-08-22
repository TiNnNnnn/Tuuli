CREATE TABLE "admin_user" (
  "id" INTEGER NOT NULL,
  "username" VARCHAR(255) NOT NULL,
  "password" VARCHAR(255) NOT NULL,
  "in_time" TIMESTAMP NOT NULL,
  "role_id" INTEGER NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("username")
);

CREATE TABLE "code" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "code" VARCHAR(255) NOT NULL,
  "in_time" TIMESTAMP NOT NULL,
  "expire_time" TIMESTAMP NOT NULL,
  "email" VARCHAR(255),
  "mobile" VARCHAR(255),
  "used" INTEGER NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("code")
);

CREATE TABLE "collect" (
  "topic_id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "in_time" TIMESTAMP NOT NULL
);

CREATE TABLE "comment" (
  "id" INTEGER NOT NULL,
  "content" TEXT NOT NULL,
  "topic_id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "in_time" TIMESTAMP NOT NULL,
  "comment_id" INTEGER,
  "up_ids" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "flyway_schema_history" (
  "installed_rank" INTEGER NOT NULL,
  "version" VARCHAR(50),
  "description" VARCHAR(200) NOT NULL,
  "type" VARCHAR(20) NOT NULL,
  "script" VARCHAR(1000) NOT NULL,
  "checksum" INTEGER,
  "installed_by" VARCHAR(100) NOT NULL,
  "installed_on" TIMESTAMP NOT NULL,
  "execution_time" INTEGER NOT NULL,
  "success" SMALLINT NOT NULL,
  PRIMARY KEY ("installed_rank")
);

CREATE TABLE "notification" (
  "id" INTEGER NOT NULL,
  "topic_id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "target_user_id" INTEGER NOT NULL,
  "action" VARCHAR(255) NOT NULL,
  "in_time" TIMESTAMP NOT NULL,
  "read" INTEGER NOT NULL,
  "content" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "oauth_user" (
  "id" INTEGER NOT NULL,
  "oauth_id" INTEGER,
  "type" VARCHAR(255) NOT NULL,
  "login" VARCHAR(255) NOT NULL,
  "access_token" VARCHAR(255) NOT NULL,
  "in_time" TIMESTAMP NOT NULL,
  "bio" TEXT,
  "email" VARCHAR(255),
  "user_id" INTEGER NOT NULL,
  "refresh_token" VARCHAR(255),
  "union_id" VARCHAR(255),
  "expires_in" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "permission" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255) NOT NULL,
  "pid" INTEGER NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("name"),
  UNIQUE ("value")
);

CREATE TABLE "role" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("name")
);

CREATE TABLE "role_permission" (
  "role_id" INTEGER NOT NULL,
  "permission_id" INTEGER NOT NULL
);

CREATE TABLE "sensitive_word" (
  "id" INTEGER NOT NULL,
  "word" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "system_config" (
  "id" BIGINT NOT NULL,
  "key" VARCHAR(255),
  "value" VARCHAR(255),
  "description" VARCHAR(1000) NOT NULL,
  "pid" INTEGER NOT NULL,
  "type" VARCHAR(255),
  "option" VARCHAR(255),
  "reboot" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "tag" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "description" VARCHAR(1000),
  "icon" VARCHAR(255),
  "topic_count" INTEGER NOT NULL,
  "in_time" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("name")
);

CREATE TABLE "topic" (
  "id" INTEGER NOT NULL,
  "title" VARCHAR(255) NOT NULL,
  "content" TEXT,
  "in_time" TIMESTAMP NOT NULL,
  "modify_time" TIMESTAMP,
  "user_id" INTEGER NOT NULL,
  "comment_count" INTEGER NOT NULL,
  "collect_count" INTEGER NOT NULL,
  "view" INTEGER NOT NULL,
  "top" INTEGER NOT NULL,
  "good" INTEGER NOT NULL,
  "up_ids" TEXT,
  PRIMARY KEY ("id"),
  UNIQUE ("title")
);

CREATE TABLE "topic_tag" (
  "tag_id" INTEGER NOT NULL,
  "topic_id" INTEGER NOT NULL
);

CREATE TABLE "user" (
  "id" INTEGER NOT NULL,
  "username" VARCHAR(255) NOT NULL,
  "password" VARCHAR(255),
  "avatar" VARCHAR(1000),
  "email" VARCHAR(255),
  "mobile" VARCHAR(255),
  "website" VARCHAR(255),
  "bio" VARCHAR(1000),
  "score" INTEGER NOT NULL,
  "in_time" TIMESTAMP NOT NULL,
  "token" VARCHAR(255) NOT NULL,
  "telegram_name" VARCHAR(255),
  "email_notification" INTEGER NOT NULL,
  "active" INTEGER NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("username"),
  UNIQUE ("token")
);

ALTER TABLE "admin_user" ADD FOREIGN KEY ("role_id") REFERENCES "role" ("id");

ALTER TABLE "code" ADD FOREIGN KEY ("user_id") REFERENCES "user" ("id");

ALTER TABLE "collect" ADD FOREIGN KEY ("topic_id") REFERENCES "topic" ("id");

ALTER TABLE "collect" ADD FOREIGN KEY ("user_id") REFERENCES "user" ("id");

ALTER TABLE "comment" ADD FOREIGN KEY ("topic_id") REFERENCES "topic" ("id");

ALTER TABLE "comment" ADD FOREIGN KEY ("user_id") REFERENCES "user" ("id");

ALTER TABLE "notification" ADD FOREIGN KEY ("topic_id") REFERENCES "topic" ("id");

ALTER TABLE "notification" ADD FOREIGN KEY ("user_id") REFERENCES "user" ("id");

ALTER TABLE "notification" ADD FOREIGN KEY ("target_user_id") REFERENCES "user" ("id");

ALTER TABLE "oauth_user" ADD FOREIGN KEY ("user_id") REFERENCES "user" ("id");

ALTER TABLE "role_permission" ADD FOREIGN KEY ("role_id") REFERENCES "role" ("id");

ALTER TABLE "role_permission" ADD FOREIGN KEY ("permission_id") REFERENCES "permission" ("id");

ALTER TABLE "topic" ADD FOREIGN KEY ("user_id") REFERENCES "user" ("id");

ALTER TABLE "topic_tag" ADD FOREIGN KEY ("tag_id") REFERENCES "tag" ("id");

ALTER TABLE "topic_tag" ADD FOREIGN KEY ("topic_id") REFERENCES "topic" ("id");

-- WeTune schema patches
ALTER TABLE "code" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "topic_tag" ADD CONSTRAINT "wetune_u_d91a20d57855e088" UNIQUE ("topic_id", "tag_id");
ALTER TABLE "role_permission" ADD CONSTRAINT "wetune_u_fb84ae712a893a03" UNIQUE ("role_id", "permission_id");
ALTER TABLE "collect" ADD CONSTRAINT "wetune_u_6946b8c36c37b6c7" UNIQUE ("topic_id", "user_id");
