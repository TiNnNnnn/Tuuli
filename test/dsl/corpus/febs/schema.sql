CREATE TABLE "qrtz_blob_triggers" (
  "sched_name" VARCHAR(120) NOT NULL,
  "trigger_name" VARCHAR(200) NOT NULL,
  "trigger_group" VARCHAR(200) NOT NULL,
  "blob_data" BYTEA,
  PRIMARY KEY ("sched_name", "trigger_name", "trigger_group")
);

CREATE TABLE "qrtz_calendars" (
  "sched_name" VARCHAR(120) NOT NULL,
  "calendar_name" VARCHAR(200) NOT NULL,
  "calendar" BYTEA NOT NULL,
  PRIMARY KEY ("sched_name", "calendar_name")
);

CREATE TABLE "qrtz_cron_triggers" (
  "sched_name" VARCHAR(120) NOT NULL,
  "trigger_name" VARCHAR(200) NOT NULL,
  "trigger_group" VARCHAR(200) NOT NULL,
  "cron_expression" VARCHAR(200) NOT NULL,
  "time_zone_id" VARCHAR(80),
  PRIMARY KEY ("sched_name", "trigger_name", "trigger_group")
);

CREATE TABLE "qrtz_fired_triggers" (
  "sched_name" VARCHAR(120) NOT NULL,
  "entry_id" VARCHAR(95) NOT NULL,
  "trigger_name" VARCHAR(200) NOT NULL,
  "trigger_group" VARCHAR(200) NOT NULL,
  "instance_name" VARCHAR(200) NOT NULL,
  "fired_time" BIGINT NOT NULL,
  "sched_time" BIGINT NOT NULL,
  "priority" INTEGER NOT NULL,
  "state" VARCHAR(16) NOT NULL,
  "job_name" VARCHAR(200),
  "job_group" VARCHAR(200),
  "is_nonconcurrent" VARCHAR(1),
  "requests_recovery" VARCHAR(1),
  PRIMARY KEY ("sched_name", "entry_id")
);

CREATE TABLE "qrtz_job_details" (
  "sched_name" VARCHAR(120) NOT NULL,
  "job_name" VARCHAR(200) NOT NULL,
  "job_group" VARCHAR(200) NOT NULL,
  "description" VARCHAR(250),
  "job_class_name" VARCHAR(250) NOT NULL,
  "is_durable" VARCHAR(1) NOT NULL,
  "is_nonconcurrent" VARCHAR(1) NOT NULL,
  "is_update_data" VARCHAR(1) NOT NULL,
  "requests_recovery" VARCHAR(1) NOT NULL,
  "job_data" BYTEA,
  PRIMARY KEY ("sched_name", "job_name", "job_group")
);

CREATE TABLE "qrtz_locks" (
  "sched_name" VARCHAR(120) NOT NULL,
  "lock_name" VARCHAR(40) NOT NULL,
  PRIMARY KEY ("sched_name", "lock_name")
);

CREATE TABLE "qrtz_paused_trigger_grps" (
  "sched_name" VARCHAR(120) NOT NULL,
  "trigger_group" VARCHAR(200) NOT NULL,
  PRIMARY KEY ("sched_name", "trigger_group")
);

CREATE TABLE "qrtz_scheduler_state" (
  "sched_name" VARCHAR(120) NOT NULL,
  "instance_name" VARCHAR(200) NOT NULL,
  "last_checkin_time" BIGINT NOT NULL,
  "checkin_interval" BIGINT NOT NULL,
  PRIMARY KEY ("sched_name", "instance_name")
);

CREATE TABLE "qrtz_simple_triggers" (
  "sched_name" VARCHAR(120) NOT NULL,
  "trigger_name" VARCHAR(200) NOT NULL,
  "trigger_group" VARCHAR(200) NOT NULL,
  "repeat_count" BIGINT NOT NULL,
  "repeat_interval" BIGINT NOT NULL,
  "times_triggered" BIGINT NOT NULL,
  PRIMARY KEY ("sched_name", "trigger_name", "trigger_group")
);

CREATE TABLE "qrtz_simprop_triggers" (
  "sched_name" VARCHAR(120) NOT NULL,
  "trigger_name" VARCHAR(200) NOT NULL,
  "trigger_group" VARCHAR(200) NOT NULL,
  "str_prop_1" VARCHAR(512),
  "str_prop_2" VARCHAR(512),
  "str_prop_3" VARCHAR(512),
  "int_prop_1" INTEGER,
  "int_prop_2" INTEGER,
  "long_prop_1" BIGINT,
  "long_prop_2" BIGINT,
  "dec_prop_1" DECIMAL(13, 4),
  "dec_prop_2" DECIMAL(13, 4),
  "bool_prop_1" VARCHAR(1),
  "bool_prop_2" VARCHAR(1),
  PRIMARY KEY ("sched_name", "trigger_name", "trigger_group")
);

CREATE TABLE "qrtz_triggers" (
  "sched_name" VARCHAR(120) NOT NULL,
  "trigger_name" VARCHAR(200) NOT NULL,
  "trigger_group" VARCHAR(200) NOT NULL,
  "job_name" VARCHAR(200) NOT NULL,
  "job_group" VARCHAR(200) NOT NULL,
  "description" VARCHAR(250),
  "next_fire_time" BIGINT,
  "prev_fire_time" BIGINT,
  "priority" INTEGER,
  "trigger_state" VARCHAR(16) NOT NULL,
  "trigger_type" VARCHAR(8) NOT NULL,
  "start_time" BIGINT NOT NULL,
  "end_time" BIGINT,
  "calendar_name" VARCHAR(200),
  "misfire_instr" SMALLINT,
  "job_data" BYTEA,
  PRIMARY KEY ("sched_name", "trigger_name", "trigger_group")
);

CREATE TABLE "t_dept" (
  "dept_id" BIGINT NOT NULL,
  "parent_id" BIGINT NOT NULL,
  "dept_name" VARCHAR(100) NOT NULL,
  "order_num" BIGINT,
  "create_time" TIMESTAMP,
  "modify_time" TIMESTAMP,
  PRIMARY KEY ("dept_id")
);

CREATE TABLE "t_eximport" (
  "field1" VARCHAR(20) NOT NULL,
  "field2" INTEGER NOT NULL,
  "field3" VARCHAR(100) NOT NULL,
  "create_time" TIMESTAMP NOT NULL
);

CREATE TABLE "t_generator_config" (
  "id" INTEGER NOT NULL,
  "author" VARCHAR(20) NOT NULL,
  "base_package" VARCHAR(50) NOT NULL,
  "entity_package" VARCHAR(20) NOT NULL,
  "mapper_package" VARCHAR(20) NOT NULL,
  "mapper_xml_package" VARCHAR(20) NOT NULL,
  "service_package" VARCHAR(20) NOT NULL,
  "service_impl_package" VARCHAR(20) NOT NULL,
  "controller_package" VARCHAR(20) NOT NULL,
  "is_trim" CHAR(1) NOT NULL,
  "trim_value" VARCHAR(10),
  PRIMARY KEY ("id")
);

CREATE TABLE "t_job" (
  "job_id" BIGINT NOT NULL,
  "bean_name" VARCHAR(50) NOT NULL,
  "method_name" VARCHAR(50) NOT NULL,
  "params" VARCHAR(50),
  "cron_expression" VARCHAR(20) NOT NULL,
  "status" CHAR(2) NOT NULL,
  "remark" VARCHAR(50),
  "create_time" TIMESTAMP,
  PRIMARY KEY ("job_id")
);

CREATE TABLE "t_job_log" (
  "log_id" BIGINT NOT NULL,
  "job_id" BIGINT NOT NULL,
  "bean_name" VARCHAR(100) NOT NULL,
  "method_name" VARCHAR(100) NOT NULL,
  "params" VARCHAR(200),
  "status" CHAR(2) NOT NULL,
  "error" TEXT,
  "times" DECIMAL(11, 0),
  "create_time" TIMESTAMP,
  PRIMARY KEY ("log_id")
);

CREATE TABLE "t_log" (
  "id" BIGINT NOT NULL,
  "username" VARCHAR(50),
  "operation" TEXT,
  "time" DECIMAL(11, 0),
  "method" TEXT,
  "params" TEXT,
  "ip" VARCHAR(64),
  "create_time" TIMESTAMP,
  "location" VARCHAR(50),
  PRIMARY KEY ("id")
);

CREATE TABLE "t_login_log" (
  "id" BIGINT NOT NULL,
  "username" VARCHAR(50) NOT NULL,
  "login_time" TIMESTAMP NOT NULL,
  "location" VARCHAR(50),
  "ip" VARCHAR(50),
  "system" VARCHAR(50),
  "browser" VARCHAR(50),
  PRIMARY KEY ("id")
);

CREATE TABLE "t_menu" (
  "menu_id" BIGINT NOT NULL,
  "parent_id" BIGINT NOT NULL,
  "menu_name" VARCHAR(50) NOT NULL,
  "url" VARCHAR(50),
  "perms" TEXT,
  "icon" VARCHAR(50),
  "type" CHAR(2) NOT NULL,
  "order_num" BIGINT,
  "create_time" TIMESTAMP NOT NULL,
  "modify_time" TIMESTAMP,
  PRIMARY KEY ("menu_id")
);

CREATE TABLE "t_role" (
  "role_id" BIGINT NOT NULL,
  "role_name" VARCHAR(100) NOT NULL,
  "remark" VARCHAR(100),
  "create_time" TIMESTAMP NOT NULL,
  "modify_time" TIMESTAMP,
  PRIMARY KEY ("role_id")
);

CREATE TABLE "t_role_menu" (
  "role_id" BIGINT NOT NULL,
  "menu_id" BIGINT NOT NULL
);

CREATE TABLE "t_user" (
  "user_id" BIGINT NOT NULL,
  "username" VARCHAR(50) NOT NULL,
  "password" VARCHAR(128) NOT NULL,
  "dept_id" BIGINT,
  "email" VARCHAR(128),
  "mobile" VARCHAR(20),
  "status" CHAR(1) NOT NULL,
  "create_time" TIMESTAMP NOT NULL,
  "modify_time" TIMESTAMP,
  "last_login_time" TIMESTAMP,
  "ssex" CHAR(1),
  "is_tab" CHAR(1),
  "theme" VARCHAR(10),
  "avatar" VARCHAR(100),
  "description" VARCHAR(100),
  PRIMARY KEY ("user_id")
);

CREATE TABLE "t_user_role" (
  "user_id" BIGINT NOT NULL,
  "role_id" BIGINT NOT NULL
);

ALTER TABLE "qrtz_blob_triggers" ADD FOREIGN KEY ("sched_name", "trigger_name", "trigger_group") REFERENCES "qrtz_triggers" ("sched_name", "trigger_name", "trigger_group");

ALTER TABLE "qrtz_cron_triggers" ADD FOREIGN KEY ("sched_name", "trigger_name", "trigger_group") REFERENCES "qrtz_triggers" ("sched_name", "trigger_name", "trigger_group");

ALTER TABLE "qrtz_simple_triggers" ADD FOREIGN KEY ("sched_name", "trigger_name", "trigger_group") REFERENCES "qrtz_triggers" ("sched_name", "trigger_name", "trigger_group");

ALTER TABLE "qrtz_simprop_triggers" ADD FOREIGN KEY ("sched_name", "trigger_name", "trigger_group") REFERENCES "qrtz_triggers" ("sched_name", "trigger_name", "trigger_group");

ALTER TABLE "qrtz_triggers" ADD FOREIGN KEY ("sched_name", "job_name", "job_group") REFERENCES "qrtz_job_details" ("sched_name", "job_name", "job_group");

-- WeTune schema patches
ALTER TABLE "t_role_menu" ADD CONSTRAINT "wetune_u_6bb15fe7d9d7c2bc" UNIQUE ("role_id", "menu_id");
ALTER TABLE "t_user_role" ADD CONSTRAINT "wetune_u_ecc05f2e19577bc2" UNIQUE ("user_id", "role_id");
ALTER TABLE "t_user_role" ADD CONSTRAINT "wetune_fk_5e7a73f614128b44" FOREIGN KEY ("user_id") REFERENCES "t_user" ("user_id");
ALTER TABLE "t_role_menu" ADD CONSTRAINT "wetune_fk_4ae6d2d15fb9dbb3" FOREIGN KEY ("menu_id") REFERENCES "t_menu" ("menu_id");
ALTER TABLE "t_user" ADD CONSTRAINT "wetune_fk_9da60682ce0cb013" FOREIGN KEY ("dept_id") REFERENCES "t_dept" ("dept_id");
ALTER TABLE "t_user_role" ADD CONSTRAINT "wetune_fk_00496af26ce81fd1" FOREIGN KEY ("role_id") REFERENCES "t_role" ("role_id");
ALTER TABLE "t_role_menu" ADD CONSTRAINT "wetune_fk_6a72c42b9494c8de" FOREIGN KEY ("role_id") REFERENCES "t_role" ("role_id");
