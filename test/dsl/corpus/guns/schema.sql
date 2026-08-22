CREATE TABLE "sys_dept" (
  "dept_id" BIGINT NOT NULL,
  "pid" BIGINT,
  "pids" VARCHAR(512),
  "simple_name" VARCHAR(45),
  "full_name" VARCHAR(255),
  "description" VARCHAR(255),
  "version" INTEGER,
  "sort" INTEGER,
  "create_time" TIMESTAMP,
  "update_time" TIMESTAMP,
  "create_user" BIGINT,
  "update_user" BIGINT,
  PRIMARY KEY ("dept_id")
);

CREATE TABLE "sys_dict" (
  "dict_id" BIGINT NOT NULL,
  "dict_type_id" BIGINT NOT NULL,
  "code" VARCHAR(50) NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "parent_id" BIGINT NOT NULL,
  "parent_ids" VARCHAR(255),
  "status" VARCHAR(10) NOT NULL,
  "sort" INTEGER,
  "description" VARCHAR(1000),
  "create_time" TIMESTAMP,
  "update_time" TIMESTAMP,
  "create_user" BIGINT,
  "update_user" BIGINT,
  PRIMARY KEY ("dict_id")
);

CREATE TABLE "sys_dict_type" (
  "dict_type_id" BIGINT NOT NULL,
  "code" VARCHAR(255) NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "description" VARCHAR(1000),
  "system_flag" CHAR(1) NOT NULL,
  "status" VARCHAR(10) NOT NULL,
  "sort" INTEGER,
  "create_time" TIMESTAMP,
  "create_user" BIGINT,
  "update_time" TIMESTAMP,
  "update_user" BIGINT,
  PRIMARY KEY ("dict_type_id")
);

CREATE TABLE "sys_file_info" (
  "file_id" VARCHAR(50) NOT NULL,
  "file_data" TEXT,
  "create_time" TIMESTAMP,
  "update_time" TIMESTAMP,
  "create_user" BIGINT,
  "update_user" BIGINT,
  PRIMARY KEY ("file_id")
);

CREATE TABLE "sys_login_log" (
  "login_log_id" BIGINT NOT NULL,
  "log_name" VARCHAR(255),
  "user_id" BIGINT,
  "create_time" TIMESTAMP,
  "succeed" VARCHAR(255),
  "message" TEXT,
  "ip_address" VARCHAR(255),
  PRIMARY KEY ("login_log_id")
);

CREATE TABLE "sys_menu" (
  "menu_id" BIGINT NOT NULL,
  "code" VARCHAR(255),
  "pcode" VARCHAR(255),
  "pcodes" VARCHAR(255),
  "name" VARCHAR(255),
  "icon" VARCHAR(255),
  "url" VARCHAR(255),
  "sort" INTEGER,
  "levels" INTEGER,
  "menu_flag" VARCHAR(32),
  "description" VARCHAR(255),
  "status" VARCHAR(32),
  "new_page_flag" VARCHAR(32),
  "open_flag" VARCHAR(32),
  "create_time" TIMESTAMP,
  "update_time" TIMESTAMP,
  "create_user" BIGINT,
  "update_user" BIGINT,
  PRIMARY KEY ("menu_id")
);

CREATE TABLE "sys_notice" (
  "notice_id" BIGINT NOT NULL,
  "title" VARCHAR(255),
  "content" TEXT,
  "create_time" TIMESTAMP,
  "create_user" BIGINT,
  "update_time" TIMESTAMP,
  "update_user" BIGINT,
  PRIMARY KEY ("notice_id")
);

CREATE TABLE "sys_operation_log" (
  "operation_log_id" BIGINT NOT NULL,
  "log_type" VARCHAR(32),
  "log_name" VARCHAR(255),
  "user_id" BIGINT,
  "class_name" VARCHAR(255),
  "method" TEXT,
  "create_time" TIMESTAMP,
  "succeed" VARCHAR(32),
  "message" TEXT,
  PRIMARY KEY ("operation_log_id")
);

CREATE TABLE "sys_relation" (
  "relation_id" BIGINT NOT NULL,
  "menu_id" BIGINT,
  "role_id" BIGINT,
  PRIMARY KEY ("relation_id")
);

CREATE TABLE "sys_role" (
  "role_id" BIGINT NOT NULL,
  "pid" BIGINT,
  "name" VARCHAR(255),
  "description" VARCHAR(255),
  "sort" INTEGER,
  "version" INTEGER,
  "create_time" TIMESTAMP,
  "update_time" TIMESTAMP,
  "create_user" BIGINT,
  "update_user" BIGINT,
  PRIMARY KEY ("role_id")
);

CREATE TABLE "sys_user" (
  "user_id" BIGINT NOT NULL,
  "avatar" VARCHAR(255),
  "account" VARCHAR(45),
  "password" VARCHAR(45),
  "salt" VARCHAR(45),
  "name" VARCHAR(45),
  "birthday" TIMESTAMP,
  "sex" VARCHAR(32),
  "email" VARCHAR(45),
  "phone" VARCHAR(45),
  "role_id" VARCHAR(255),
  "dept_id" BIGINT,
  "status" VARCHAR(32),
  "create_time" TIMESTAMP,
  "create_user" BIGINT,
  "update_time" TIMESTAMP,
  "update_user" BIGINT,
  "version" INTEGER,
  PRIMARY KEY ("user_id")
);

-- WeTune schema patches
ALTER TABLE "sys_menu" ADD CONSTRAINT "wetune_u_d22a7f02ca9ccba2" UNIQUE ("code");
ALTER TABLE "sys_relation" ADD CONSTRAINT "wetune_fk_9ecb4d8cb171068a" FOREIGN KEY ("menu_id") REFERENCES "sys_menu" ("menu_id");
