CREATE TABLE "cms_category" (
  "id" INTEGER NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "name" VARCHAR(50) NOT NULL,
  "parent_id" INTEGER,
  "type_id" INTEGER,
  "child_ids" TEXT,
  "tag_type_ids" TEXT,
  "code" VARCHAR(50) NOT NULL,
  "template_path" VARCHAR(255),
  "path" VARCHAR(1000),
  "only_url" SMALLINT NOT NULL,
  "has_static" SMALLINT NOT NULL,
  "url" VARCHAR(1000),
  "content_path" VARCHAR(1000),
  "contain_child" SMALLINT NOT NULL,
  "page_size" INTEGER,
  "allow_contribute" SMALLINT NOT NULL,
  "sort" INTEGER NOT NULL,
  "hidden" SMALLINT NOT NULL,
  "disabled" SMALLINT NOT NULL,
  "extend_id" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("site_id", "code")
);

CREATE TABLE "cms_category_attribute" (
  "category_id" INTEGER NOT NULL,
  "title" VARCHAR(80),
  "keywords" VARCHAR(100),
  "description" VARCHAR(300),
  "data" TEXT,
  PRIMARY KEY ("category_id")
);

CREATE TABLE "cms_category_model" (
  "category_id" INTEGER NOT NULL,
  "model_id" VARCHAR(20) NOT NULL,
  "template_path" VARCHAR(200),
  PRIMARY KEY ("category_id", "model_id")
);

CREATE TABLE "cms_category_type" (
  "id" INTEGER NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "name" VARCHAR(50) NOT NULL,
  "sort" INTEGER NOT NULL,
  "extend_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "cms_comment" (
  "id" BIGINT NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "user_id" BIGINT NOT NULL,
  "reply_id" BIGINT,
  "reply_user_id" BIGINT,
  "content_id" BIGINT NOT NULL,
  "check_user_id" BIGINT,
  "check_date" TIMESTAMP,
  "update_date" TIMESTAMP,
  "create_date" TIMESTAMP NOT NULL,
  "status" INTEGER NOT NULL,
  "disabled" SMALLINT NOT NULL,
  "text" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "cms_content" (
  "id" BIGINT NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "title" VARCHAR(255) NOT NULL,
  "user_id" BIGINT NOT NULL,
  "check_user_id" BIGINT,
  "category_id" INTEGER NOT NULL,
  "model_id" VARCHAR(20) NOT NULL,
  "parent_id" BIGINT,
  "quote_content_id" BIGINT,
  "copied" SMALLINT NOT NULL,
  "author" VARCHAR(50),
  "editor" VARCHAR(50),
  "only_url" SMALLINT NOT NULL,
  "has_images" SMALLINT NOT NULL,
  "has_files" SMALLINT NOT NULL,
  "has_static" SMALLINT NOT NULL,
  "url" VARCHAR(1000),
  "description" VARCHAR(300),
  "tag_ids" TEXT,
  "dictionar_values" TEXT,
  "cover" VARCHAR(255),
  "childs" INTEGER NOT NULL,
  "scores" INTEGER NOT NULL,
  "comments" INTEGER NOT NULL,
  "clicks" INTEGER NOT NULL,
  "publish_date" TIMESTAMP NOT NULL,
  "expiry_date" TIMESTAMP,
  "check_date" TIMESTAMP,
  "update_date" TIMESTAMP,
  "create_date" TIMESTAMP NOT NULL,
  "sort" INTEGER NOT NULL,
  "status" INTEGER NOT NULL,
  "disabled" SMALLINT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "cms_content_attribute" (
  "content_id" BIGINT NOT NULL,
  "source" VARCHAR(50),
  "source_url" VARCHAR(1000),
  "data" TEXT,
  "search_text" TEXT,
  "text" TEXT,
  "word_count" INTEGER NOT NULL,
  PRIMARY KEY ("content_id")
);

CREATE TABLE "cms_content_file" (
  "id" BIGINT NOT NULL,
  "content_id" BIGINT NOT NULL,
  "user_id" BIGINT NOT NULL,
  "file_path" VARCHAR(255) NOT NULL,
  "file_type" VARCHAR(20) NOT NULL,
  "file_size" BIGINT NOT NULL,
  "clicks" INTEGER NOT NULL,
  "sort" INTEGER NOT NULL,
  "description" VARCHAR(300),
  PRIMARY KEY ("id")
);

CREATE TABLE "cms_content_related" (
  "id" BIGINT NOT NULL,
  "content_id" BIGINT NOT NULL,
  "related_content_id" BIGINT,
  "user_id" BIGINT NOT NULL,
  "url" VARCHAR(1000),
  "title" VARCHAR(255),
  "description" VARCHAR(300),
  "clicks" INTEGER NOT NULL,
  "sort" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "cms_dictionary" (
  "id" VARCHAR(20) NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "name" VARCHAR(100) NOT NULL,
  "multiple" SMALLINT NOT NULL,
  PRIMARY KEY ("id", "site_id")
);

CREATE TABLE "cms_dictionary_data" (
  "dictionary_id" VARCHAR(20) NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "value" VARCHAR(50) NOT NULL,
  "text" VARCHAR(100) NOT NULL,
  PRIMARY KEY ("dictionary_id", "site_id", "value")
);

CREATE TABLE "cms_place" (
  "id" BIGINT NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "path" VARCHAR(100) NOT NULL,
  "user_id" BIGINT,
  "check_user_id" BIGINT,
  "item_type" VARCHAR(50),
  "item_id" BIGINT,
  "title" VARCHAR(255) NOT NULL,
  "url" VARCHAR(1000),
  "cover" VARCHAR(255),
  "create_date" TIMESTAMP NOT NULL,
  "publish_date" TIMESTAMP NOT NULL,
  "expiry_date" TIMESTAMP,
  "status" INTEGER NOT NULL,
  "clicks" INTEGER NOT NULL,
  "disabled" SMALLINT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "cms_place_attribute" (
  "place_id" BIGINT NOT NULL,
  "data" TEXT,
  PRIMARY KEY ("place_id")
);

CREATE TABLE "cms_tag" (
  "id" BIGINT NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "name" VARCHAR(50) NOT NULL,
  "type_id" INTEGER,
  "search_count" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "cms_tag_type" (
  "id" INTEGER NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "name" VARCHAR(50) NOT NULL,
  "count" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "cms_word" (
  "id" BIGINT NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "name" VARCHAR(100) NOT NULL,
  "search_count" INTEGER NOT NULL,
  "hidden" SMALLINT NOT NULL,
  "create_date" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("name", "site_id")
);

CREATE TABLE "log_login" (
  "id" BIGINT NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "name" VARCHAR(50) NOT NULL,
  "user_id" BIGINT,
  "ip" VARCHAR(64) NOT NULL,
  "channel" VARCHAR(50) NOT NULL,
  "result" SMALLINT NOT NULL,
  "create_date" TIMESTAMP NOT NULL,
  "error_password" VARCHAR(100),
  PRIMARY KEY ("id")
);

CREATE TABLE "log_operate" (
  "id" BIGINT NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "user_id" BIGINT,
  "channel" VARCHAR(50) NOT NULL,
  "operate" VARCHAR(40) NOT NULL,
  "ip" VARCHAR(64),
  "create_date" TIMESTAMP NOT NULL,
  "content" TEXT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "log_task" (
  "id" BIGINT NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "task_id" INTEGER NOT NULL,
  "begintime" TIMESTAMP NOT NULL,
  "endtime" TIMESTAMP,
  "success" SMALLINT NOT NULL,
  "result" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "log_upload" (
  "id" BIGINT NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "user_id" BIGINT NOT NULL,
  "channel" VARCHAR(50) NOT NULL,
  "original_name" VARCHAR(255),
  "file_type" VARCHAR(20) NOT NULL,
  "file_size" BIGINT NOT NULL,
  "ip" VARCHAR(64),
  "create_date" TIMESTAMP NOT NULL,
  "file_path" VARCHAR(500) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "sys_app" (
  "id" INTEGER NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "channel" VARCHAR(50) NOT NULL,
  "app_key" VARCHAR(50) NOT NULL,
  "app_secret" VARCHAR(50) NOT NULL,
  "authorized_apis" TEXT,
  "expiry_minutes" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("app_key")
);

CREATE TABLE "sys_app_client" (
  "id" BIGINT NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "channel" VARCHAR(20) NOT NULL,
  "uuid" VARCHAR(50) NOT NULL,
  "user_id" BIGINT,
  "client_version" VARCHAR(50),
  "last_login_date" TIMESTAMP,
  "last_login_ip" VARCHAR(64),
  "create_date" TIMESTAMP NOT NULL,
  "disabled" SMALLINT NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("site_id", "channel", "uuid")
);

CREATE TABLE "sys_app_token" (
  "auth_token" VARCHAR(40) NOT NULL,
  "app_id" INTEGER NOT NULL,
  "create_date" TIMESTAMP NOT NULL,
  "expiry_date" TIMESTAMP,
  PRIMARY KEY ("auth_token")
);

CREATE TABLE "sys_cluster" (
  "uuid" VARCHAR(40) NOT NULL,
  "create_date" TIMESTAMP NOT NULL,
  "heartbeat_date" TIMESTAMP NOT NULL,
  "master" SMALLINT NOT NULL,
  "cms_version" VARCHAR(20),
  PRIMARY KEY ("uuid")
);

CREATE TABLE "sys_config_data" (
  "site_id" SMALLINT NOT NULL,
  "code" VARCHAR(50) NOT NULL,
  "data" TEXT NOT NULL,
  PRIMARY KEY ("site_id", "code")
);

CREATE TABLE "sys_dept" (
  "id" INTEGER NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "name" VARCHAR(50) NOT NULL,
  "parent_id" INTEGER,
  "description" VARCHAR(300),
  "user_id" BIGINT,
  "max_sort" INTEGER NOT NULL,
  "owns_all_category" SMALLINT NOT NULL,
  "owns_all_page" SMALLINT NOT NULL,
  "owns_all_config" SMALLINT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "sys_dept_category" (
  "dept_id" INTEGER NOT NULL,
  "category_id" INTEGER NOT NULL,
  PRIMARY KEY ("dept_id", "category_id")
);

CREATE TABLE "sys_dept_config" (
  "dept_id" INTEGER NOT NULL,
  "config" VARCHAR(100) NOT NULL,
  PRIMARY KEY ("dept_id", "config")
);

CREATE TABLE "sys_dept_page" (
  "dept_id" INTEGER NOT NULL,
  "page" VARCHAR(100) NOT NULL,
  PRIMARY KEY ("dept_id", "page")
);

CREATE TABLE "sys_domain" (
  "name" VARCHAR(100) NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "wild" SMALLINT NOT NULL,
  "path" VARCHAR(100),
  PRIMARY KEY ("name")
);

CREATE TABLE "sys_email_token" (
  "auth_token" VARCHAR(40) NOT NULL,
  "user_id" BIGINT NOT NULL,
  "email" VARCHAR(100) NOT NULL,
  "create_date" TIMESTAMP NOT NULL,
  "expiry_date" TIMESTAMP NOT NULL,
  PRIMARY KEY ("auth_token")
);

CREATE TABLE "sys_extend" (
  "id" INTEGER NOT NULL,
  "item_type" VARCHAR(20) NOT NULL,
  "item_id" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "sys_extend_field" (
  "extend_id" INTEGER NOT NULL,
  "code" VARCHAR(20) NOT NULL,
  "required" SMALLINT NOT NULL,
  "searchable" SMALLINT NOT NULL,
  "maxlength" INTEGER,
  "name" VARCHAR(20) NOT NULL,
  "description" VARCHAR(100),
  "input_type" VARCHAR(20) NOT NULL,
  "default_value" VARCHAR(50),
  "dictionary_id" VARCHAR(20),
  "sort" INTEGER NOT NULL,
  PRIMARY KEY ("extend_id", "code")
);

CREATE TABLE "sys_module" (
  "id" VARCHAR(30) NOT NULL,
  "url" VARCHAR(255),
  "authorized_url" TEXT,
  "attached" VARCHAR(50),
  "parent_id" VARCHAR(30),
  "menu" SMALLINT NOT NULL,
  "sort" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "sys_module_lang" (
  "module_id" VARCHAR(30) NOT NULL,
  "lang" VARCHAR(20) NOT NULL,
  "value" VARCHAR(100),
  PRIMARY KEY ("module_id", "lang")
);

CREATE TABLE "sys_role" (
  "id" INTEGER NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "name" VARCHAR(50) NOT NULL,
  "owns_all_right" SMALLINT NOT NULL,
  "show_all_module" SMALLINT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "sys_role_authorized" (
  "role_id" INTEGER NOT NULL,
  "url" VARCHAR(100) NOT NULL,
  PRIMARY KEY ("role_id", "url")
);

CREATE TABLE "sys_role_module" (
  "role_id" INTEGER NOT NULL,
  "module_id" VARCHAR(30) NOT NULL,
  PRIMARY KEY ("role_id", "module_id")
);

CREATE TABLE "sys_role_user" (
  "role_id" INTEGER NOT NULL,
  "user_id" BIGINT NOT NULL,
  PRIMARY KEY ("role_id", "user_id")
);

CREATE TABLE "sys_site" (
  "id" SMALLINT NOT NULL,
  "parent_id" SMALLINT,
  "name" VARCHAR(50) NOT NULL,
  "use_static" SMALLINT NOT NULL,
  "site_path" VARCHAR(255) NOT NULL,
  "use_ssi" SMALLINT NOT NULL,
  "dynamic_path" VARCHAR(255) NOT NULL,
  "disabled" SMALLINT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "sys_task" (
  "id" INTEGER NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "name" VARCHAR(50) NOT NULL,
  "status" INTEGER NOT NULL,
  "cron_expression" VARCHAR(50) NOT NULL,
  "description" VARCHAR(300),
  "file_path" VARCHAR(255),
  "update_date" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "sys_user" (
  "id" BIGINT NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "name" VARCHAR(50) NOT NULL,
  "password" VARCHAR(128) NOT NULL,
  "salt" VARCHAR(20),
  "weak_password" SMALLINT NOT NULL,
  "nick_name" VARCHAR(45) NOT NULL,
  "dept_id" INTEGER,
  "owns_all_content" SMALLINT NOT NULL,
  "roles" TEXT,
  "email" VARCHAR(100),
  "email_checked" SMALLINT NOT NULL,
  "superuser_access" SMALLINT NOT NULL,
  "disabled" SMALLINT NOT NULL,
  "last_login_date" TIMESTAMP,
  "last_login_ip" VARCHAR(64),
  "login_count" INTEGER NOT NULL,
  "registered_date" TIMESTAMP,
  PRIMARY KEY ("id"),
  UNIQUE ("name", "site_id")
);

CREATE TABLE "sys_user_token" (
  "auth_token" VARCHAR(40) NOT NULL,
  "site_id" SMALLINT NOT NULL,
  "user_id" BIGINT NOT NULL,
  "channel" VARCHAR(50) NOT NULL,
  "create_date" TIMESTAMP NOT NULL,
  "expiry_date" TIMESTAMP,
  "login_ip" VARCHAR(64) NOT NULL,
  PRIMARY KEY ("auth_token")
);

-- WeTune schema patches
ALTER TABLE "cms_comment" ALTER COLUMN "update_date" SET NOT NULL;
ALTER TABLE "cms_comment" ALTER COLUMN "reply_user_id" SET NOT NULL;
ALTER TABLE "cms_comment" ALTER COLUMN "reply_id" SET NOT NULL;
ALTER TABLE "log_login" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "cms_category" ALTER COLUMN "type_id" SET NOT NULL;
ALTER TABLE "cms_category" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "sys_task" ALTER COLUMN "update_date" SET NOT NULL;
ALTER TABLE "sys_user" ALTER COLUMN "email" SET NOT NULL;
ALTER TABLE "sys_user" ALTER COLUMN "last_login_date" SET NOT NULL;
ALTER TABLE "sys_user" ALTER COLUMN "dept_id" SET NOT NULL;
ALTER TABLE "sys_app_client" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "sys_module" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "log_upload" ALTER COLUMN "ip" SET NOT NULL;
ALTER TABLE "log_operate" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "log_operate" ALTER COLUMN "ip" SET NOT NULL;
ALTER TABLE "cms_place" ALTER COLUMN "item_type" SET NOT NULL;
ALTER TABLE "cms_place" ALTER COLUMN "item_id" SET NOT NULL;
ALTER TABLE "cms_place" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "cms_place" ALTER COLUMN "check_user_id" SET NOT NULL;
ALTER TABLE "cms_place" ALTER COLUMN "expiry_date" SET NOT NULL;
ALTER TABLE "cms_content_related" ALTER COLUMN "related_content_id" SET NOT NULL;
ALTER TABLE "cms_content" ALTER COLUMN "check_date" SET NOT NULL;
ALTER TABLE "cms_content" ALTER COLUMN "update_date" SET NOT NULL;
ALTER TABLE "cms_content" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "cms_content" ALTER COLUMN "expiry_date" SET NOT NULL;
ALTER TABLE "cms_content" ALTER COLUMN "quote_content_id" SET NOT NULL;
