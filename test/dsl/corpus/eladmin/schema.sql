CREATE TABLE "alipay_config" (
  "id" BIGINT NOT NULL,
  "app_id" VARCHAR(255),
  "charset" VARCHAR(255),
  "format" VARCHAR(255),
  "gateway_url" VARCHAR(255),
  "notify_url" VARCHAR(255),
  "private_key" TEXT,
  "public_key" TEXT,
  "return_url" VARCHAR(255),
  "sign_type" VARCHAR(255),
  "sys_service_provider_id" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "dept" (
  "id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "pid" BIGINT NOT NULL,
  "create_time" TIMESTAMP,
  "enabled" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "dict" (
  "id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "remark" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "dict_detail" (
  "id" BIGINT NOT NULL,
  "label" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255) NOT NULL,
  "sort" VARCHAR(255),
  "dict_id" BIGINT,
  PRIMARY KEY ("id")
);

CREATE TABLE "email_config" (
  "id" BIGINT NOT NULL,
  "from_user" VARCHAR(255),
  "host" VARCHAR(255),
  "pass" VARCHAR(255),
  "port" VARCHAR(255),
  "user" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "gen_config" (
  "id" BIGINT NOT NULL,
  "author" VARCHAR(255),
  "cover" INTEGER,
  "module_name" VARCHAR(255),
  "pack" VARCHAR(255),
  "path" VARCHAR(255),
  "api_path" VARCHAR(255),
  "prefix" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "job" (
  "id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "enabled" INTEGER NOT NULL,
  "create_time" TIMESTAMP,
  "sort" BIGINT NOT NULL,
  "dept_id" BIGINT,
  PRIMARY KEY ("id")
);

CREATE TABLE "local_storage" (
  "id" BIGINT NOT NULL,
  "real_name" VARCHAR(255),
  "name" VARCHAR(255),
  "suffix" VARCHAR(255),
  "path" VARCHAR(255),
  "type" VARCHAR(255),
  "size" VARCHAR(100),
  "operate" VARCHAR(255),
  "create_time" TIMESTAMP,
  "update_time" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "log" (
  "id" BIGINT NOT NULL,
  "create_time" TIMESTAMP,
  "description" VARCHAR(255),
  "exception_detail" TEXT,
  "log_type" VARCHAR(255),
  "method" VARCHAR(255),
  "params" TEXT,
  "request_ip" VARCHAR(255),
  "time" BIGINT,
  "username" VARCHAR(255),
  "address" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "menu" (
  "id" BIGINT NOT NULL,
  "create_time" TIMESTAMP,
  "i_frame" INTEGER,
  "name" VARCHAR(255),
  "component" VARCHAR(255),
  "pid" BIGINT NOT NULL,
  "sort" BIGINT NOT NULL,
  "icon" VARCHAR(255),
  "path" VARCHAR(255),
  "cache" INTEGER,
  "hidden" INTEGER,
  "component_name" VARCHAR(20),
  PRIMARY KEY ("id")
);

CREATE TABLE "permission" (
  "id" BIGINT NOT NULL,
  "alias" VARCHAR(255),
  "create_time" TIMESTAMP,
  "name" VARCHAR(255),
  "pid" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "picture" (
  "id" BIGINT NOT NULL,
  "create_time" TIMESTAMP,
  "delete_url" VARCHAR(255),
  "filename" VARCHAR(255),
  "height" VARCHAR(255),
  "size" VARCHAR(255),
  "url" VARCHAR(255),
  "username" VARCHAR(255),
  "width" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "qiniu_config" (
  "id" BIGINT NOT NULL,
  "access_key" TEXT,
  "bucket" VARCHAR(255),
  "host" VARCHAR(255) NOT NULL,
  "secret_key" TEXT,
  "type" VARCHAR(255),
  "zone" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "qiniu_content" (
  "id" BIGINT NOT NULL,
  "bucket" VARCHAR(255),
  "name" VARCHAR(255),
  "size" VARCHAR(255),
  "type" VARCHAR(255),
  "update_time" TIMESTAMP,
  "url" VARCHAR(255),
  "suffix" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "quartz_job" (
  "id" BIGINT NOT NULL,
  "bean_name" VARCHAR(255),
  "cron_expression" VARCHAR(255),
  "is_pause" INTEGER,
  "job_name" VARCHAR(255),
  "method_name" VARCHAR(255),
  "params" VARCHAR(255),
  "remark" VARCHAR(255),
  "update_time" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "quartz_log" (
  "id" BIGINT NOT NULL,
  "baen_name" VARCHAR(255),
  "create_time" TIMESTAMP,
  "cron_expression" VARCHAR(255),
  "exception_detail" TEXT,
  "is_success" INTEGER,
  "job_name" VARCHAR(255),
  "method_name" VARCHAR(255),
  "params" VARCHAR(255),
  "time" BIGINT,
  PRIMARY KEY ("id")
);

CREATE TABLE "role" (
  "id" BIGINT NOT NULL,
  "create_time" TIMESTAMP,
  "name" VARCHAR(255) NOT NULL,
  "remark" VARCHAR(255),
  "data_scope" VARCHAR(255),
  "level" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "roles_depts" (
  "role_id" BIGINT NOT NULL,
  "dept_id" BIGINT NOT NULL,
  PRIMARY KEY ("role_id", "dept_id")
);

CREATE TABLE "roles_menus" (
  "menu_id" BIGINT NOT NULL,
  "role_id" BIGINT NOT NULL,
  PRIMARY KEY ("menu_id", "role_id")
);

CREATE TABLE "roles_permissions" (
  "role_id" BIGINT NOT NULL,
  "permission_id" BIGINT NOT NULL,
  PRIMARY KEY ("role_id", "permission_id")
);

CREATE TABLE "user" (
  "id" BIGINT NOT NULL,
  "avatar_id" BIGINT,
  "create_time" TIMESTAMP,
  "email" VARCHAR(255),
  "enabled" BIGINT,
  "password" VARCHAR(255),
  "username" VARCHAR(255),
  "last_password_reset_time" TIMESTAMP,
  "dept_id" BIGINT,
  "phone" VARCHAR(255),
  "job_id" BIGINT,
  PRIMARY KEY ("id"),
  UNIQUE ("email"),
  UNIQUE ("username")
);

CREATE TABLE "user_avatar" (
  "id" BIGINT NOT NULL,
  "real_name" VARCHAR(255),
  "path" VARCHAR(255),
  "size" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "users_roles" (
  "user_id" BIGINT NOT NULL,
  "role_id" BIGINT NOT NULL,
  PRIMARY KEY ("user_id", "role_id")
);

CREATE TABLE "verification_code" (
  "id" BIGINT NOT NULL,
  "code" VARCHAR(255),
  "create_time" TIMESTAMP,
  "status" INTEGER,
  "type" VARCHAR(255),
  "value" VARCHAR(255),
  "scenes" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "visits" (
  "id" BIGINT NOT NULL,
  "create_time" TIMESTAMP,
  "date" VARCHAR(255),
  "ip_counts" BIGINT,
  "pv_counts" BIGINT,
  "week_day" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("date")
);

ALTER TABLE "dict_detail" ADD FOREIGN KEY ("dict_id") REFERENCES "dict" ("id");

ALTER TABLE "job" ADD FOREIGN KEY ("dept_id") REFERENCES "dept" ("id");

ALTER TABLE "roles_depts" ADD FOREIGN KEY ("dept_id") REFERENCES "dept" ("id");

ALTER TABLE "roles_depts" ADD FOREIGN KEY ("role_id") REFERENCES "role" ("id");

ALTER TABLE "roles_menus" ADD FOREIGN KEY ("menu_id") REFERENCES "menu" ("id");

ALTER TABLE "roles_menus" ADD FOREIGN KEY ("role_id") REFERENCES "role" ("id");

ALTER TABLE "roles_permissions" ADD FOREIGN KEY ("role_id") REFERENCES "role" ("id");

ALTER TABLE "roles_permissions" ADD FOREIGN KEY ("permission_id") REFERENCES "permission" ("id");

ALTER TABLE "user" ADD FOREIGN KEY ("dept_id") REFERENCES "dept" ("id");

ALTER TABLE "user" ADD FOREIGN KEY ("job_id") REFERENCES "job" ("id");

ALTER TABLE "user" ADD FOREIGN KEY ("avatar_id") REFERENCES "user_avatar" ("id");

ALTER TABLE "users_roles" ADD FOREIGN KEY ("user_id") REFERENCES "user" ("id");

ALTER TABLE "users_roles" ADD FOREIGN KEY ("role_id") REFERENCES "role" ("id");

-- WeTune schema patches
ALTER TABLE "visits" ALTER COLUMN "date" SET NOT NULL;
ALTER TABLE "dict_detail" ALTER COLUMN "dict_id" SET NOT NULL;
ALTER TABLE "job" ALTER COLUMN "dept_id" SET NOT NULL;
ALTER TABLE "user" ALTER COLUMN "email" SET NOT NULL;
ALTER TABLE "user" ALTER COLUMN "username" SET NOT NULL;
ALTER TABLE "user" ALTER COLUMN "dept_id" SET NOT NULL;
ALTER TABLE "user" ALTER COLUMN "job_id" SET NOT NULL;
ALTER TABLE "user" ALTER COLUMN "avatar_id" SET NOT NULL;
