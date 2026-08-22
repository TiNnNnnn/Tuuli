CREATE TABLE "ar_internal_metadata" (
  "key" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255),
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("key")
);

CREATE TABLE "attachments" (
  "id" INTEGER NOT NULL,
  "container_id" INTEGER,
  "container_type" VARCHAR(30),
  "filename" VARCHAR(255) NOT NULL,
  "disk_filename" VARCHAR(255) NOT NULL,
  "filesize" BIGINT NOT NULL,
  "content_type" VARCHAR(255),
  "digest" VARCHAR(64) NOT NULL,
  "downloads" INTEGER NOT NULL,
  "author_id" INTEGER NOT NULL,
  "created_on" TIMESTAMP NOT NULL,
  "description" VARCHAR(255),
  "disk_directory" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "auth_sources" (
  "id" INTEGER NOT NULL,
  "type" VARCHAR(30) NOT NULL,
  "name" VARCHAR(60) NOT NULL,
  "host" VARCHAR(60),
  "port" INTEGER,
  "account" VARCHAR(255),
  "account_password" VARCHAR(255),
  "base_dn" VARCHAR(255),
  "attr_login" VARCHAR(30),
  "attr_firstname" VARCHAR(30),
  "attr_lastname" VARCHAR(30),
  "attr_mail" VARCHAR(30),
  "onthefly_register" SMALLINT NOT NULL,
  "tls" SMALLINT NOT NULL,
  "filter" TEXT,
  "timeout" INTEGER,
  "verify_peer" SMALLINT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "boards" (
  "id" INTEGER NOT NULL,
  "project_id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "description" VARCHAR(255),
  "position" INTEGER,
  "topics_count" INTEGER NOT NULL,
  "messages_count" INTEGER NOT NULL,
  "last_message_id" INTEGER,
  "parent_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "changes" (
  "id" INTEGER NOT NULL,
  "changeset_id" INTEGER NOT NULL,
  "action" VARCHAR(1) NOT NULL,
  "path" TEXT NOT NULL,
  "from_path" TEXT,
  "from_revision" VARCHAR(255),
  "revision" VARCHAR(255),
  "branch" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "changeset_parents" (
  "changeset_id" INTEGER NOT NULL,
  "parent_id" INTEGER NOT NULL
);

CREATE TABLE "changesets" (
  "id" INTEGER NOT NULL,
  "repository_id" INTEGER NOT NULL,
  "revision" VARCHAR(255) NOT NULL,
  "committer" VARCHAR(255),
  "committed_on" TIMESTAMP NOT NULL,
  "comments" TEXT,
  "commit_date" DATE,
  "scmid" VARCHAR(255),
  "user_id" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("repository_id", "revision")
);

CREATE TABLE "changesets_issues" (
  "changeset_id" INTEGER NOT NULL,
  "issue_id" INTEGER NOT NULL,
  UNIQUE ("changeset_id", "issue_id")
);

CREATE TABLE "comments" (
  "id" INTEGER NOT NULL,
  "commented_type" VARCHAR(30) NOT NULL,
  "commented_id" INTEGER NOT NULL,
  "author_id" INTEGER NOT NULL,
  "content" TEXT,
  "created_on" TIMESTAMP NOT NULL,
  "updated_on" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "custom_field_enumerations" (
  "id" INTEGER NOT NULL,
  "custom_field_id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "active" SMALLINT NOT NULL,
  "position" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "custom_fields" (
  "id" INTEGER NOT NULL,
  "type" VARCHAR(30) NOT NULL,
  "name" VARCHAR(30) NOT NULL,
  "field_format" VARCHAR(30) NOT NULL,
  "possible_values" TEXT,
  "regexp" VARCHAR(255),
  "min_length" INTEGER,
  "max_length" INTEGER,
  "is_required" SMALLINT NOT NULL,
  "is_for_all" SMALLINT NOT NULL,
  "is_filter" SMALLINT NOT NULL,
  "position" INTEGER,
  "searchable" SMALLINT,
  "default_value" TEXT,
  "editable" SMALLINT,
  "visible" SMALLINT NOT NULL,
  "multiple" SMALLINT,
  "format_store" TEXT,
  "description" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "custom_fields_projects" (
  "custom_field_id" INTEGER NOT NULL,
  "project_id" INTEGER NOT NULL,
  UNIQUE ("custom_field_id", "project_id")
);

CREATE TABLE "custom_fields_roles" (
  "custom_field_id" INTEGER NOT NULL,
  "role_id" INTEGER NOT NULL,
  UNIQUE ("custom_field_id", "role_id")
);

CREATE TABLE "custom_fields_trackers" (
  "custom_field_id" INTEGER NOT NULL,
  "tracker_id" INTEGER NOT NULL,
  UNIQUE ("custom_field_id", "tracker_id")
);

CREATE TABLE "custom_values" (
  "id" INTEGER NOT NULL,
  "customized_type" VARCHAR(30) NOT NULL,
  "customized_id" INTEGER NOT NULL,
  "custom_field_id" INTEGER NOT NULL,
  "value" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "documents" (
  "id" INTEGER NOT NULL,
  "project_id" INTEGER NOT NULL,
  "category_id" INTEGER NOT NULL,
  "title" VARCHAR(255) NOT NULL,
  "description" TEXT,
  "created_on" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "email_addresses" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "address" VARCHAR(255) NOT NULL,
  "is_default" SMALLINT NOT NULL,
  "notify" SMALLINT NOT NULL,
  "created_on" TIMESTAMP NOT NULL,
  "updated_on" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "enabled_modules" (
  "id" INTEGER NOT NULL,
  "project_id" INTEGER,
  "name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "enumerations" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(30) NOT NULL,
  "position" INTEGER,
  "is_default" SMALLINT NOT NULL,
  "type" VARCHAR(255),
  "active" SMALLINT NOT NULL,
  "project_id" INTEGER,
  "parent_id" INTEGER,
  "position_name" VARCHAR(30),
  PRIMARY KEY ("id")
);

CREATE TABLE "groups_users" (
  "group_id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  UNIQUE ("group_id", "user_id")
);

CREATE TABLE "import_items" (
  "id" INTEGER NOT NULL,
  "import_id" INTEGER NOT NULL,
  "position" INTEGER NOT NULL,
  "obj_id" INTEGER,
  "message" TEXT,
  "unique_id" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "imports" (
  "id" INTEGER NOT NULL,
  "type" VARCHAR(255),
  "user_id" INTEGER NOT NULL,
  "filename" VARCHAR(255),
  "settings" TEXT,
  "total_items" INTEGER,
  "finished" SMALLINT NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "issue_categories" (
  "id" INTEGER NOT NULL,
  "project_id" INTEGER NOT NULL,
  "name" VARCHAR(60) NOT NULL,
  "assigned_to_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "issue_relations" (
  "id" INTEGER NOT NULL,
  "issue_from_id" INTEGER NOT NULL,
  "issue_to_id" INTEGER NOT NULL,
  "relation_type" VARCHAR(255) NOT NULL,
  "delay" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("issue_from_id", "issue_to_id")
);

CREATE TABLE "issue_statuses" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(30) NOT NULL,
  "is_closed" SMALLINT NOT NULL,
  "position" INTEGER,
  "default_done_ratio" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "issues" (
  "id" INTEGER NOT NULL,
  "tracker_id" INTEGER NOT NULL,
  "project_id" INTEGER NOT NULL,
  "subject" VARCHAR(255) NOT NULL,
  "description" TEXT,
  "due_date" DATE,
  "category_id" INTEGER,
  "status_id" INTEGER NOT NULL,
  "assigned_to_id" INTEGER,
  "priority_id" INTEGER NOT NULL,
  "fixed_version_id" INTEGER,
  "author_id" INTEGER NOT NULL,
  "lock_version" INTEGER NOT NULL,
  "created_on" TIMESTAMP NOT NULL,
  "updated_on" TIMESTAMP NOT NULL,
  "start_date" DATE,
  "done_ratio" INTEGER NOT NULL,
  "estimated_hours" REAL,
  "parent_id" INTEGER,
  "root_id" INTEGER,
  "lft" INTEGER,
  "rgt" INTEGER,
  "is_private" SMALLINT NOT NULL,
  "closed_on" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "journal_details" (
  "id" INTEGER NOT NULL,
  "journal_id" INTEGER NOT NULL,
  "property" VARCHAR(30) NOT NULL,
  "prop_key" VARCHAR(30) NOT NULL,
  "old_value" TEXT,
  "value" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "journals" (
  "id" INTEGER NOT NULL,
  "journalized_id" INTEGER NOT NULL,
  "journalized_type" VARCHAR(30) NOT NULL,
  "user_id" INTEGER NOT NULL,
  "notes" TEXT,
  "created_on" TIMESTAMP NOT NULL,
  "private_notes" SMALLINT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "member_roles" (
  "id" INTEGER NOT NULL,
  "member_id" INTEGER NOT NULL,
  "role_id" INTEGER NOT NULL,
  "inherited_from" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "members" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "project_id" INTEGER NOT NULL,
  "created_on" TIMESTAMP NOT NULL,
  "mail_notification" SMALLINT NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "project_id")
);

CREATE TABLE "messages" (
  "id" INTEGER NOT NULL,
  "board_id" INTEGER NOT NULL,
  "parent_id" INTEGER,
  "subject" VARCHAR(255) NOT NULL,
  "content" TEXT,
  "author_id" INTEGER,
  "replies_count" INTEGER NOT NULL,
  "last_reply_id" INTEGER,
  "created_on" TIMESTAMP NOT NULL,
  "updated_on" TIMESTAMP NOT NULL,
  "locked" SMALLINT,
  "sticky" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "news" (
  "id" INTEGER NOT NULL,
  "project_id" INTEGER,
  "title" VARCHAR(60) NOT NULL,
  "summary" VARCHAR(255),
  "description" TEXT,
  "author_id" INTEGER NOT NULL,
  "created_on" TIMESTAMP NOT NULL,
  "comments_count" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "open_id_authentication_associations" (
  "id" INTEGER NOT NULL,
  "issued" INTEGER,
  "lifetime" INTEGER,
  "handle" VARCHAR(255),
  "assoc_type" VARCHAR(255),
  "server_url" BYTEA,
  "secret" BYTEA,
  PRIMARY KEY ("id")
);

CREATE TABLE "open_id_authentication_nonces" (
  "id" INTEGER NOT NULL,
  "timestamp" INTEGER NOT NULL,
  "server_url" VARCHAR(255),
  "salt" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "projects" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "description" TEXT,
  "homepage" VARCHAR(255),
  "is_public" SMALLINT NOT NULL,
  "parent_id" INTEGER,
  "created_on" TIMESTAMP NOT NULL,
  "updated_on" TIMESTAMP NOT NULL,
  "identifier" VARCHAR(255),
  "status" INTEGER NOT NULL,
  "lft" INTEGER,
  "rgt" INTEGER,
  "inherit_members" SMALLINT NOT NULL,
  "default_version_id" INTEGER,
  "default_assigned_to_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "projects_trackers" (
  "project_id" INTEGER NOT NULL,
  "tracker_id" INTEGER NOT NULL,
  UNIQUE ("project_id", "tracker_id")
);

CREATE TABLE "queries" (
  "id" INTEGER NOT NULL,
  "project_id" INTEGER,
  "name" VARCHAR(255) NOT NULL,
  "filters" TEXT,
  "user_id" INTEGER NOT NULL,
  "column_names" TEXT,
  "sort_criteria" TEXT,
  "group_by" VARCHAR(255),
  "type" VARCHAR(255),
  "visibility" INTEGER,
  "options" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "queries_roles" (
  "query_id" INTEGER NOT NULL,
  "role_id" INTEGER NOT NULL,
  UNIQUE ("query_id", "role_id")
);

CREATE TABLE "repositories" (
  "id" INTEGER NOT NULL,
  "project_id" INTEGER NOT NULL,
  "url" VARCHAR(255) NOT NULL,
  "login" VARCHAR(60),
  "password" VARCHAR(255),
  "root_url" VARCHAR(255),
  "type" VARCHAR(255),
  "path_encoding" VARCHAR(64),
  "log_encoding" VARCHAR(64),
  "extra_info" TEXT,
  "identifier" VARCHAR(255),
  "is_default" SMALLINT,
  "created_on" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "roles" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "position" INTEGER,
  "assignable" SMALLINT,
  "builtin" INTEGER NOT NULL,
  "permissions" TEXT,
  "issues_visibility" VARCHAR(30) NOT NULL,
  "users_visibility" VARCHAR(30) NOT NULL,
  "time_entries_visibility" VARCHAR(30) NOT NULL,
  "all_roles_managed" SMALLINT NOT NULL,
  "settings" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "roles_managed_roles" (
  "role_id" INTEGER NOT NULL,
  "managed_role_id" INTEGER NOT NULL,
  UNIQUE ("role_id", "managed_role_id")
);

CREATE TABLE "schema_migrations" (
  "version" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("version")
);

CREATE TABLE "settings" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "value" TEXT,
  "updated_on" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "time_entries" (
  "id" INTEGER NOT NULL,
  "project_id" INTEGER NOT NULL,
  "author_id" INTEGER,
  "user_id" INTEGER NOT NULL,
  "issue_id" INTEGER,
  "hours" REAL NOT NULL,
  "comments" VARCHAR(1024),
  "activity_id" INTEGER NOT NULL,
  "spent_on" DATE NOT NULL,
  "tyear" INTEGER NOT NULL,
  "tmonth" INTEGER NOT NULL,
  "tweek" INTEGER NOT NULL,
  "created_on" TIMESTAMP NOT NULL,
  "updated_on" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "tokens" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "action" VARCHAR(30) NOT NULL,
  "value" VARCHAR(40) NOT NULL,
  "created_on" TIMESTAMP NOT NULL,
  "updated_on" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("value")
);

CREATE TABLE "trackers" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(30) NOT NULL,
  "description" VARCHAR(255),
  "is_in_chlog" SMALLINT NOT NULL,
  "position" INTEGER,
  "is_in_roadmap" SMALLINT NOT NULL,
  "fields_bits" INTEGER,
  "default_status_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "user_preferences" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "others" TEXT,
  "hide_mail" SMALLINT,
  "time_zone" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "users" (
  "id" INTEGER NOT NULL,
  "login" VARCHAR(255) NOT NULL,
  "hashed_password" VARCHAR(40) NOT NULL,
  "firstname" VARCHAR(30) NOT NULL,
  "lastname" VARCHAR(255) NOT NULL,
  "admin" SMALLINT NOT NULL,
  "status" INTEGER NOT NULL,
  "last_login_on" TIMESTAMP,
  "language" VARCHAR(5),
  "auth_source_id" INTEGER,
  "created_on" TIMESTAMP NOT NULL,
  "updated_on" TIMESTAMP NOT NULL,
  "type" VARCHAR(255),
  "identity_url" VARCHAR(255),
  "mail_notification" VARCHAR(255) NOT NULL,
  "salt" VARCHAR(64),
  "must_change_passwd" SMALLINT NOT NULL,
  "passwd_changed_on" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "versions" (
  "id" INTEGER NOT NULL,
  "project_id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "description" VARCHAR(255),
  "effective_date" DATE,
  "created_on" TIMESTAMP NOT NULL,
  "updated_on" TIMESTAMP NOT NULL,
  "wiki_page_title" VARCHAR(255),
  "status" VARCHAR(255),
  "sharing" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "watchers" (
  "id" INTEGER NOT NULL,
  "watchable_type" VARCHAR(255) NOT NULL,
  "watchable_id" INTEGER NOT NULL,
  "user_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "wiki_content_versions" (
  "id" INTEGER NOT NULL,
  "wiki_content_id" INTEGER NOT NULL,
  "page_id" INTEGER NOT NULL,
  "author_id" INTEGER,
  "data" BYTEA,
  "compression" VARCHAR(6),
  "comments" VARCHAR(1024),
  "updated_on" TIMESTAMP NOT NULL,
  "version" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "wiki_contents" (
  "id" INTEGER NOT NULL,
  "page_id" INTEGER NOT NULL,
  "author_id" INTEGER,
  "text" TEXT,
  "comments" VARCHAR(1024),
  "updated_on" TIMESTAMP NOT NULL,
  "version" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "wiki_pages" (
  "id" INTEGER NOT NULL,
  "wiki_id" INTEGER NOT NULL,
  "title" VARCHAR(255) NOT NULL,
  "created_on" TIMESTAMP NOT NULL,
  "protected" SMALLINT NOT NULL,
  "parent_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "wiki_redirects" (
  "id" INTEGER NOT NULL,
  "wiki_id" INTEGER NOT NULL,
  "title" VARCHAR(255),
  "redirects_to" VARCHAR(255),
  "created_on" TIMESTAMP NOT NULL,
  "redirects_to_wiki_id" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "wikis" (
  "id" INTEGER NOT NULL,
  "project_id" INTEGER NOT NULL,
  "start_page" VARCHAR(255) NOT NULL,
  "status" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "workflows" (
  "id" INTEGER NOT NULL,
  "tracker_id" INTEGER NOT NULL,
  "old_status_id" INTEGER NOT NULL,
  "new_status_id" INTEGER NOT NULL,
  "role_id" INTEGER NOT NULL,
  "assignee" SMALLINT NOT NULL,
  "author" SMALLINT NOT NULL,
  "type" VARCHAR(30),
  "field_name" VARCHAR(30),
  "rule" VARCHAR(30),
  PRIMARY KEY ("id")
);

-- WeTune schema patches
ALTER TABLE "watchers" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "lft" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "rgt" SET NOT NULL;
ALTER TABLE "documents" ALTER COLUMN "created_on" SET NOT NULL;
ALTER TABLE "enumerations" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "enumerations" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "category_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "assigned_to_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "fixed_version_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "created_on" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "root_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "lft" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "rgt" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "member_roles" ALTER COLUMN "inherited_from" SET NOT NULL;
ALTER TABLE "import_items" ALTER COLUMN "unique_id" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "auth_source_id" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "messages" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "messages" ALTER COLUMN "last_reply_id" SET NOT NULL;
ALTER TABLE "messages" ALTER COLUMN "author_id" SET NOT NULL;
ALTER TABLE "wiki_pages" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "attachments" ALTER COLUMN "created_on" SET NOT NULL;
ALTER TABLE "attachments" ALTER COLUMN "container_id" SET NOT NULL;
ALTER TABLE "attachments" ALTER COLUMN "container_type" SET NOT NULL;
ALTER TABLE "wiki_contents" ALTER COLUMN "author_id" SET NOT NULL;
ALTER TABLE "boards" ALTER COLUMN "last_message_id" SET NOT NULL;
ALTER TABLE "wiki_redirects" ALTER COLUMN "title" SET NOT NULL;
ALTER TABLE "issue_statuses" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "time_entries" ALTER COLUMN "issue_id" SET NOT NULL;
ALTER TABLE "news" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "news" ALTER COLUMN "created_on" SET NOT NULL;
ALTER TABLE "enabled_modules" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "issue_categories" ALTER COLUMN "assigned_to_id" SET NOT NULL;
ALTER TABLE "queries" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "changesets" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "changesets" ALTER COLUMN "scmid" SET NOT NULL;
ALTER TABLE "watchers" ADD CONSTRAINT "wetune_u_92611aedc1f312e7" UNIQUE ("watchable_type", "watchable_id", "user_id");
ALTER TABLE "changeset_parents" ADD CONSTRAINT "wetune_u_8b86c069bb422202" UNIQUE ("changeset_id", "parent_id");
ALTER TABLE "enabled_modules" ADD CONSTRAINT "wetune_fk_2c873532a1009d39" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "issues" ADD CONSTRAINT "wetune_fk_a0905674aef8a59f" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "journals" ADD CONSTRAINT "wetune_fk_5f9b9d4a911c6eb5" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "attachments" ADD CONSTRAINT "wetune_fk_fdda4b893b63ec86" FOREIGN KEY ("container_id") REFERENCES "messages" ("id");
ALTER TABLE "documents" ADD CONSTRAINT "wetune_fk_d21a5f77ea6e3522" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "issue_relations" ADD CONSTRAINT "wetune_fk_f56cc9ff9f5d5aa7" FOREIGN KEY ("issue_from_id") REFERENCES "issues" ("id");
ALTER TABLE "changes" ADD CONSTRAINT "wetune_fk_eae9b8752b9b4b1a" FOREIGN KEY ("changeset_id") REFERENCES "changesets" ("id");
ALTER TABLE "member_roles" ADD CONSTRAINT "wetune_fk_7d41d4b26bdb4bb9" FOREIGN KEY ("role_id") REFERENCES "roles" ("id");
ALTER TABLE "time_entries" ADD CONSTRAINT "wetune_fk_35fb8813d8128e08" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "issues" ADD CONSTRAINT "wetune_fk_033f351cf6d7b53b" FOREIGN KEY ("fixed_version_id") REFERENCES "versions" ("id");
ALTER TABLE "changesets_issues" ADD CONSTRAINT "wetune_fk_ebb6ee30a8e6c7e4" FOREIGN KEY ("issue_id") REFERENCES "issues" ("id");
ALTER TABLE "members" ADD CONSTRAINT "wetune_fk_d78a4e0c8ead4d82" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "wiki_contents" ADD CONSTRAINT "wetune_fk_33c0bc443c738b57" FOREIGN KEY ("page_id") REFERENCES "wiki_pages" ("id");
ALTER TABLE "changeset_parents" ADD CONSTRAINT "wetune_fk_ff969318225801fb" FOREIGN KEY ("parent_id") REFERENCES "changesets" ("id");
ALTER TABLE "issues" ADD CONSTRAINT "wetune_fk_bf2745c6884e6130" FOREIGN KEY ("category_id") REFERENCES "issue_categories" ("id");
ALTER TABLE "issues" ADD CONSTRAINT "wetune_fk_bc9c9ced85d1c385" FOREIGN KEY ("status_id") REFERENCES "issue_statuses" ("id");
ALTER TABLE "news" ADD CONSTRAINT "wetune_fk_3fd8c5812ba7047c" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "queries" ADD CONSTRAINT "wetune_fk_1f2b00ed3b9e54f3" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "custom_values" ADD CONSTRAINT "wetune_fk_d7c9ba460c7ea5b4" FOREIGN KEY ("customized_id") REFERENCES "issues" ("id");
ALTER TABLE "member_roles" ADD CONSTRAINT "wetune_fk_41106c4bbdccc1d2" FOREIGN KEY ("member_id") REFERENCES "members" ("id");
ALTER TABLE "messages" ADD CONSTRAINT "wetune_fk_a101d67caddd905f" FOREIGN KEY ("board_id") REFERENCES "boards" ("id");
ALTER TABLE "issues" ADD CONSTRAINT "wetune_fk_59a3c216aea25eb6" FOREIGN KEY ("priority_id") REFERENCES "enumerations" ("id");
ALTER TABLE "news" ADD CONSTRAINT "wetune_fk_a3f37e64c7552786" FOREIGN KEY ("author_id") REFERENCES "users" ("id");
ALTER TABLE "changesets" ADD CONSTRAINT "wetune_fk_0fae31abcb9dbce0" FOREIGN KEY ("repository_id") REFERENCES "repositories" ("id");
ALTER TABLE "groups_users" ADD CONSTRAINT "wetune_fk_4c28ef1f3aa39cd3" FOREIGN KEY ("group_id") REFERENCES "users" ("id");
ALTER TABLE "versions" ADD CONSTRAINT "wetune_fk_ee9b80f3f8f941bd" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "queries_roles" ADD CONSTRAINT "wetune_fk_43524408720ee7e7" FOREIGN KEY ("query_id") REFERENCES "queries" ("id");
ALTER TABLE "projects_trackers" ADD CONSTRAINT "wetune_fk_384ee5b75689df9c" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "wiki_pages" ADD CONSTRAINT "wetune_fk_3150532cda890363" FOREIGN KEY ("wiki_id") REFERENCES "wikis" ("id");
ALTER TABLE "boards" ADD CONSTRAINT "wetune_fk_2f05621d4a1a78dd" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "issues" ADD CONSTRAINT "wetune_fk_309d5b84f4dd02fc" FOREIGN KEY ("author_id") REFERENCES "users" ("id");
ALTER TABLE "wikis" ADD CONSTRAINT "wetune_fk_18a916620155dadf" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "journals" ADD CONSTRAINT "wetune_fk_e4362c683e655e50" FOREIGN KEY ("journalized_id") REFERENCES "issues" ("id");
ALTER TABLE "workflows" ADD CONSTRAINT "wetune_fk_5195bcdf86974e06" FOREIGN KEY ("new_status_id") REFERENCES "issue_statuses" ("id");
ALTER TABLE "custom_fields_projects" ADD CONSTRAINT "wetune_fk_a50080fc39f8ff39" FOREIGN KEY ("custom_field_id") REFERENCES "custom_fields" ("id");
ALTER TABLE "email_addresses" ADD CONSTRAINT "wetune_fk_3eefffc05bdbde80" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "custom_fields_roles" ADD CONSTRAINT "wetune_fk_6a46495075653f66" FOREIGN KEY ("custom_field_id") REFERENCES "custom_fields" ("id");
ALTER TABLE "custom_fields_trackers" ADD CONSTRAINT "wetune_fk_f56188fdbe5dab8c" FOREIGN KEY ("custom_field_id") REFERENCES "custom_fields" ("id");
ALTER TABLE "roles_managed_roles" ADD CONSTRAINT "wetune_fk_1dabe7b8b881b6ec" FOREIGN KEY ("managed_role_id") REFERENCES "roles" ("id");
ALTER TABLE "members" ADD CONSTRAINT "wetune_fk_cbabbab3f48d5335" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "custom_fields_roles" ADD CONSTRAINT "wetune_fk_b24ea043aff0cbe4" FOREIGN KEY ("role_id") REFERENCES "roles" ("id");
ALTER TABLE "time_entries" ADD CONSTRAINT "wetune_fk_3cecaf823f8425eb" FOREIGN KEY ("activity_id") REFERENCES "enumerations" ("id");
ALTER TABLE "custom_fields_trackers" ADD CONSTRAINT "wetune_fk_0c89f5c558a95592" FOREIGN KEY ("tracker_id") REFERENCES "trackers" ("id");
ALTER TABLE "changesets_issues" ADD CONSTRAINT "wetune_fk_cc4e7ecd04d82448" FOREIGN KEY ("changeset_id") REFERENCES "changesets" ("id");
ALTER TABLE "projects_trackers" ADD CONSTRAINT "wetune_fk_94d8d13e99f056c9" FOREIGN KEY ("tracker_id") REFERENCES "trackers" ("id");
ALTER TABLE "issues" ADD CONSTRAINT "wetune_fk_0e98d6f67a7f47f7" FOREIGN KEY ("assigned_to_id") REFERENCES "users" ("id");
ALTER TABLE "wiki_content_versions" ADD CONSTRAINT "wetune_fk_4c06ba8452260a10" FOREIGN KEY ("page_id") REFERENCES "wiki_pages" ("id");
ALTER TABLE "time_entries" ADD CONSTRAINT "wetune_fk_66540dee5b39b05c" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "changesets" ADD CONSTRAINT "wetune_fk_d5620f91a71220b2" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "changeset_parents" ADD CONSTRAINT "wetune_fk_3861f3d3019b6720" FOREIGN KEY ("changeset_id") REFERENCES "changesets" ("id");
ALTER TABLE "issues" ADD CONSTRAINT "wetune_fk_36f1b88c6a8ea41d" FOREIGN KEY ("tracker_id") REFERENCES "trackers" ("id");
ALTER TABLE "groups_users" ADD CONSTRAINT "wetune_fk_fb57370d5ff25a70" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "time_entries" ADD CONSTRAINT "wetune_fk_25c1ce62a4aa785e" FOREIGN KEY ("issue_id") REFERENCES "issues" ("id");
ALTER TABLE "custom_values" ADD CONSTRAINT "wetune_fk_1de08e07792c6b06" FOREIGN KEY ("custom_field_id") REFERENCES "custom_fields" ("id");
ALTER TABLE "repositories" ADD CONSTRAINT "wetune_fk_e62bbcbf25bdd4f5" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "watchers" ADD CONSTRAINT "wetune_fk_33816c3744962c6a" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "custom_fields_projects" ADD CONSTRAINT "wetune_fk_0a55d3b06c8dcf45" FOREIGN KEY ("project_id") REFERENCES "projects" ("id");
ALTER TABLE "queries_roles" ADD CONSTRAINT "wetune_fk_c6651a06c87b8d17" FOREIGN KEY ("role_id") REFERENCES "roles" ("id");
ALTER TABLE "issue_relations" ADD CONSTRAINT "wetune_fk_3671256cf58e0fe7" FOREIGN KEY ("issue_to_id") REFERENCES "issues" ("id");
