CREATE TABLE "member_profile" (
  "id" NUMERIC(20) NOT NULL,
  "avatar_url" VARCHAR(255),
  "bio" VARCHAR(255),
  "latitude" DOUBLE PRECISION,
  "longitude" DOUBLE PRECISION,
  "github_id" BIGINT,
  "github_username" VARCHAR(255),
  "gravatar_email" VARCHAR(255),
  "hidden" SMALLINT,
  "lanyrd_username" VARCHAR(255),
  "location" VARCHAR(255),
  "name" VARCHAR(255),
  "speakerdeck_username" VARCHAR(255),
  "twitter_username" VARCHAR(255),
  "username" VARCHAR(255) NOT NULL,
  "video_embeds" VARCHAR(255),
  "job_title" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("id")
);

CREATE TABLE "post" (
  "id" NUMERIC(20) NOT NULL,
  "broadcast" SMALLINT NOT NULL,
  "category" VARCHAR(255) NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "draft" SMALLINT NOT NULL,
  "format" VARCHAR(255),
  "public_slug" VARCHAR(255),
  "publish_at" TIMESTAMP NOT NULL,
  "raw_content" VARCHAR(255) NOT NULL,
  "rendered_content" VARCHAR(255) NOT NULL,
  "rendered_summary" VARCHAR(255) NOT NULL,
  "title" VARCHAR(255) NOT NULL,
  "author_id" INTEGER NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("id"),
  UNIQUE ("public_slug")
);

CREATE TABLE "post_public_slug_aliases" (
  "post_id" INTEGER NOT NULL,
  "public_slug_aliases" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("post_id", "public_slug_aliases"),
  UNIQUE ("public_slug_aliases")
);

CREATE TABLE "project" (
  "id" VARCHAR(255) NOT NULL,
  "name" VARCHAR(255),
  "repo_url" VARCHAR(255),
  "category" VARCHAR(255),
  "site_url" VARCHAR(255),
  "stack_overflow_tags" VARCHAR(255),
  "raw_boot_config" VARCHAR(255),
  "rendered_boot_config" VARCHAR(255),
  "raw_overview" VARCHAR(255),
  "rendered_overview" VARCHAR(255),
  "parent_project_id" VARCHAR(255),
  "display_order" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "project_release_list" (
  "project_id" VARCHAR(255) NOT NULL,
  "repository_id" VARCHAR(255),
  "api_doc_url" VARCHAR(255),
  "artifact_id" VARCHAR(255),
  "group_id" VARCHAR(255),
  "is_current" SMALLINT,
  "ref_doc_url" VARCHAR(255),
  "release_status" INTEGER,
  "version_name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("project_id", "version_name")
);

CREATE TABLE "project_repository" (
  "id" VARCHAR(255) NOT NULL,
  "name" VARCHAR(255),
  "url" VARCHAR(255),
  "snapshots_enabled" SMALLINT,
  PRIMARY KEY ("id")
);

CREATE TABLE "project_sample_list" (
  "title" VARCHAR(255),
  "description" VARCHAR(255),
  "url" VARCHAR(255),
  "display_order" INTEGER NOT NULL,
  "project_id" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("project_id", "display_order")
);

CREATE TABLE "schema_version" (
  "version_rank" INTEGER NOT NULL,
  "installed_rank" INTEGER NOT NULL,
  "version" VARCHAR(50) NOT NULL,
  "description" VARCHAR(200) NOT NULL,
  "type" VARCHAR(20) NOT NULL,
  "script" VARCHAR(1000) NOT NULL,
  "checksum" INTEGER,
  "installed_by" VARCHAR(100) NOT NULL,
  "installed_on" TIMESTAMP NOT NULL,
  "execution_time" INTEGER NOT NULL,
  "success" SMALLINT NOT NULL,
  PRIMARY KEY ("version")
);

CREATE TABLE "spring_tools_platform" (
  "id" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spring_tools_platform_downloads" (
  "spring_tools_platform_id" VARCHAR(255) NOT NULL,
  "download_url" VARCHAR(255) NOT NULL,
  "variant" VARCHAR(255) NOT NULL,
  "label" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("spring_tools_platform_id", "variant")
);

-- WeTune schema patches
ALTER TABLE "post" ALTER COLUMN "public_slug" SET NOT NULL;
ALTER TABLE "post" ALTER COLUMN "publish_at" SET NOT NULL;
ALTER TABLE "project_release_list" ADD CONSTRAINT "wetune_fk_19cca5b68171b72c" FOREIGN KEY ("repository_id") REFERENCES "project_repository" ("id");
ALTER TABLE "project" ADD CONSTRAINT "wetune_fk_64882269833ce37d" FOREIGN KEY ("parent_project_id") REFERENCES "project" ("id");
ALTER TABLE "post" ADD CONSTRAINT "wetune_fk_1117062b856074d9" FOREIGN KEY ("author_id") REFERENCES "member_profile" ("id");
