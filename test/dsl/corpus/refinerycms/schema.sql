CREATE TABLE "ar_internal_metadata" (
  "key" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("key")
);

CREATE TABLE "refinery_crud_dummies" (
  "id" BIGINT NOT NULL,
  "parent_id" INTEGER,
  "lft" INTEGER,
  "rgt" INTEGER,
  "depth" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "refinery_image_translations" (
  "id" INTEGER NOT NULL,
  "image_alt" VARCHAR(255),
  "image_title" VARCHAR(255),
  "locale" VARCHAR(255) NOT NULL,
  "refinery_image_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("refinery_image_id", "locale")
);

CREATE TABLE "refinery_images" (
  "id" INTEGER NOT NULL,
  "image_mime_type" VARCHAR(255),
  "image_name" VARCHAR(255),
  "image_size" INTEGER,
  "image_width" INTEGER,
  "image_height" INTEGER,
  "image_uid" VARCHAR(255),
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "parent_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "refinery_page_part_translations" (
  "id" INTEGER NOT NULL,
  "body" TEXT,
  "locale" VARCHAR(255) NOT NULL,
  "refinery_page_part_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("refinery_page_part_id", "locale")
);

CREATE TABLE "refinery_page_parts" (
  "id" INTEGER NOT NULL,
  "refinery_page_id" INTEGER,
  "slug" VARCHAR(255),
  "position" INTEGER,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "title" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "refinery_page_translations" (
  "id" INTEGER NOT NULL,
  "title" VARCHAR(255),
  "custom_slug" VARCHAR(255),
  "menu_title" VARCHAR(255),
  "slug" VARCHAR(255),
  "locale" VARCHAR(255) NOT NULL,
  "refinery_page_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("refinery_page_id", "locale")
);

CREATE TABLE "refinery_pages" (
  "id" INTEGER NOT NULL,
  "parent_id" INTEGER,
  "path" VARCHAR(255),
  "show_in_menu" SMALLINT,
  "link_url" VARCHAR(255),
  "menu_match" VARCHAR(255),
  "deletable" SMALLINT,
  "draft" SMALLINT,
  "skip_to_first_child" SMALLINT,
  "lft" INTEGER,
  "rgt" INTEGER,
  "depth" INTEGER,
  "view_template" VARCHAR(255),
  "layout_template" VARCHAR(255),
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "children_count" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "refinery_resource_translations" (
  "id" INTEGER NOT NULL,
  "resource_title" VARCHAR(255),
  "locale" VARCHAR(255) NOT NULL,
  "refinery_resource_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("refinery_resource_id", "locale")
);

CREATE TABLE "refinery_resources" (
  "id" INTEGER NOT NULL,
  "file_mime_type" VARCHAR(255),
  "file_name" VARCHAR(255),
  "file_size" INTEGER,
  "file_uid" VARCHAR(255),
  "file_ext" VARCHAR(255),
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "schema_migrations" (
  "version" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("version")
);

CREATE TABLE "seo_meta" (
  "id" INTEGER NOT NULL,
  "seo_meta_id" INTEGER,
  "seo_meta_type" VARCHAR(255),
  "browser_title" VARCHAR(255),
  "meta_description" TEXT,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

-- WeTune schema patches
ALTER TABLE "refinery_page_parts" ALTER COLUMN "refinery_page_id" SET NOT NULL;
ALTER TABLE "seo_meta" ALTER COLUMN "seo_meta_id" SET NOT NULL;
ALTER TABLE "seo_meta" ALTER COLUMN "seo_meta_type" SET NOT NULL;
ALTER TABLE "refinery_pages" ALTER COLUMN "depth" SET NOT NULL;
ALTER TABLE "refinery_pages" ALTER COLUMN "lft" SET NOT NULL;
ALTER TABLE "refinery_pages" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "refinery_pages" ALTER COLUMN "rgt" SET NOT NULL;
ALTER TABLE "refinery_crud_dummies" ADD CONSTRAINT "wetune_fk_49b3127643b69c6b" FOREIGN KEY ("parent_id") REFERENCES "refinery_crud_dummies" ("id");
ALTER TABLE "refinery_pages" ADD CONSTRAINT "wetune_fk_f6e801cbbb88cffd" FOREIGN KEY ("parent_id") REFERENCES "refinery_pages" ("id");
ALTER TABLE "refinery_page_translations" ADD CONSTRAINT "wetune_fk_2fd636718bade0a6" FOREIGN KEY ("refinery_page_id") REFERENCES "refinery_pages" ("id");
