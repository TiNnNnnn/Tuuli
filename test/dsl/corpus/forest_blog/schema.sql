CREATE TABLE "article" (
  "article_id" INTEGER NOT NULL,
  "article_user_id" BIGINT,
  "article_title" VARCHAR(255),
  "article_content" TEXT,
  "article_view_count" INTEGER,
  "article_comment_count" INTEGER,
  "article_like_count" INTEGER,
  "article_is_comment" BIGINT,
  "article_status" BIGINT,
  "article_order" BIGINT,
  "article_update_time" TIMESTAMP,
  "article_create_time" TIMESTAMP,
  "article_summary" TEXT,
  PRIMARY KEY ("article_id")
);

CREATE TABLE "article_category_ref" (
  "article_id" INTEGER,
  "category_id" INTEGER
);

CREATE TABLE "article_tag_ref" (
  "article_id" INTEGER NOT NULL,
  "tag_id" INTEGER NOT NULL,
  PRIMARY KEY ("article_id", "tag_id")
);

CREATE TABLE "category" (
  "category_id" BIGINT NOT NULL,
  "category_pid" INTEGER,
  "category_name" VARCHAR(50),
  "category_description" VARCHAR(255),
  "category_order" BIGINT,
  "category_icon" VARCHAR(20),
  PRIMARY KEY ("category_id"),
  UNIQUE ("category_name")
);

CREATE TABLE "comment" (
  "comment_id" BIGINT NOT NULL,
  "comment_pid" BIGINT,
  "comment_pname" VARCHAR(255),
  "comment_article_id" BIGINT,
  "comment_author_name" VARCHAR(50),
  "comment_author_email" VARCHAR(50),
  "comment_author_url" VARCHAR(50),
  "comment_author_avatar" VARCHAR(100),
  "comment_content" VARCHAR(1000),
  "comment_agent" VARCHAR(200),
  "comment_ip" VARCHAR(50),
  "comment_create_time" TIMESTAMP,
  "comment_role" INTEGER,
  PRIMARY KEY ("comment_id")
);

CREATE TABLE "link" (
  "link_id" BIGINT NOT NULL,
  "link_url" VARCHAR(255),
  "link_name" VARCHAR(255),
  "link_image" VARCHAR(255),
  "link_description" VARCHAR(255),
  "link_owner_nickname" VARCHAR(40),
  "link_owner_contact" VARCHAR(255),
  "link_update_time" TIMESTAMP,
  "link_create_time" TIMESTAMP,
  "link_order" BIGINT,
  "link_status" BIGINT,
  PRIMARY KEY ("link_id"),
  UNIQUE ("link_name")
);

CREATE TABLE "menu" (
  "menu_id" INTEGER NOT NULL,
  "menu_name" VARCHAR(255),
  "menu_url" VARCHAR(255),
  "menu_level" INTEGER,
  "menu_icon" VARCHAR(255),
  "menu_order" INTEGER,
  PRIMARY KEY ("menu_id"),
  UNIQUE ("menu_name")
);

CREATE TABLE "notice" (
  "notice_id" INTEGER NOT NULL,
  "notice_title" VARCHAR(255),
  "notice_content" VARCHAR(10000),
  "notice_create_time" TIMESTAMP,
  "notice_update_time" TIMESTAMP,
  "notice_status" BIGINT,
  "notice_order" INTEGER,
  PRIMARY KEY ("notice_id")
);

CREATE TABLE "options" (
  "option_id" INTEGER NOT NULL,
  "option_site_title" VARCHAR(255),
  "option_site_descrption" VARCHAR(255),
  "option_meta_descrption" VARCHAR(255),
  "option_meta_keyword" VARCHAR(255),
  "option_aboutsite_avatar" VARCHAR(255),
  "option_aboutsite_title" VARCHAR(255),
  "option_aboutsite_content" VARCHAR(255),
  "option_aboutsite_wechat" VARCHAR(255),
  "option_aboutsite_qq" VARCHAR(255),
  "option_aboutsite_github" VARCHAR(255),
  "option_aboutsite_weibo" VARCHAR(255),
  "option_tongji" VARCHAR(255),
  "option_status" INTEGER,
  PRIMARY KEY ("option_id")
);

CREATE TABLE "page" (
  "page_id" BIGINT NOT NULL,
  "page_key" VARCHAR(50),
  "page_title" VARCHAR(50),
  "page_content" TEXT,
  "page_create_time" TIMESTAMP,
  "page_update_time" TIMESTAMP,
  "page_view_count" BIGINT,
  "page_comment_count" BIGINT,
  "page_status" BIGINT,
  PRIMARY KEY ("page_id"),
  UNIQUE ("page_key")
);

CREATE TABLE "tag" (
  "tag_id" BIGINT NOT NULL,
  "tag_name" VARCHAR(50),
  "tag_description" VARCHAR(255),
  PRIMARY KEY ("tag_id"),
  UNIQUE ("tag_name")
);

CREATE TABLE "user" (
  "user_id" BIGINT NOT NULL,
  "user_name" VARCHAR(255) NOT NULL,
  "user_pass" VARCHAR(255) NOT NULL,
  "user_nickname" VARCHAR(255) NOT NULL,
  "user_email" VARCHAR(100),
  "user_url" VARCHAR(100),
  "user_avatar" VARCHAR(255),
  "user_last_login_ip" VARCHAR(255),
  "user_register_time" TIMESTAMP,
  "user_last_login_time" TIMESTAMP,
  "user_status" BIGINT,
  PRIMARY KEY ("user_id"),
  UNIQUE ("user_name"),
  UNIQUE ("user_email")
);

-- WeTune schema patches
ALTER TABLE "link" ALTER COLUMN "link_name" SET NOT NULL;
ALTER TABLE "page" ALTER COLUMN "page_key" SET NOT NULL;
ALTER TABLE "tag" ALTER COLUMN "tag_name" SET NOT NULL;
ALTER TABLE "category" ALTER COLUMN "category_name" SET NOT NULL;
ALTER TABLE "menu" ALTER COLUMN "menu_name" SET NOT NULL;
ALTER TABLE "user" ALTER COLUMN "user_email" SET NOT NULL;
ALTER TABLE "article_category_ref" ADD CONSTRAINT "wetune_u_87fc854eb6be0102" UNIQUE ("article_id", "category_id");
ALTER TABLE "article_category_ref" ADD CONSTRAINT "wetune_fk_26870b832c6cfc74" FOREIGN KEY ("category_id") REFERENCES "category" ("category_id");
ALTER TABLE "article_category_ref" ADD CONSTRAINT "wetune_fk_e3ab7407a4e3d9a6" FOREIGN KEY ("article_id") REFERENCES "article" ("article_id");
