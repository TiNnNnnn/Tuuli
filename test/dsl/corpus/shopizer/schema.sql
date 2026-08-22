CREATE TABLE "category" (
  "category_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "category_image" VARCHAR(100),
  "category_status" INTEGER,
  "code" VARCHAR(100) NOT NULL,
  "depth" INTEGER,
  "featured" INTEGER,
  "lineage" VARCHAR(255),
  "sort_order" INTEGER,
  "visible" INTEGER,
  "merchant_id" INTEGER NOT NULL,
  "parent_id" BIGINT,
  PRIMARY KEY ("category_id"),
  UNIQUE ("merchant_id", "code")
);

CREATE TABLE "category_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "category_highlight" VARCHAR(255),
  "meta_description" VARCHAR(255),
  "meta_keywords" VARCHAR(255),
  "meta_title" VARCHAR(120),
  "sef_url" VARCHAR(120),
  "language_id" INTEGER NOT NULL,
  "category_id" BIGINT NOT NULL,
  PRIMARY KEY ("description_id"),
  UNIQUE ("category_id", "language_id")
);

CREATE TABLE "content" (
  "content_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "code" VARCHAR(100) NOT NULL,
  "content_position" VARCHAR(10),
  "content_type" VARCHAR(10),
  "link_to_menu" INTEGER,
  "product_group" VARCHAR(255),
  "sort_order" INTEGER,
  "visible" INTEGER,
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("content_id"),
  UNIQUE ("merchant_id", "code")
);

CREATE TABLE "content_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "meta_description" VARCHAR(255),
  "meta_keywords" VARCHAR(255),
  "meta_title" VARCHAR(255),
  "sef_url" VARCHAR(120),
  "language_id" INTEGER NOT NULL,
  "content_id" BIGINT NOT NULL,
  PRIMARY KEY ("description_id"),
  UNIQUE ("content_id", "language_id")
);

CREATE TABLE "country" (
  "country_id" INTEGER NOT NULL,
  "country_isocode" VARCHAR(255) NOT NULL,
  "country_supported" INTEGER,
  "geozone_id" BIGINT,
  PRIMARY KEY ("country_id"),
  UNIQUE ("country_isocode")
);

CREATE TABLE "country_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "language_id" INTEGER NOT NULL,
  "country_id" INTEGER NOT NULL,
  PRIMARY KEY ("description_id"),
  UNIQUE ("country_id", "language_id")
);

CREATE TABLE "currency" (
  "currency_id" BIGINT NOT NULL,
  "currency_code" VARCHAR(255),
  "currency_currency_code" VARCHAR(255) NOT NULL,
  "currency_name" VARCHAR(255),
  "currency_supported" INTEGER,
  PRIMARY KEY ("currency_id"),
  UNIQUE ("currency_currency_code"),
  UNIQUE ("currency_code"),
  UNIQUE ("currency_name")
);

CREATE TABLE "customer" (
  "customer_id" BIGINT NOT NULL,
  "customer_anonymous" INTEGER,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "billing_street_address" VARCHAR(256),
  "billing_city" VARCHAR(100),
  "billing_company" VARCHAR(100),
  "billing_first_name" VARCHAR(64) NOT NULL,
  "billing_last_name" VARCHAR(64) NOT NULL,
  "latitude" VARCHAR(100),
  "longitude" VARCHAR(100),
  "billing_postcode" VARCHAR(20),
  "billing_state" VARCHAR(100),
  "billing_telephone" VARCHAR(32),
  "customer_company" VARCHAR(100),
  "review_avg" DECIMAL(19, 2),
  "review_count" INTEGER,
  "customer_dob" TIMESTAMP,
  "delivery_street_address" VARCHAR(256),
  "delivery_city" VARCHAR(100),
  "delivery_company" VARCHAR(100),
  "delivery_first_name" VARCHAR(64),
  "delivery_last_name" VARCHAR(64),
  "delivery_postcode" VARCHAR(20),
  "delivery_state" VARCHAR(100),
  "delivery_telephone" VARCHAR(32),
  "customer_email_address" VARCHAR(96) NOT NULL,
  "customer_gender" VARCHAR(1),
  "customer_nick" VARCHAR(96),
  "customer_password" VARCHAR(60),
  "provider" VARCHAR(255),
  "billing_country_id" INTEGER NOT NULL,
  "billing_zone_id" BIGINT,
  "language_id" INTEGER NOT NULL,
  "delivery_country_id" INTEGER,
  "delivery_zone_id" BIGINT,
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("customer_id")
);

CREATE TABLE "customer_attribute" (
  "customer_attribute_id" BIGINT NOT NULL,
  "customer_attr_txt_val" VARCHAR(255),
  "customer_id" BIGINT NOT NULL,
  "option_id" BIGINT NOT NULL,
  "option_value_id" BIGINT NOT NULL,
  PRIMARY KEY ("customer_attribute_id"),
  UNIQUE ("option_id", "customer_id")
);

CREATE TABLE "customer_group" (
  "customer_id" BIGINT NOT NULL,
  "group_id" INTEGER NOT NULL
);

CREATE TABLE "customer_opt_val_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "language_id" INTEGER NOT NULL,
  "customer_opt_val_id" BIGINT,
  PRIMARY KEY ("description_id"),
  UNIQUE ("customer_opt_val_id", "language_id")
);

CREATE TABLE "customer_optin" (
  "customer_optin_id" BIGINT NOT NULL,
  "email" VARCHAR(255) NOT NULL,
  "first" VARCHAR(255),
  "last" VARCHAR(255),
  "optin_date" TIMESTAMP,
  "value" TEXT,
  "merchant_id" INTEGER NOT NULL,
  "optin_id" BIGINT,
  PRIMARY KEY ("customer_optin_id"),
  UNIQUE ("email", "optin_id")
);

CREATE TABLE "customer_option" (
  "customer_option_id" BIGINT NOT NULL,
  "customer_opt_active" INTEGER,
  "customer_opt_code" VARCHAR(255) NOT NULL,
  "customer_option_type" VARCHAR(10),
  "customer_opt_public" INTEGER,
  "sort_order" INTEGER,
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("customer_option_id"),
  UNIQUE ("merchant_id", "customer_opt_code")
);

CREATE TABLE "customer_option_desc" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "customer_option_comment" TEXT,
  "language_id" INTEGER NOT NULL,
  "customer_option_id" BIGINT NOT NULL,
  PRIMARY KEY ("description_id"),
  UNIQUE ("customer_option_id", "language_id")
);

CREATE TABLE "customer_option_set" (
  "customer_optionset_id" BIGINT NOT NULL,
  "sort_order" INTEGER,
  "customer_option_id" BIGINT NOT NULL,
  "customer_option_value_id" BIGINT NOT NULL,
  PRIMARY KEY ("customer_optionset_id"),
  UNIQUE ("customer_option_id", "customer_option_value_id")
);

CREATE TABLE "customer_option_value" (
  "customer_option_value_id" BIGINT NOT NULL,
  "customer_opt_val_code" VARCHAR(255) NOT NULL,
  "customer_opt_val_image" VARCHAR(255),
  "sort_order" INTEGER,
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("customer_option_value_id"),
  UNIQUE ("merchant_id", "customer_opt_val_code")
);

CREATE TABLE "customer_review" (
  "customer_review_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "review_date" TIMESTAMP,
  "reviews_rating" DOUBLE PRECISION,
  "reviews_read" BIGINT,
  "status" INTEGER,
  "customers_id" BIGINT,
  "reviewed_customer_id" BIGINT,
  PRIMARY KEY ("customer_review_id"),
  UNIQUE ("customers_id", "reviewed_customer_id")
);

CREATE TABLE "customer_review_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "language_id" INTEGER NOT NULL,
  "customer_review_id" BIGINT,
  PRIMARY KEY ("description_id"),
  UNIQUE ("customer_review_id", "language_id")
);

CREATE TABLE "file_history" (
  "file_history_id" BIGINT NOT NULL,
  "accounted_date" TIMESTAMP,
  "date_added" TIMESTAMP NOT NULL,
  "date_deleted" TIMESTAMP,
  "download_count" INTEGER NOT NULL,
  "file_id" BIGINT,
  "filesize" INTEGER NOT NULL,
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("file_history_id"),
  UNIQUE ("merchant_id", "file_id")
);

CREATE TABLE "geozone" (
  "geozone_id" BIGINT NOT NULL,
  "geozone_code" VARCHAR(255),
  "geozone_name" VARCHAR(255),
  PRIMARY KEY ("geozone_id")
);

CREATE TABLE "geozone_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "language_id" INTEGER NOT NULL,
  "geozone_id" BIGINT,
  PRIMARY KEY ("description_id"),
  UNIQUE ("geozone_id", "language_id")
);

CREATE TABLE "language" (
  "language_id" INTEGER NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "code" VARCHAR(255) NOT NULL,
  "sort_order" INTEGER,
  PRIMARY KEY ("language_id")
);

CREATE TABLE "manufacturer" (
  "manufacturer_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "code" VARCHAR(100) NOT NULL,
  "manufacturer_image" VARCHAR(255),
  "sort_order" INTEGER,
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("manufacturer_id"),
  UNIQUE ("merchant_id", "code")
);

CREATE TABLE "manufacturer_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "date_last_click" TIMESTAMP,
  "manufacturers_url" VARCHAR(255),
  "url_clicked" INTEGER,
  "language_id" INTEGER NOT NULL,
  "manufacturer_id" BIGINT NOT NULL,
  PRIMARY KEY ("description_id"),
  UNIQUE ("manufacturer_id", "language_id")
);

CREATE TABLE "merchant_configuration" (
  "merchant_config_id" BIGINT NOT NULL,
  "active" INTEGER,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "config_key" VARCHAR(255),
  "type" VARCHAR(255),
  "value" TEXT,
  "merchant_id" INTEGER,
  PRIMARY KEY ("merchant_config_id"),
  UNIQUE ("merchant_id", "config_key")
);

CREATE TABLE "merchant_language" (
  "stores_merchant_id" INTEGER NOT NULL,
  "languages_language_id" INTEGER NOT NULL
);

CREATE TABLE "merchant_log" (
  "merchant_log_id" BIGINT NOT NULL,
  "log" TEXT,
  "module" VARCHAR(25),
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("merchant_log_id")
);

CREATE TABLE "merchant_store" (
  "merchant_id" INTEGER NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "store_code" VARCHAR(100) NOT NULL,
  "continueshoppingurl" VARCHAR(150),
  "currency_format_national" INTEGER,
  "domain_name" VARCHAR(80),
  "in_business_since" DATE,
  "invoice_template" VARCHAR(25),
  "seizeunitcode" VARCHAR(5),
  "store_email" VARCHAR(60) NOT NULL,
  "store_logo" VARCHAR(100),
  "store_template" VARCHAR(25),
  "store_address" VARCHAR(255),
  "store_city" VARCHAR(100) NOT NULL,
  "store_name" VARCHAR(100) NOT NULL,
  "store_phone" VARCHAR(50) NOT NULL,
  "store_postal_code" VARCHAR(15) NOT NULL,
  "store_state_prov" VARCHAR(100),
  "use_cache" INTEGER,
  "weightunitcode" VARCHAR(5),
  "country_id" INTEGER NOT NULL,
  "currency_id" BIGINT NOT NULL,
  "language_id" INTEGER NOT NULL,
  "zone_id" BIGINT,
  PRIMARY KEY ("merchant_id"),
  UNIQUE ("store_code")
);

CREATE TABLE "module_configuration" (
  "module_conf_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "code" VARCHAR(255) NOT NULL,
  "details" TEXT,
  "configuration" TEXT,
  "custom_ind" INTEGER,
  "image" VARCHAR(255),
  "module" VARCHAR(255),
  "regions" VARCHAR(255),
  "type" VARCHAR(255),
  PRIMARY KEY ("module_conf_id")
);

CREATE TABLE "optin" (
  "optin_id" BIGINT NOT NULL,
  "code" VARCHAR(255) NOT NULL,
  "description" VARCHAR(255),
  "end_date" TIMESTAMP,
  "type" VARCHAR(255) NOT NULL,
  "start_date" TIMESTAMP,
  "merchant_id" INTEGER,
  PRIMARY KEY ("optin_id"),
  UNIQUE ("merchant_id", "code")
);

CREATE TABLE "order_account" (
  "order_account_id" BIGINT NOT NULL,
  "order_account_bill_day" INTEGER NOT NULL,
  "order_account_end_date" DATE,
  "order_account_start_date" DATE NOT NULL,
  "order_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_account_id")
);

CREATE TABLE "order_account_product" (
  "order_account_product_id" BIGINT NOT NULL,
  "order_account_product_accnt_dt" DATE,
  "order_account_product_end_dt" DATE,
  "order_account_product_eot" TIMESTAMP,
  "order_account_product_l_st_dt" TIMESTAMP,
  "order_account_product_l_trx_st" INTEGER NOT NULL,
  "order_account_product_pm_fr_ty" INTEGER NOT NULL,
  "order_account_product_st_dt" DATE NOT NULL,
  "order_account_product_status" INTEGER NOT NULL,
  "order_account_id" BIGINT NOT NULL,
  "order_product_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_account_product_id")
);

CREATE TABLE "order_attribute" (
  "order_attribute_id" BIGINT NOT NULL,
  "identifier" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255) NOT NULL,
  "order_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_attribute_id")
);

CREATE TABLE "order_product" (
  "order_product_id" BIGINT NOT NULL,
  "onetime_charge" DECIMAL(19, 2) NOT NULL,
  "product_name" VARCHAR(64) NOT NULL,
  "product_quantity" INTEGER,
  "product_sku" VARCHAR(255),
  "order_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_product_id")
);

CREATE TABLE "order_product_attribute" (
  "order_product_attribute_id" BIGINT NOT NULL,
  "product_attribute_is_free" INTEGER NOT NULL,
  "product_attribute_name" VARCHAR(255),
  "product_attribute_price" DECIMAL(15, 4) NOT NULL,
  "product_attribute_val_name" VARCHAR(255),
  "product_attribute_weight" DECIMAL(15, 4),
  "product_option_id" BIGINT NOT NULL,
  "product_option_value_id" BIGINT NOT NULL,
  "order_product_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_product_attribute_id")
);

CREATE TABLE "order_product_download" (
  "order_product_download_id" BIGINT NOT NULL,
  "download_count" INTEGER NOT NULL,
  "download_maxdays" INTEGER NOT NULL,
  "order_product_filename" VARCHAR(255) NOT NULL,
  "order_product_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_product_download_id")
);

CREATE TABLE "order_product_price" (
  "order_product_price_id" BIGINT NOT NULL,
  "default_price" INTEGER NOT NULL,
  "product_price" DECIMAL(19, 2) NOT NULL,
  "product_price_code" VARCHAR(64) NOT NULL,
  "product_price_name" VARCHAR(255),
  "product_price_special" DECIMAL(19, 2),
  "prd_price_special_end_dt" TIMESTAMP,
  "prd_price_special_st_dt" TIMESTAMP,
  "order_product_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_product_price_id")
);

CREATE TABLE "order_status_history" (
  "order_status_history_id" BIGINT NOT NULL,
  "comments" TEXT,
  "customer_notified" INTEGER,
  "date_added" TIMESTAMP NOT NULL,
  "status" VARCHAR(255),
  "order_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_status_history_id")
);

CREATE TABLE "order_total" (
  "order_account_id" BIGINT NOT NULL,
  "module" VARCHAR(60),
  "code" VARCHAR(255) NOT NULL,
  "order_total_type" VARCHAR(255),
  "order_value_type" VARCHAR(255),
  "sort_order" INTEGER NOT NULL,
  "text" TEXT,
  "title" VARCHAR(255),
  "value" DECIMAL(15, 4) NOT NULL,
  "order_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_account_id")
);

CREATE TABLE "orders" (
  "order_id" BIGINT NOT NULL,
  "billing_street_address" VARCHAR(256),
  "billing_city" VARCHAR(100),
  "billing_company" VARCHAR(100),
  "billing_first_name" VARCHAR(64) NOT NULL,
  "billing_last_name" VARCHAR(64) NOT NULL,
  "latitude" VARCHAR(100),
  "longitude" VARCHAR(100),
  "billing_postcode" VARCHAR(20),
  "billing_state" VARCHAR(100),
  "billing_telephone" VARCHAR(32),
  "channel" VARCHAR(255),
  "confirmed_address" INTEGER,
  "card_type" VARCHAR(255),
  "cc_cvv" VARCHAR(255),
  "cc_expires" VARCHAR(255),
  "cc_number" VARCHAR(255),
  "cc_owner" VARCHAR(255),
  "currency_value" DECIMAL(19, 2),
  "customer_agreed" INTEGER,
  "customer_email_address" VARCHAR(50) NOT NULL,
  "customer_id" BIGINT,
  "date_purchased" DATE,
  "delivery_street_address" VARCHAR(256),
  "delivery_city" VARCHAR(100),
  "delivery_company" VARCHAR(100),
  "delivery_first_name" VARCHAR(64),
  "delivery_last_name" VARCHAR(64),
  "delivery_postcode" VARCHAR(20),
  "delivery_state" VARCHAR(100),
  "delivery_telephone" VARCHAR(32),
  "ip_address" VARCHAR(255),
  "last_modified" TIMESTAMP,
  "locale" VARCHAR(255),
  "order_date_finished" TIMESTAMP,
  "order_type" VARCHAR(255),
  "payment_module_code" VARCHAR(255),
  "payment_type" VARCHAR(255),
  "shipping_module_code" VARCHAR(255),
  "order_status" VARCHAR(255),
  "order_total" DECIMAL(19, 2),
  "billing_country_id" INTEGER NOT NULL,
  "billing_zone_id" BIGINT,
  "currency_id" BIGINT,
  "delivery_country_id" INTEGER,
  "delivery_zone_id" BIGINT,
  "merchantid" INTEGER,
  PRIMARY KEY ("order_id")
);

CREATE TABLE "permission" (
  "permission_id" INTEGER NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "permission_name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("permission_id"),
  UNIQUE ("permission_name")
);

CREATE TABLE "permission_group" (
  "permission_id" INTEGER NOT NULL,
  "group_id" INTEGER NOT NULL
);

CREATE TABLE "product" (
  "product_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "available" INTEGER,
  "cond" INTEGER,
  "date_available" TIMESTAMP,
  "preorder" INTEGER,
  "product_height" DECIMAL(19, 2),
  "product_free" INTEGER,
  "product_length" DECIMAL(19, 2),
  "quantity_ordered" INTEGER,
  "review_avg" DECIMAL(19, 2),
  "review_count" INTEGER,
  "product_ship" INTEGER,
  "product_virtual" INTEGER,
  "product_weight" DECIMAL(19, 2),
  "product_width" DECIMAL(19, 2),
  "ref_sku" VARCHAR(255),
  "rental_duration" INTEGER,
  "rental_period" INTEGER,
  "rental_status" INTEGER,
  "sku" VARCHAR(255) NOT NULL,
  "sort_order" INTEGER,
  "manufacturer_id" BIGINT,
  "merchant_id" INTEGER NOT NULL,
  "customer_id" BIGINT,
  "tax_class_id" BIGINT,
  "product_type_id" BIGINT,
  PRIMARY KEY ("product_id"),
  UNIQUE ("merchant_id", "sku")
);

CREATE TABLE "product_attribute" (
  "product_attribute_id" BIGINT NOT NULL,
  "product_attribute_default" INTEGER,
  "product_attribute_discounted" INTEGER,
  "product_attribute_for_disp" INTEGER,
  "product_attribute_required" INTEGER,
  "product_attribute_free" INTEGER,
  "product_atribute_price" DECIMAL(19, 2),
  "product_attribute_weight" DECIMAL(19, 2),
  "product_attribute_sort_ord" INTEGER,
  "product_id" BIGINT NOT NULL,
  "option_id" BIGINT NOT NULL,
  "option_value_id" BIGINT NOT NULL,
  PRIMARY KEY ("product_attribute_id"),
  UNIQUE ("option_id", "option_value_id", "product_id")
);

CREATE TABLE "product_availability" (
  "product_avail_id" BIGINT NOT NULL,
  "date_available" DATE,
  "free_shipping" INTEGER,
  "quantity" INTEGER NOT NULL,
  "quantity_ord_max" INTEGER,
  "quantity_ord_min" INTEGER,
  "status" INTEGER,
  "region" VARCHAR(255),
  "region_variant" VARCHAR(255),
  "product_id" BIGINT NOT NULL,
  PRIMARY KEY ("product_avail_id")
);

CREATE TABLE "product_category" (
  "product_id" BIGINT NOT NULL,
  "category_id" BIGINT NOT NULL,
  PRIMARY KEY ("product_id", "category_id")
);

CREATE TABLE "product_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "meta_description" VARCHAR(255),
  "meta_keywords" VARCHAR(255),
  "meta_title" VARCHAR(255),
  "download_lnk" VARCHAR(255),
  "product_highlight" VARCHAR(255),
  "sef_url" VARCHAR(255),
  "language_id" INTEGER NOT NULL,
  "product_id" BIGINT NOT NULL,
  PRIMARY KEY ("description_id"),
  UNIQUE ("product_id", "language_id")
);

CREATE TABLE "product_digital" (
  "product_digital_id" BIGINT NOT NULL,
  "file_name" VARCHAR(255) NOT NULL,
  "product_id" BIGINT NOT NULL,
  PRIMARY KEY ("product_digital_id"),
  UNIQUE ("product_id", "file_name")
);

CREATE TABLE "product_image" (
  "product_image_id" BIGINT NOT NULL,
  "default_image" INTEGER,
  "image_crop" INTEGER,
  "image_type" INTEGER,
  "product_image" VARCHAR(255),
  "product_image_url" VARCHAR(255),
  "product_id" BIGINT NOT NULL,
  PRIMARY KEY ("product_image_id")
);

CREATE TABLE "product_image_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "alt_tag" VARCHAR(100),
  "language_id" INTEGER NOT NULL,
  "product_image_id" BIGINT NOT NULL,
  PRIMARY KEY ("description_id"),
  UNIQUE ("product_image_id", "language_id")
);

CREATE TABLE "product_option" (
  "product_option_id" BIGINT NOT NULL,
  "product_option_code" VARCHAR(255) NOT NULL,
  "product_option_sort_ord" INTEGER,
  "product_option_type" VARCHAR(10),
  "product_option_read" INTEGER,
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("product_option_id"),
  UNIQUE ("merchant_id", "product_option_code")
);

CREATE TABLE "product_option_desc" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "product_option_comment" TEXT,
  "language_id" INTEGER NOT NULL,
  "product_option_id" BIGINT NOT NULL,
  PRIMARY KEY ("description_id"),
  UNIQUE ("product_option_id", "language_id")
);

CREATE TABLE "product_option_value" (
  "product_option_value_id" BIGINT NOT NULL,
  "product_option_val_code" VARCHAR(255) NOT NULL,
  "product_opt_for_disp" INTEGER,
  "product_opt_val_image" VARCHAR(255),
  "product_opt_val_sort_ord" INTEGER,
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("product_option_value_id"),
  UNIQUE ("merchant_id", "product_option_val_code")
);

CREATE TABLE "product_option_value_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "language_id" INTEGER NOT NULL,
  "product_option_value_id" BIGINT,
  PRIMARY KEY ("description_id"),
  UNIQUE ("product_option_value_id", "language_id")
);

CREATE TABLE "product_price" (
  "product_price_id" BIGINT NOT NULL,
  "product_price_code" VARCHAR(255) NOT NULL,
  "default_price" INTEGER,
  "product_price_amount" DECIMAL(19, 2) NOT NULL,
  "product_price_special_amount" DECIMAL(19, 2),
  "product_price_special_end_date" DATE,
  "product_price_special_st_date" DATE,
  "product_price_type" VARCHAR(20),
  "product_avail_id" BIGINT NOT NULL,
  PRIMARY KEY ("product_price_id")
);

CREATE TABLE "product_price_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "language_id" INTEGER NOT NULL,
  "product_price_id" BIGINT NOT NULL,
  PRIMARY KEY ("description_id"),
  UNIQUE ("product_price_id", "language_id")
);

CREATE TABLE "product_relationship" (
  "product_relationship_id" BIGINT NOT NULL,
  "active" INTEGER,
  "code" VARCHAR(255),
  "product_id" BIGINT,
  "related_product_id" BIGINT,
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("product_relationship_id")
);

CREATE TABLE "product_review" (
  "product_review_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "review_date" TIMESTAMP,
  "reviews_rating" DOUBLE PRECISION,
  "reviews_read" BIGINT,
  "status" INTEGER,
  "customers_id" BIGINT,
  "product_id" BIGINT,
  PRIMARY KEY ("product_review_id"),
  UNIQUE ("customers_id", "product_id")
);

CREATE TABLE "product_review_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "language_id" INTEGER NOT NULL,
  "product_review_id" BIGINT,
  PRIMARY KEY ("description_id"),
  UNIQUE ("product_review_id", "language_id")
);

CREATE TABLE "product_type" (
  "product_type_id" BIGINT NOT NULL,
  "prd_type_add_to_cart" INTEGER,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "prd_type_code" VARCHAR(255),
  PRIMARY KEY ("product_type_id")
);

CREATE TABLE "shiping_origin" (
  "ship_origin_id" BIGINT NOT NULL,
  "active" INTEGER,
  "street_address" VARCHAR(256) NOT NULL,
  "city" VARCHAR(100) NOT NULL,
  "postcode" VARCHAR(20) NOT NULL,
  "state" VARCHAR(100),
  "country_id" INTEGER,
  "merchant_id" INTEGER NOT NULL,
  "zone_id" BIGINT,
  PRIMARY KEY ("ship_origin_id")
);

CREATE TABLE "shipping_quote" (
  "shipping_quote_id" BIGINT NOT NULL,
  "cart_id" BIGINT,
  "customer_id" BIGINT,
  "delivery_street_address" VARCHAR(256),
  "delivery_city" VARCHAR(100),
  "delivery_company" VARCHAR(100),
  "delivery_first_name" VARCHAR(64),
  "delivery_last_name" VARCHAR(64),
  "delivery_postcode" VARCHAR(20),
  "delivery_state" VARCHAR(100),
  "delivery_telephone" VARCHAR(32),
  "shipping_number_days" INTEGER,
  "free_shipping" INTEGER,
  "quote_handling" DECIMAL(19, 2),
  "module" VARCHAR(255) NOT NULL,
  "option_code" VARCHAR(255),
  "option_delivery_date" TIMESTAMP,
  "option_name" VARCHAR(255),
  "option_shipping_date" TIMESTAMP,
  "order_id" BIGINT,
  "quote_price" DECIMAL(19, 2),
  "quote_date" TIMESTAMP,
  "delivery_country_id" INTEGER,
  "delivery_zone_id" BIGINT,
  PRIMARY KEY ("shipping_quote_id")
);

CREATE TABLE "shopping_cart" (
  "shp_cart_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "customer_id" BIGINT,
  "shp_cart_code" VARCHAR(255) NOT NULL,
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("shp_cart_id"),
  UNIQUE ("shp_cart_code")
);

CREATE TABLE "shopping_cart_attr_item" (
  "shp_cart_attr_item_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "product_attr_id" BIGINT NOT NULL,
  "shp_cart_item_id" BIGINT NOT NULL,
  PRIMARY KEY ("shp_cart_attr_item_id")
);

CREATE TABLE "shopping_cart_item" (
  "shp_cart_item_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "product_id" BIGINT NOT NULL,
  "quantity" INTEGER,
  "shp_cart_id" BIGINT NOT NULL,
  PRIMARY KEY ("shp_cart_item_id")
);

CREATE TABLE "sm_group" (
  "group_id" INTEGER NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "group_name" VARCHAR(255) NOT NULL,
  "group_type" VARCHAR(255),
  PRIMARY KEY ("group_id"),
  UNIQUE ("group_name")
);

CREATE TABLE "sm_sequencer" (
  "seq_name" VARCHAR(255) NOT NULL,
  "seq_count" BIGINT,
  PRIMARY KEY ("seq_name")
);

CREATE TABLE "sm_transaction" (
  "transaction_id" BIGINT NOT NULL,
  "amount" DECIMAL(19, 2),
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "details" TEXT,
  "payment_type" VARCHAR(255),
  "transaction_date" TIMESTAMP,
  "transaction_type" VARCHAR(255),
  "order_id" BIGINT,
  PRIMARY KEY ("transaction_id")
);

CREATE TABLE "system_configuration" (
  "system_config_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "config_key" VARCHAR(255),
  "value" VARCHAR(255),
  PRIMARY KEY ("system_config_id")
);

CREATE TABLE "system_notification" (
  "system_notif_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "end_date" DATE,
  "config_key" VARCHAR(255),
  "start_date" DATE,
  "value" VARCHAR(255),
  "merchant_id" INTEGER,
  "user_id" BIGINT,
  PRIMARY KEY ("system_notif_id"),
  UNIQUE ("merchant_id", "config_key")
);

CREATE TABLE "tax_class" (
  "tax_class_id" BIGINT NOT NULL,
  "tax_class_code" VARCHAR(10) NOT NULL,
  "tax_class_title" VARCHAR(32) NOT NULL,
  "merchant_id" INTEGER,
  PRIMARY KEY ("tax_class_id"),
  UNIQUE ("merchant_id", "tax_class_code")
);

CREATE TABLE "tax_rate" (
  "tax_rate_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "tax_code" VARCHAR(255) NOT NULL,
  "piggyback" INTEGER,
  "store_state_prov" VARCHAR(100),
  "tax_priority" INTEGER,
  "tax_rate" DECIMAL(7, 4) NOT NULL,
  "country_id" INTEGER NOT NULL,
  "merchant_id" INTEGER NOT NULL,
  "parent_id" BIGINT,
  "tax_class_id" BIGINT NOT NULL,
  "zone_id" BIGINT,
  PRIMARY KEY ("tax_rate_id"),
  UNIQUE ("tax_code", "merchant_id")
);

CREATE TABLE "tax_rate_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "language_id" INTEGER NOT NULL,
  "tax_rate_id" BIGINT,
  PRIMARY KEY ("description_id"),
  UNIQUE ("tax_rate_id", "language_id")
);

CREATE TABLE "user" (
  "user_id" BIGINT NOT NULL,
  "active" INTEGER,
  "admin_email" VARCHAR(255) NOT NULL,
  "admin_name" VARCHAR(100) NOT NULL,
  "admin_password" VARCHAR(60) NOT NULL,
  "admin_a1" VARCHAR(255),
  "admin_a2" VARCHAR(255),
  "admin_a3" VARCHAR(255),
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "admin_first_name" VARCHAR(255),
  "last_access" TIMESTAMP,
  "admin_last_name" VARCHAR(255),
  "login_access" TIMESTAMP,
  "admin_q1" VARCHAR(255),
  "admin_q2" VARCHAR(255),
  "admin_q3" VARCHAR(255),
  "language_id" INTEGER,
  "merchant_id" INTEGER NOT NULL,
  PRIMARY KEY ("user_id"),
  UNIQUE ("admin_name")
);

CREATE TABLE "user_group" (
  "user_id" BIGINT NOT NULL,
  "group_id" INTEGER NOT NULL
);

CREATE TABLE "userconnection" (
  "providerid" VARCHAR(255) NOT NULL,
  "provideruserid" VARCHAR(255) NOT NULL,
  "userid" VARCHAR(255) NOT NULL,
  "accesstoken" VARCHAR(255),
  "displayname" VARCHAR(255),
  "expiretime" BIGINT,
  "imageurl" VARCHAR(255),
  "profileurl" VARCHAR(255),
  "refreshtoken" VARCHAR(255),
  "secret" VARCHAR(255),
  "userrank" INTEGER NOT NULL,
  PRIMARY KEY ("providerid", "provideruserid", "userid")
);

CREATE TABLE "zone" (
  "zone_id" BIGINT NOT NULL,
  "zone_code" VARCHAR(255) NOT NULL,
  "country_id" INTEGER NOT NULL,
  PRIMARY KEY ("zone_id"),
  UNIQUE ("zone_code")
);

CREATE TABLE "zone_description" (
  "description_id" BIGINT NOT NULL,
  "date_created" TIMESTAMP,
  "date_modified" TIMESTAMP,
  "updt_id" VARCHAR(20),
  "description" TEXT,
  "name" VARCHAR(120) NOT NULL,
  "title" VARCHAR(100),
  "language_id" INTEGER NOT NULL,
  "zone_id" BIGINT NOT NULL,
  PRIMARY KEY ("description_id"),
  UNIQUE ("zone_id", "language_id")
);

ALTER TABLE "category" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "category" ADD FOREIGN KEY ("parent_id") REFERENCES "category" ("category_id");

ALTER TABLE "category_description" ADD FOREIGN KEY ("category_id") REFERENCES "category" ("category_id");

ALTER TABLE "category_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "content" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "content_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "content_description" ADD FOREIGN KEY ("content_id") REFERENCES "content" ("content_id");

ALTER TABLE "country" ADD FOREIGN KEY ("geozone_id") REFERENCES "geozone" ("geozone_id");

ALTER TABLE "country_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "country_description" ADD FOREIGN KEY ("country_id") REFERENCES "country" ("country_id");

ALTER TABLE "customer" ADD FOREIGN KEY ("delivery_zone_id") REFERENCES "zone" ("zone_id");

ALTER TABLE "customer" ADD FOREIGN KEY ("billing_country_id") REFERENCES "country" ("country_id");

ALTER TABLE "customer" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "customer" ADD FOREIGN KEY ("delivery_country_id") REFERENCES "country" ("country_id");

ALTER TABLE "customer" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "customer" ADD FOREIGN KEY ("billing_zone_id") REFERENCES "zone" ("zone_id");

ALTER TABLE "customer_attribute" ADD FOREIGN KEY ("option_id") REFERENCES "customer_option" ("customer_option_id");

ALTER TABLE "customer_attribute" ADD FOREIGN KEY ("option_value_id") REFERENCES "customer_option_value" ("customer_option_value_id");

ALTER TABLE "customer_attribute" ADD FOREIGN KEY ("customer_id") REFERENCES "customer" ("customer_id");

ALTER TABLE "customer_group" ADD FOREIGN KEY ("customer_id") REFERENCES "customer" ("customer_id");

ALTER TABLE "customer_group" ADD FOREIGN KEY ("group_id") REFERENCES "sm_group" ("group_id");

ALTER TABLE "customer_opt_val_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "customer_opt_val_description" ADD FOREIGN KEY ("customer_opt_val_id") REFERENCES "customer_option_value" ("customer_option_value_id");

ALTER TABLE "customer_optin" ADD FOREIGN KEY ("optin_id") REFERENCES "optin" ("optin_id");

ALTER TABLE "customer_optin" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "customer_option" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "customer_option_desc" ADD FOREIGN KEY ("customer_option_id") REFERENCES "customer_option" ("customer_option_id");

ALTER TABLE "customer_option_desc" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "customer_option_set" ADD FOREIGN KEY ("customer_option_id") REFERENCES "customer_option" ("customer_option_id");

ALTER TABLE "customer_option_set" ADD FOREIGN KEY ("customer_option_value_id") REFERENCES "customer_option_value" ("customer_option_value_id");

ALTER TABLE "customer_option_value" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "customer_review" ADD FOREIGN KEY ("reviewed_customer_id") REFERENCES "customer" ("customer_id");

ALTER TABLE "customer_review" ADD FOREIGN KEY ("customers_id") REFERENCES "customer" ("customer_id");

ALTER TABLE "customer_review_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "customer_review_description" ADD FOREIGN KEY ("customer_review_id") REFERENCES "customer_review" ("customer_review_id");

ALTER TABLE "file_history" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "geozone_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "geozone_description" ADD FOREIGN KEY ("geozone_id") REFERENCES "geozone" ("geozone_id");

ALTER TABLE "manufacturer" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "manufacturer_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "manufacturer_description" ADD FOREIGN KEY ("manufacturer_id") REFERENCES "manufacturer" ("manufacturer_id");

ALTER TABLE "merchant_configuration" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "merchant_language" ADD FOREIGN KEY ("stores_merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "merchant_language" ADD FOREIGN KEY ("languages_language_id") REFERENCES "language" ("language_id");

ALTER TABLE "merchant_log" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "merchant_store" ADD FOREIGN KEY ("country_id") REFERENCES "country" ("country_id");

ALTER TABLE "merchant_store" ADD FOREIGN KEY ("zone_id") REFERENCES "zone" ("zone_id");

ALTER TABLE "merchant_store" ADD FOREIGN KEY ("currency_id") REFERENCES "currency" ("currency_id");

ALTER TABLE "merchant_store" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "optin" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "order_account" ADD FOREIGN KEY ("order_id") REFERENCES "orders" ("order_id");

ALTER TABLE "order_account_product" ADD FOREIGN KEY ("order_product_id") REFERENCES "order_product" ("order_product_id");

ALTER TABLE "order_account_product" ADD FOREIGN KEY ("order_account_id") REFERENCES "order_account" ("order_account_id");

ALTER TABLE "order_attribute" ADD FOREIGN KEY ("order_id") REFERENCES "orders" ("order_id");

ALTER TABLE "order_product" ADD FOREIGN KEY ("order_id") REFERENCES "orders" ("order_id");

ALTER TABLE "order_product_attribute" ADD FOREIGN KEY ("order_product_id") REFERENCES "order_product" ("order_product_id");

ALTER TABLE "order_product_download" ADD FOREIGN KEY ("order_product_id") REFERENCES "order_product" ("order_product_id");

ALTER TABLE "order_product_price" ADD FOREIGN KEY ("order_product_id") REFERENCES "order_product" ("order_product_id");

ALTER TABLE "order_status_history" ADD FOREIGN KEY ("order_id") REFERENCES "orders" ("order_id");

ALTER TABLE "order_total" ADD FOREIGN KEY ("order_id") REFERENCES "orders" ("order_id");

ALTER TABLE "orders" ADD FOREIGN KEY ("merchantid") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "orders" ADD FOREIGN KEY ("currency_id") REFERENCES "currency" ("currency_id");

ALTER TABLE "orders" ADD FOREIGN KEY ("billing_country_id") REFERENCES "country" ("country_id");

ALTER TABLE "orders" ADD FOREIGN KEY ("billing_zone_id") REFERENCES "zone" ("zone_id");

ALTER TABLE "orders" ADD FOREIGN KEY ("delivery_zone_id") REFERENCES "zone" ("zone_id");

ALTER TABLE "orders" ADD FOREIGN KEY ("delivery_country_id") REFERENCES "country" ("country_id");

ALTER TABLE "permission_group" ADD FOREIGN KEY ("permission_id") REFERENCES "permission" ("permission_id");

ALTER TABLE "permission_group" ADD FOREIGN KEY ("group_id") REFERENCES "sm_group" ("group_id");

ALTER TABLE "product" ADD FOREIGN KEY ("tax_class_id") REFERENCES "tax_class" ("tax_class_id");

ALTER TABLE "product" ADD FOREIGN KEY ("product_type_id") REFERENCES "product_type" ("product_type_id");

ALTER TABLE "product" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "product" ADD FOREIGN KEY ("customer_id") REFERENCES "customer" ("customer_id");

ALTER TABLE "product" ADD FOREIGN KEY ("manufacturer_id") REFERENCES "manufacturer" ("manufacturer_id");

ALTER TABLE "product_attribute" ADD FOREIGN KEY ("option_id") REFERENCES "product_option" ("product_option_id");

ALTER TABLE "product_attribute" ADD FOREIGN KEY ("option_value_id") REFERENCES "product_option_value" ("product_option_value_id");

ALTER TABLE "product_attribute" ADD FOREIGN KEY ("product_id") REFERENCES "product" ("product_id");

ALTER TABLE "product_availability" ADD FOREIGN KEY ("product_id") REFERENCES "product" ("product_id");

ALTER TABLE "product_category" ADD FOREIGN KEY ("category_id") REFERENCES "category" ("category_id");

ALTER TABLE "product_category" ADD FOREIGN KEY ("product_id") REFERENCES "product" ("product_id");

ALTER TABLE "product_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "product_description" ADD FOREIGN KEY ("product_id") REFERENCES "product" ("product_id");

ALTER TABLE "product_digital" ADD FOREIGN KEY ("product_id") REFERENCES "product" ("product_id");

ALTER TABLE "product_image" ADD FOREIGN KEY ("product_id") REFERENCES "product" ("product_id");

ALTER TABLE "product_image_description" ADD FOREIGN KEY ("product_image_id") REFERENCES "product_image" ("product_image_id");

ALTER TABLE "product_image_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "product_option" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "product_option_desc" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "product_option_desc" ADD FOREIGN KEY ("product_option_id") REFERENCES "product_option" ("product_option_id");

ALTER TABLE "product_option_value" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "product_option_value_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "product_option_value_description" ADD FOREIGN KEY ("product_option_value_id") REFERENCES "product_option_value" ("product_option_value_id");

ALTER TABLE "product_price" ADD FOREIGN KEY ("product_avail_id") REFERENCES "product_availability" ("product_avail_id");

ALTER TABLE "product_price_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "product_price_description" ADD FOREIGN KEY ("product_price_id") REFERENCES "product_price" ("product_price_id");

ALTER TABLE "product_relationship" ADD FOREIGN KEY ("related_product_id") REFERENCES "product" ("product_id");

ALTER TABLE "product_relationship" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "product_relationship" ADD FOREIGN KEY ("product_id") REFERENCES "product" ("product_id");

ALTER TABLE "product_review" ADD FOREIGN KEY ("customers_id") REFERENCES "customer" ("customer_id");

ALTER TABLE "product_review" ADD FOREIGN KEY ("product_id") REFERENCES "product" ("product_id");

ALTER TABLE "product_review_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "product_review_description" ADD FOREIGN KEY ("product_review_id") REFERENCES "product_review" ("product_review_id");

ALTER TABLE "shiping_origin" ADD FOREIGN KEY ("zone_id") REFERENCES "zone" ("zone_id");

ALTER TABLE "shiping_origin" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "shiping_origin" ADD FOREIGN KEY ("country_id") REFERENCES "country" ("country_id");

ALTER TABLE "shipping_quote" ADD FOREIGN KEY ("delivery_country_id") REFERENCES "country" ("country_id");

ALTER TABLE "shipping_quote" ADD FOREIGN KEY ("delivery_zone_id") REFERENCES "zone" ("zone_id");

ALTER TABLE "shopping_cart" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "shopping_cart_attr_item" ADD FOREIGN KEY ("shp_cart_item_id") REFERENCES "shopping_cart_item" ("shp_cart_item_id");

ALTER TABLE "shopping_cart_item" ADD FOREIGN KEY ("shp_cart_id") REFERENCES "shopping_cart" ("shp_cart_id");

ALTER TABLE "sm_transaction" ADD FOREIGN KEY ("order_id") REFERENCES "orders" ("order_id");

ALTER TABLE "system_notification" ADD FOREIGN KEY ("user_id") REFERENCES "user" ("user_id");

ALTER TABLE "system_notification" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "tax_class" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "tax_rate" ADD FOREIGN KEY ("country_id") REFERENCES "country" ("country_id");

ALTER TABLE "tax_rate" ADD FOREIGN KEY ("tax_class_id") REFERENCES "tax_class" ("tax_class_id");

ALTER TABLE "tax_rate" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "tax_rate" ADD FOREIGN KEY ("zone_id") REFERENCES "zone" ("zone_id");

ALTER TABLE "tax_rate" ADD FOREIGN KEY ("parent_id") REFERENCES "tax_rate" ("tax_rate_id");

ALTER TABLE "tax_rate_description" ADD FOREIGN KEY ("tax_rate_id") REFERENCES "tax_rate" ("tax_rate_id");

ALTER TABLE "tax_rate_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "user" ADD FOREIGN KEY ("merchant_id") REFERENCES "merchant_store" ("merchant_id");

ALTER TABLE "user" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "user_group" ADD FOREIGN KEY ("group_id") REFERENCES "sm_group" ("group_id");

ALTER TABLE "user_group" ADD FOREIGN KEY ("user_id") REFERENCES "user" ("user_id");

ALTER TABLE "zone" ADD FOREIGN KEY ("country_id") REFERENCES "country" ("country_id");

ALTER TABLE "zone_description" ADD FOREIGN KEY ("language_id") REFERENCES "language" ("language_id");

ALTER TABLE "zone_description" ADD FOREIGN KEY ("zone_id") REFERENCES "zone" ("zone_id");

-- WeTune schema patches
ALTER TABLE "product" ALTER COLUMN "manufacturer_id" SET NOT NULL;
ALTER TABLE "country" ALTER COLUMN "geozone_id" SET NOT NULL;
ALTER TABLE "sm_transaction" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "system_notification" ALTER COLUMN "config_key" SET NOT NULL;
ALTER TABLE "system_notification" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "system_notification" ALTER COLUMN "merchant_id" SET NOT NULL;
ALTER TABLE "merchant_store" ALTER COLUMN "zone_id" SET NOT NULL;
ALTER TABLE "tax_rate_description" ALTER COLUMN "tax_rate_id" SET NOT NULL;
ALTER TABLE "tax_rate" ALTER COLUMN "zone_id" SET NOT NULL;
ALTER TABLE "tax_rate" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "product_option_value_description" ALTER COLUMN "product_option_value_id" SET NOT NULL;
ALTER TABLE "shopping_cart" ALTER COLUMN "customer_id" SET NOT NULL;
ALTER TABLE "product_review" ALTER COLUMN "customers_id" SET NOT NULL;
ALTER TABLE "product_review" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "customer_optin" ALTER COLUMN "optin_id" SET NOT NULL;
ALTER TABLE "file_history" ALTER COLUMN "file_id" SET NOT NULL;
ALTER TABLE "geozone_description" ALTER COLUMN "geozone_id" SET NOT NULL;
ALTER TABLE "orders" ALTER COLUMN "merchantid" SET NOT NULL;
ALTER TABLE "orders" ALTER COLUMN "currency_id" SET NOT NULL;
ALTER TABLE "orders" ALTER COLUMN "billing_zone_id" SET NOT NULL;
ALTER TABLE "orders" ALTER COLUMN "delivery_zone_id" SET NOT NULL;
ALTER TABLE "orders" ALTER COLUMN "delivery_country_id" SET NOT NULL;
ALTER TABLE "shipping_quote" ALTER COLUMN "delivery_country_id" SET NOT NULL;
ALTER TABLE "shipping_quote" ALTER COLUMN "delivery_zone_id" SET NOT NULL;
ALTER TABLE "merchant_configuration" ALTER COLUMN "config_key" SET NOT NULL;
ALTER TABLE "merchant_configuration" ALTER COLUMN "merchant_id" SET NOT NULL;
ALTER TABLE "optin" ALTER COLUMN "merchant_id" SET NOT NULL;
ALTER TABLE "product_relationship" ALTER COLUMN "related_product_id" SET NOT NULL;
ALTER TABLE "product_relationship" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "currency" ALTER COLUMN "currency_code" SET NOT NULL;
ALTER TABLE "currency" ALTER COLUMN "currency_name" SET NOT NULL;
ALTER TABLE "customer_review" ALTER COLUMN "reviewed_customer_id" SET NOT NULL;
ALTER TABLE "customer_review" ALTER COLUMN "customers_id" SET NOT NULL;
ALTER TABLE "product_review_description" ALTER COLUMN "product_review_id" SET NOT NULL;
ALTER TABLE "product" ALTER COLUMN "tax_class_id" SET NOT NULL;
ALTER TABLE "product" ALTER COLUMN "product_type_id" SET NOT NULL;
ALTER TABLE "product" ALTER COLUMN "customer_id" SET NOT NULL;
ALTER TABLE "shiping_origin" ALTER COLUMN "zone_id" SET NOT NULL;
ALTER TABLE "shiping_origin" ALTER COLUMN "country_id" SET NOT NULL;
ALTER TABLE "tax_class" ALTER COLUMN "merchant_id" SET NOT NULL;
ALTER TABLE "customer_opt_val_description" ALTER COLUMN "customer_opt_val_id" SET NOT NULL;
ALTER TABLE "category" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "customer_review_description" ALTER COLUMN "customer_review_id" SET NOT NULL;
ALTER TABLE "user" ALTER COLUMN "language_id" SET NOT NULL;
ALTER TABLE "customer" ALTER COLUMN "delivery_zone_id" SET NOT NULL;
ALTER TABLE "customer" ALTER COLUMN "delivery_country_id" SET NOT NULL;
ALTER TABLE "customer" ALTER COLUMN "billing_zone_id" SET NOT NULL;
ALTER TABLE "user_group" ADD CONSTRAINT "wetune_u_7e3a351071eaac89" UNIQUE ("user_id", "group_id");
ALTER TABLE "merchant_language" ADD CONSTRAINT "wetune_u_1cf5452602fa176e" UNIQUE ("stores_merchant_id", "languages_language_id");
ALTER TABLE "permission_group" ADD CONSTRAINT "wetune_u_700b7f1c737b332c" UNIQUE ("permission_id", "group_id");
ALTER TABLE "customer_group" ADD CONSTRAINT "wetune_u_329a36964c8df05a" UNIQUE ("customer_id", "group_id");
