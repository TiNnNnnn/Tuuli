CREATE TABLE "blc_additional_offer_info" (
  "blc_order_order_id" BIGINT NOT NULL,
  "offer_info_id" BIGINT NOT NULL,
  "offer_id" BIGINT NOT NULL,
  PRIMARY KEY ("blc_order_order_id", "offer_id")
);

CREATE TABLE "blc_address" (
  "address_id" BIGINT NOT NULL,
  "address_line1" VARCHAR(255) NOT NULL,
  "address_line2" VARCHAR(255),
  "address_line3" VARCHAR(255),
  "city" VARCHAR(255) NOT NULL,
  "company_name" VARCHAR(255),
  "county" VARCHAR(255),
  "email_address" VARCHAR(255),
  "fax" VARCHAR(255),
  "first_name" VARCHAR(255),
  "full_name" VARCHAR(255),
  "is_active" INTEGER,
  "is_business" INTEGER,
  "is_default" INTEGER,
  "is_mailing" INTEGER,
  "is_street" INTEGER,
  "iso_country_sub" VARCHAR(255),
  "last_name" VARCHAR(255),
  "postal_code" VARCHAR(255),
  "primary_phone" VARCHAR(255),
  "secondary_phone" VARCHAR(255),
  "standardized" INTEGER,
  "sub_state_prov_reg" VARCHAR(255),
  "tokenized_address" VARCHAR(255),
  "verification_level" VARCHAR(255),
  "zip_four" VARCHAR(255),
  "country" VARCHAR(255),
  "iso_country_alpha2" VARCHAR(255),
  "phone_fax_id" BIGINT,
  "phone_primary_id" BIGINT,
  "phone_secondary_id" BIGINT,
  "state_prov_region" VARCHAR(255),
  PRIMARY KEY ("address_id")
);

CREATE TABLE "blc_admin_module" (
  "admin_module_id" BIGINT NOT NULL,
  "display_order" INTEGER,
  "icon" VARCHAR(255),
  "module_key" VARCHAR(255) NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("admin_module_id")
);

CREATE TABLE "blc_admin_password_token" (
  "password_token" VARCHAR(255) NOT NULL,
  "admin_user_id" BIGINT NOT NULL,
  "create_date" TIMESTAMP NOT NULL,
  "token_used_date" TIMESTAMP,
  "token_used_flag" INTEGER NOT NULL,
  PRIMARY KEY ("password_token")
);

CREATE TABLE "blc_admin_permission" (
  "admin_permission_id" BIGINT NOT NULL,
  "description" VARCHAR(255) NOT NULL,
  "is_friendly" INTEGER,
  "name" VARCHAR(255) NOT NULL,
  "permission_type" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("admin_permission_id")
);

CREATE TABLE "blc_admin_permission_entity" (
  "admin_permission_entity_id" BIGINT NOT NULL,
  "ceiling_entity" VARCHAR(255) NOT NULL,
  "admin_permission_id" BIGINT,
  PRIMARY KEY ("admin_permission_entity_id")
);

CREATE TABLE "blc_admin_permission_xref" (
  "child_permission_id" BIGINT NOT NULL,
  "admin_permission_id" BIGINT NOT NULL
);

CREATE TABLE "blc_admin_role" (
  "admin_role_id" BIGINT NOT NULL,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "description" VARCHAR(255) NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("admin_role_id")
);

CREATE TABLE "blc_admin_role_permission_xref" (
  "admin_role_id" BIGINT NOT NULL,
  "admin_permission_id" BIGINT NOT NULL,
  PRIMARY KEY ("admin_permission_id", "admin_role_id")
);

CREATE TABLE "blc_admin_sec_perm_xref" (
  "admin_section_id" BIGINT NOT NULL,
  "admin_permission_id" BIGINT NOT NULL
);

CREATE TABLE "blc_admin_section" (
  "admin_section_id" BIGINT NOT NULL,
  "ceiling_entity" VARCHAR(255),
  "display_controller" VARCHAR(255),
  "display_order" INTEGER,
  "name" VARCHAR(255) NOT NULL,
  "section_key" VARCHAR(255) NOT NULL,
  "url" VARCHAR(255),
  "use_default_handler" INTEGER,
  "admin_module_id" BIGINT NOT NULL,
  PRIMARY KEY ("admin_section_id"),
  UNIQUE ("section_key")
);

CREATE TABLE "blc_admin_user" (
  "admin_user_id" BIGINT NOT NULL,
  "active_status_flag" INTEGER,
  "archived" CHAR(1),
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "email" VARCHAR(255) NOT NULL,
  "login" VARCHAR(255) NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "password" VARCHAR(255),
  "phone_number" VARCHAR(255),
  PRIMARY KEY ("admin_user_id")
);

CREATE TABLE "blc_admin_user_addtl_fields" (
  "attribute_id" BIGINT NOT NULL,
  "field_name" VARCHAR(255) NOT NULL,
  "field_value" VARCHAR(255),
  "admin_user_id" BIGINT NOT NULL,
  PRIMARY KEY ("attribute_id")
);

CREATE TABLE "blc_admin_user_permission_xref" (
  "admin_user_id" BIGINT NOT NULL,
  "admin_permission_id" BIGINT NOT NULL,
  PRIMARY KEY ("admin_permission_id", "admin_user_id")
);

CREATE TABLE "blc_admin_user_role_xref" (
  "admin_user_id" BIGINT NOT NULL,
  "admin_role_id" BIGINT NOT NULL,
  PRIMARY KEY ("admin_role_id", "admin_user_id")
);

CREATE TABLE "blc_admin_user_sandbox" (
  "sandbox_id" BIGINT,
  "admin_user_id" BIGINT NOT NULL,
  PRIMARY KEY ("admin_user_id")
);

CREATE TABLE "blc_asset_desc_map" (
  "static_asset_id" BIGINT NOT NULL,
  "static_asset_desc_id" BIGINT NOT NULL,
  "map_key" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("static_asset_id", "map_key")
);

CREATE TABLE "blc_bank_account_payment" (
  "payment_id" BIGINT NOT NULL,
  "account_number" VARCHAR(255) NOT NULL,
  "reference_number" VARCHAR(255) NOT NULL,
  "routing_number" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("payment_id")
);

CREATE TABLE "blc_bund_item_fee_price" (
  "bund_item_fee_price_id" BIGINT NOT NULL,
  "amount" DECIMAL(19, 5),
  "is_taxable" INTEGER,
  "name" VARCHAR(255),
  "reporting_code" VARCHAR(255),
  "bund_order_item_id" BIGINT NOT NULL,
  PRIMARY KEY ("bund_item_fee_price_id")
);

CREATE TABLE "blc_bundle_order_item" (
  "base_retail_price" DECIMAL(19, 5),
  "base_sale_price" DECIMAL(19, 5),
  "order_item_id" BIGINT NOT NULL,
  "product_bundle_id" BIGINT,
  "sku_id" BIGINT,
  PRIMARY KEY ("order_item_id")
);

CREATE TABLE "blc_candidate_fg_offer" (
  "candidate_fg_offer_id" BIGINT NOT NULL,
  "discounted_price" DECIMAL(19, 5),
  "fulfillment_group_id" BIGINT,
  "offer_id" BIGINT NOT NULL,
  PRIMARY KEY ("candidate_fg_offer_id")
);

CREATE TABLE "blc_candidate_item_offer" (
  "candidate_item_offer_id" BIGINT NOT NULL,
  "discounted_price" DECIMAL(19, 5),
  "offer_id" BIGINT NOT NULL,
  "order_item_id" BIGINT,
  PRIMARY KEY ("candidate_item_offer_id")
);

CREATE TABLE "blc_candidate_order_offer" (
  "candidate_order_offer_id" BIGINT NOT NULL,
  "discounted_price" DECIMAL(19, 5),
  "offer_id" BIGINT NOT NULL,
  "order_id" BIGINT,
  PRIMARY KEY ("candidate_order_offer_id")
);

CREATE TABLE "blc_cat_search_facet_excl_xref" (
  "cat_excl_search_facet_id" BIGINT NOT NULL,
  "sequence" DECIMAL(19, 2),
  "category_id" BIGINT,
  "search_facet_id" BIGINT,
  PRIMARY KEY ("cat_excl_search_facet_id")
);

CREATE TABLE "blc_cat_search_facet_xref" (
  "category_search_facet_id" BIGINT NOT NULL,
  "sequence" DECIMAL(19, 2),
  "category_id" BIGINT,
  "search_facet_id" BIGINT,
  PRIMARY KEY ("category_search_facet_id")
);

CREATE TABLE "blc_cat_site_map_gen_cfg" (
  "ending_depth" INTEGER NOT NULL,
  "starting_depth" INTEGER NOT NULL,
  "gen_config_id" BIGINT NOT NULL,
  "root_category_id" BIGINT NOT NULL,
  PRIMARY KEY ("gen_config_id")
);

CREATE TABLE "blc_catalog" (
  "catalog_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "name" VARCHAR(255),
  PRIMARY KEY ("catalog_id")
);

CREATE TABLE "blc_category" (
  "category_id" BIGINT NOT NULL,
  "active_end_date" TIMESTAMP,
  "active_start_date" TIMESTAMP,
  "archived" CHAR(1),
  "description" VARCHAR(255),
  "display_template" VARCHAR(255),
  "external_id" VARCHAR(255),
  "fulfillment_type" VARCHAR(255),
  "inventory_type" VARCHAR(255),
  "long_description" TEXT,
  "meta_desc" VARCHAR(255),
  "meta_title" VARCHAR(255),
  "name" VARCHAR(255) NOT NULL,
  "override_generated_url" INTEGER,
  "product_desc_pattern_override" VARCHAR(255),
  "product_title_pattern_override" VARCHAR(255),
  "root_display_order" DECIMAL(10, 6),
  "tax_code" VARCHAR(255),
  "url" VARCHAR(255),
  "url_key" VARCHAR(255),
  "default_parent_category_id" BIGINT,
  PRIMARY KEY ("category_id")
);

CREATE TABLE "blc_category_attribute" (
  "category_attribute_id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255),
  "category_id" BIGINT NOT NULL,
  PRIMARY KEY ("category_attribute_id")
);

CREATE TABLE "blc_category_media_map" (
  "category_media_id" BIGINT NOT NULL,
  "map_key" VARCHAR(255) NOT NULL,
  "blc_category_category_id" BIGINT NOT NULL,
  "media_id" BIGINT,
  PRIMARY KEY ("category_media_id")
);

CREATE TABLE "blc_category_product_xref" (
  "category_product_id" BIGINT NOT NULL,
  "default_reference" INTEGER,
  "display_order" DECIMAL(10, 6),
  "category_id" BIGINT NOT NULL,
  "product_id" BIGINT NOT NULL,
  PRIMARY KEY ("category_product_id")
);

CREATE TABLE "blc_category_xref" (
  "category_xref_id" BIGINT NOT NULL,
  "default_reference" INTEGER,
  "display_order" DECIMAL(10, 6),
  "category_id" BIGINT NOT NULL,
  "sub_category_id" BIGINT NOT NULL,
  PRIMARY KEY ("category_xref_id")
);

CREATE TABLE "blc_challenge_question" (
  "question_id" BIGINT NOT NULL,
  "question" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("question_id")
);

CREATE TABLE "blc_cms_menu" (
  "menu_id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("menu_id")
);

CREATE TABLE "blc_cms_menu_item" (
  "menu_item_id" BIGINT NOT NULL,
  "action_url" VARCHAR(255),
  "alt_text" VARCHAR(255),
  "custom_html" TEXT,
  "image_url" VARCHAR(255),
  "label" VARCHAR(255),
  "sequence" DECIMAL(10, 6),
  "menu_item_type" VARCHAR(255),
  "linked_menu_id" BIGINT,
  "linked_page_id" BIGINT,
  "parent_menu_id" BIGINT,
  PRIMARY KEY ("menu_item_id")
);

CREATE TABLE "blc_code_types" (
  "code_id" BIGINT NOT NULL,
  "code_type" VARCHAR(255) NOT NULL,
  "code_desc" VARCHAR(255),
  "code_key" VARCHAR(255) NOT NULL,
  "modifiable" CHAR(1),
  PRIMARY KEY ("code_id")
);

CREATE TABLE "blc_country" (
  "abbreviation" VARCHAR(255) NOT NULL,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("abbreviation")
);

CREATE TABLE "blc_country_sub" (
  "abbreviation" VARCHAR(255) NOT NULL,
  "alt_abbreviation" VARCHAR(255),
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "name" VARCHAR(255) NOT NULL,
  "country_sub_cat" BIGINT,
  "country" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("abbreviation")
);

CREATE TABLE "blc_country_sub_cat" (
  "country_sub_cat_id" BIGINT NOT NULL,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("country_sub_cat_id")
);

CREATE TABLE "blc_credit_card_payment" (
  "payment_id" BIGINT NOT NULL,
  "expiration_month" INTEGER NOT NULL,
  "expiration_year" INTEGER NOT NULL,
  "name_on_card" VARCHAR(255) NOT NULL,
  "pan" VARCHAR(255) NOT NULL,
  "reference_number" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("payment_id")
);

CREATE TABLE "blc_currency" (
  "currency_code" VARCHAR(255) NOT NULL,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "default_flag" INTEGER,
  "friendly_name" VARCHAR(255),
  PRIMARY KEY ("currency_code")
);

CREATE TABLE "blc_cust_site_map_gen_cfg" (
  "gen_config_id" BIGINT NOT NULL,
  PRIMARY KEY ("gen_config_id")
);

CREATE TABLE "blc_customer" (
  "customer_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "challenge_answer" VARCHAR(255),
  "deactivated" INTEGER,
  "email_address" VARCHAR(255),
  "external_id" VARCHAR(255),
  "first_name" VARCHAR(255),
  "is_tax_exempt" INTEGER,
  "last_name" VARCHAR(255),
  "password" VARCHAR(255),
  "password_change_required" INTEGER,
  "is_preview" INTEGER,
  "receive_email" INTEGER,
  "is_registered" INTEGER,
  "tax_exemption_code" VARCHAR(255),
  "user_name" VARCHAR(255),
  "challenge_question_id" BIGINT,
  "locale_code" VARCHAR(255),
  PRIMARY KEY ("customer_id")
);

CREATE TABLE "blc_customer_address" (
  "customer_address_id" BIGINT NOT NULL,
  "address_name" VARCHAR(255),
  "archived" CHAR(1),
  "address_id" BIGINT NOT NULL,
  "customer_id" BIGINT NOT NULL,
  PRIMARY KEY ("customer_address_id")
);

CREATE TABLE "blc_customer_attribute" (
  "customer_attr_id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255),
  "customer_id" BIGINT NOT NULL,
  PRIMARY KEY ("customer_attr_id")
);

CREATE TABLE "blc_customer_offer_xref" (
  "customer_offer_id" BIGINT NOT NULL,
  "customer_id" BIGINT NOT NULL,
  "offer_id" BIGINT NOT NULL,
  PRIMARY KEY ("customer_offer_id")
);

CREATE TABLE "blc_customer_password_token" (
  "password_token" VARCHAR(255) NOT NULL,
  "create_date" TIMESTAMP NOT NULL,
  "customer_id" BIGINT NOT NULL,
  "token_used_date" TIMESTAMP,
  "token_used_flag" INTEGER NOT NULL,
  PRIMARY KEY ("password_token")
);

CREATE TABLE "blc_customer_payment" (
  "customer_payment_id" BIGINT NOT NULL,
  "is_default" INTEGER,
  "gateway_type" VARCHAR(255),
  "payment_token" VARCHAR(255),
  "payment_type" VARCHAR(255),
  "address_id" BIGINT,
  "customer_id" BIGINT NOT NULL,
  PRIMARY KEY ("customer_payment_id"),
  UNIQUE ("customer_id", "payment_token")
);

CREATE TABLE "blc_customer_payment_fields" (
  "customer_payment_id" BIGINT NOT NULL,
  "field_value" TEXT,
  "field_name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("customer_payment_id", "field_name")
);

CREATE TABLE "blc_customer_phone" (
  "customer_phone_id" BIGINT NOT NULL,
  "phone_name" VARCHAR(255),
  "customer_id" BIGINT NOT NULL,
  "phone_id" BIGINT NOT NULL,
  PRIMARY KEY ("customer_phone_id"),
  UNIQUE ("customer_id", "phone_name")
);

CREATE TABLE "blc_customer_role" (
  "customer_role_id" BIGINT NOT NULL,
  "customer_id" BIGINT NOT NULL,
  "role_id" BIGINT NOT NULL,
  PRIMARY KEY ("customer_role_id")
);

CREATE TABLE "blc_data_drvn_enum" (
  "enum_id" BIGINT NOT NULL,
  "enum_key" VARCHAR(255),
  "modifiable" INTEGER,
  PRIMARY KEY ("enum_id")
);

CREATE TABLE "blc_data_drvn_enum_val" (
  "enum_val_id" BIGINT NOT NULL,
  "display" VARCHAR(255),
  "hidden" INTEGER,
  "enum_key" VARCHAR(255),
  "enum_type" BIGINT,
  PRIMARY KEY ("enum_val_id")
);

CREATE TABLE "blc_disc_item_fee_price" (
  "disc_item_fee_price_id" BIGINT NOT NULL,
  "amount" DECIMAL(19, 5),
  "name" VARCHAR(255),
  "reporting_code" VARCHAR(255),
  "order_item_id" BIGINT NOT NULL,
  PRIMARY KEY ("disc_item_fee_price_id")
);

CREATE TABLE "blc_discrete_order_item" (
  "base_retail_price" DECIMAL(19, 5),
  "base_sale_price" DECIMAL(19, 5),
  "order_item_id" BIGINT NOT NULL,
  "bundle_order_item_id" BIGINT,
  "product_id" BIGINT,
  "sku_id" BIGINT NOT NULL,
  "sku_bundle_item_id" BIGINT,
  PRIMARY KEY ("order_item_id")
);

CREATE TABLE "blc_dyn_discrete_order_item" (
  "order_item_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_item_id")
);

CREATE TABLE "blc_email_tracking" (
  "email_tracking_id" BIGINT NOT NULL,
  "date_sent" TIMESTAMP,
  "email_address" VARCHAR(255),
  "type" VARCHAR(255),
  PRIMARY KEY ("email_tracking_id")
);

CREATE TABLE "blc_email_tracking_clicks" (
  "click_id" BIGINT NOT NULL,
  "customer_id" VARCHAR(255),
  "date_clicked" TIMESTAMP NOT NULL,
  "destination_uri" VARCHAR(255),
  "query_string" VARCHAR(255),
  "email_tracking_id" BIGINT NOT NULL,
  PRIMARY KEY ("click_id")
);

CREATE TABLE "blc_email_tracking_opens" (
  "open_id" BIGINT NOT NULL,
  "date_opened" TIMESTAMP,
  "user_agent" VARCHAR(255),
  "email_tracking_id" BIGINT,
  PRIMARY KEY ("open_id")
);

CREATE TABLE "blc_fg_adjustment" (
  "fg_adjustment_id" BIGINT NOT NULL,
  "adjustment_reason" VARCHAR(255) NOT NULL,
  "adjustment_value" DECIMAL(19, 5) NOT NULL,
  "fulfillment_group_id" BIGINT,
  "offer_id" BIGINT NOT NULL,
  PRIMARY KEY ("fg_adjustment_id")
);

CREATE TABLE "blc_fg_fee_tax_xref" (
  "fulfillment_group_fee_id" BIGINT NOT NULL,
  "tax_detail_id" BIGINT NOT NULL,
  UNIQUE ("tax_detail_id")
);

CREATE TABLE "blc_fg_fg_tax_xref" (
  "fulfillment_group_id" BIGINT NOT NULL,
  "tax_detail_id" BIGINT NOT NULL,
  UNIQUE ("tax_detail_id")
);

CREATE TABLE "blc_fg_item_tax_xref" (
  "fulfillment_group_item_id" BIGINT NOT NULL,
  "tax_detail_id" BIGINT NOT NULL,
  UNIQUE ("tax_detail_id")
);

CREATE TABLE "blc_field" (
  "field_id" BIGINT NOT NULL,
  "abbreviation" VARCHAR(255),
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "entity_type" VARCHAR(255) NOT NULL,
  "friendly_name" VARCHAR(255),
  "override_generated_prop_name" INTEGER,
  "property_name" VARCHAR(255) NOT NULL,
  "translatable" INTEGER,
  PRIMARY KEY ("field_id")
);

CREATE TABLE "blc_fld_def" (
  "fld_def_id" BIGINT NOT NULL,
  "allow_multiples" INTEGER,
  "column_width" VARCHAR(255),
  "fld_order" INTEGER,
  "fld_type" VARCHAR(255),
  "friendly_name" VARCHAR(255),
  "help_text" VARCHAR(255),
  "hidden_flag" INTEGER,
  "hint" VARCHAR(255),
  "max_length" INTEGER,
  "name" VARCHAR(255),
  "required_flag" INTEGER,
  "security_level" VARCHAR(255),
  "text_area_flag" INTEGER,
  "tooltip" VARCHAR(255),
  "vldtn_error_mssg_key" VARCHAR(255),
  "vldtn_regex" VARCHAR(255),
  "enum_id" BIGINT,
  "fld_group_id" BIGINT,
  PRIMARY KEY ("fld_def_id")
);

CREATE TABLE "blc_fld_enum" (
  "fld_enum_id" BIGINT NOT NULL,
  "name" VARCHAR(255),
  PRIMARY KEY ("fld_enum_id")
);

CREATE TABLE "blc_fld_enum_item" (
  "fld_enum_item_id" BIGINT NOT NULL,
  "fld_order" INTEGER,
  "friendly_name" VARCHAR(255),
  "name" VARCHAR(255),
  "fld_enum_id" BIGINT,
  PRIMARY KEY ("fld_enum_item_id")
);

CREATE TABLE "blc_fld_group" (
  "fld_group_id" BIGINT NOT NULL,
  "init_collapsed_flag" INTEGER,
  "is_master_field_group" INTEGER,
  "name" VARCHAR(255),
  PRIMARY KEY ("fld_group_id")
);

CREATE TABLE "blc_fulfillment_group" (
  "fulfillment_group_id" BIGINT NOT NULL,
  "delivery_instruction" VARCHAR(255),
  "price" DECIMAL(19, 5),
  "shipping_price_taxable" INTEGER,
  "merchandise_total" DECIMAL(19, 5),
  "method" VARCHAR(255),
  "is_primary" INTEGER,
  "reference_number" VARCHAR(255),
  "retail_price" DECIMAL(19, 5),
  "sale_price" DECIMAL(19, 5),
  "fulfillment_group_sequnce" INTEGER,
  "service" VARCHAR(255),
  "shipping_override" INTEGER,
  "status" VARCHAR(255),
  "total" DECIMAL(19, 5),
  "total_fee_tax" DECIMAL(19, 5),
  "total_fg_tax" DECIMAL(19, 5),
  "total_item_tax" DECIMAL(19, 5),
  "total_tax" DECIMAL(19, 5),
  "type" VARCHAR(255),
  "address_id" BIGINT,
  "fulfillment_option_id" BIGINT,
  "order_id" BIGINT NOT NULL,
  "personal_message_id" BIGINT,
  "phone_id" BIGINT,
  PRIMARY KEY ("fulfillment_group_id")
);

CREATE TABLE "blc_fulfillment_group_fee" (
  "fulfillment_group_fee_id" BIGINT NOT NULL,
  "amount" DECIMAL(19, 5),
  "fee_taxable_flag" INTEGER,
  "name" VARCHAR(255),
  "reporting_code" VARCHAR(255),
  "total_fee_tax" DECIMAL(19, 5),
  "fulfillment_group_id" BIGINT NOT NULL,
  PRIMARY KEY ("fulfillment_group_fee_id")
);

CREATE TABLE "blc_fulfillment_group_item" (
  "fulfillment_group_item_id" BIGINT NOT NULL,
  "prorated_order_adj" DECIMAL(19, 2),
  "quantity" INTEGER NOT NULL,
  "status" VARCHAR(255),
  "total_item_amount" DECIMAL(19, 5),
  "total_item_taxable_amount" DECIMAL(19, 5),
  "total_item_tax" DECIMAL(19, 5),
  "fulfillment_group_id" BIGINT NOT NULL,
  "order_item_id" BIGINT NOT NULL,
  PRIMARY KEY ("fulfillment_group_item_id")
);

CREATE TABLE "blc_fulfillment_opt_banded_prc" (
  "fulfillment_option_id" BIGINT NOT NULL,
  PRIMARY KEY ("fulfillment_option_id")
);

CREATE TABLE "blc_fulfillment_opt_banded_wgt" (
  "fulfillment_option_id" BIGINT NOT NULL,
  PRIMARY KEY ("fulfillment_option_id")
);

CREATE TABLE "blc_fulfillment_option" (
  "fulfillment_option_id" BIGINT NOT NULL,
  "fulfillment_type" VARCHAR(255) NOT NULL,
  "long_description" TEXT,
  "name" VARCHAR(255),
  "tax_code" VARCHAR(255),
  "taxable" INTEGER,
  "use_flat_rates" INTEGER,
  PRIMARY KEY ("fulfillment_option_id")
);

CREATE TABLE "blc_fulfillment_option_fixed" (
  "price" DECIMAL(19, 5) NOT NULL,
  "fulfillment_option_id" BIGINT NOT NULL,
  "currency_code" VARCHAR(255),
  PRIMARY KEY ("fulfillment_option_id")
);

CREATE TABLE "blc_fulfillment_price_band" (
  "fulfillment_price_band_id" BIGINT NOT NULL,
  "result_amount" DECIMAL(19, 5) NOT NULL,
  "result_amount_type" VARCHAR(255) NOT NULL,
  "retail_price_minimum_amount" DECIMAL(19, 5) NOT NULL,
  "fulfillment_option_id" BIGINT,
  PRIMARY KEY ("fulfillment_price_band_id")
);

CREATE TABLE "blc_fulfillment_weight_band" (
  "fulfillment_weight_band_id" BIGINT NOT NULL,
  "result_amount" DECIMAL(19, 5) NOT NULL,
  "result_amount_type" VARCHAR(255) NOT NULL,
  "minimum_weight" DECIMAL(19, 5),
  "weight_unit_of_measure" VARCHAR(255),
  "fulfillment_option_id" BIGINT,
  PRIMARY KEY ("fulfillment_weight_band_id")
);

CREATE TABLE "blc_gift_card_payment" (
  "payment_id" BIGINT NOT NULL,
  "pan" VARCHAR(255) NOT NULL,
  "pin" VARCHAR(255),
  "reference_number" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("payment_id")
);

CREATE TABLE "blc_giftwrap_order_item" (
  "order_item_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_item_id")
);

CREATE TABLE "blc_id_generation" (
  "id_type" VARCHAR(255) NOT NULL,
  "batch_size" BIGINT NOT NULL,
  "batch_start" BIGINT NOT NULL,
  "id_min" BIGINT,
  "id_max" BIGINT,
  "version" INTEGER,
  PRIMARY KEY ("id_type")
);

CREATE TABLE "blc_img_static_asset" (
  "height" INTEGER,
  "width" INTEGER,
  "static_asset_id" BIGINT NOT NULL,
  PRIMARY KEY ("static_asset_id")
);

CREATE TABLE "blc_index_field" (
  "index_field_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "searchable" INTEGER,
  "field_id" BIGINT NOT NULL,
  PRIMARY KEY ("index_field_id")
);

CREATE TABLE "blc_index_field_type" (
  "index_field_type_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "field_type" VARCHAR(255),
  "index_field_id" BIGINT NOT NULL,
  PRIMARY KEY ("index_field_type_id")
);

CREATE TABLE "blc_iso_country" (
  "alpha_2" VARCHAR(255) NOT NULL,
  "alpha_3" VARCHAR(255),
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "name" VARCHAR(255),
  "numeric_code" INTEGER,
  "status" VARCHAR(255),
  PRIMARY KEY ("alpha_2")
);

CREATE TABLE "blc_item_offer_qualifier" (
  "item_offer_qualifier_id" BIGINT NOT NULL,
  "quantity" BIGINT,
  "offer_id" BIGINT NOT NULL,
  "order_item_id" BIGINT,
  PRIMARY KEY ("item_offer_qualifier_id")
);

CREATE TABLE "blc_locale" (
  "locale_code" VARCHAR(255) NOT NULL,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "default_flag" INTEGER,
  "friendly_name" VARCHAR(255),
  "use_in_search_index" INTEGER,
  "currency_code" VARCHAR(255),
  PRIMARY KEY ("locale_code")
);

CREATE TABLE "blc_media" (
  "media_id" BIGINT NOT NULL,
  "alt_text" VARCHAR(255),
  "tags" VARCHAR(255),
  "title" VARCHAR(255),
  "url" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("media_id")
);

CREATE TABLE "blc_module_configuration" (
  "module_config_id" BIGINT NOT NULL,
  "active_end_date" TIMESTAMP,
  "active_start_date" TIMESTAMP,
  "archived" CHAR(1),
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "config_type" VARCHAR(255) NOT NULL,
  "is_default" INTEGER NOT NULL,
  "module_name" VARCHAR(255) NOT NULL,
  "module_priority" INTEGER NOT NULL,
  PRIMARY KEY ("module_config_id")
);

CREATE TABLE "blc_offer" (
  "offer_id" BIGINT NOT NULL,
  "apply_to_child_items" INTEGER,
  "apply_to_sale_price" INTEGER,
  "archived" CHAR(1),
  "automatically_added" INTEGER,
  "combinable_with_other_offers" INTEGER,
  "offer_description" VARCHAR(255),
  "offer_discount_type" VARCHAR(255),
  "end_date" TIMESTAMP,
  "marketing_messasge" VARCHAR(255),
  "max_uses_per_customer" BIGINT,
  "max_uses" INTEGER,
  "offer_name" VARCHAR(255) NOT NULL,
  "offer_item_qualifier_rule" VARCHAR(255),
  "offer_item_target_rule" VARCHAR(255),
  "order_min_total" DECIMAL(19, 5),
  "offer_priority" INTEGER,
  "qualifying_item_min_total" DECIMAL(19, 5),
  "requires_related_tar_qual" INTEGER,
  "start_date" TIMESTAMP,
  "target_min_total" DECIMAL(19, 5),
  "target_system" VARCHAR(255),
  "totalitarian_offer" INTEGER,
  "offer_type" VARCHAR(255) NOT NULL,
  "offer_value" DECIMAL(19, 5) NOT NULL,
  PRIMARY KEY ("offer_id")
);

CREATE TABLE "blc_offer_audit" (
  "offer_audit_id" BIGINT NOT NULL,
  "customer_id" BIGINT,
  "offer_code_id" BIGINT,
  "offer_id" BIGINT,
  "order_id" BIGINT,
  "redeemed_date" TIMESTAMP,
  PRIMARY KEY ("offer_audit_id")
);

CREATE TABLE "blc_offer_code" (
  "offer_code_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "email_address" VARCHAR(255),
  "max_uses" INTEGER,
  "offer_code" VARCHAR(255) NOT NULL,
  "end_date" TIMESTAMP,
  "start_date" TIMESTAMP,
  "uses" INTEGER,
  "offer_id" BIGINT NOT NULL,
  PRIMARY KEY ("offer_code_id")
);

CREATE TABLE "blc_offer_info" (
  "offer_info_id" BIGINT NOT NULL,
  PRIMARY KEY ("offer_info_id")
);

CREATE TABLE "blc_offer_info_fields" (
  "offer_info_fields_id" BIGINT NOT NULL,
  "field_value" VARCHAR(255),
  "field_name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("offer_info_fields_id", "field_name")
);

CREATE TABLE "blc_offer_item_criteria" (
  "offer_item_criteria_id" BIGINT NOT NULL,
  "order_item_match_rule" TEXT,
  "quantity" INTEGER NOT NULL,
  PRIMARY KEY ("offer_item_criteria_id")
);

CREATE TABLE "blc_offer_rule" (
  "offer_rule_id" BIGINT NOT NULL,
  "match_rule" TEXT,
  PRIMARY KEY ("offer_rule_id")
);

CREATE TABLE "blc_offer_rule_map" (
  "offer_offer_rule_id" BIGINT NOT NULL,
  "map_key" VARCHAR(255) NOT NULL,
  "blc_offer_offer_id" BIGINT NOT NULL,
  "offer_rule_id" BIGINT,
  PRIMARY KEY ("offer_offer_rule_id")
);

CREATE TABLE "blc_order" (
  "order_id" BIGINT NOT NULL,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "email_address" VARCHAR(255),
  "name" VARCHAR(255),
  "order_number" VARCHAR(255),
  "is_preview" INTEGER,
  "order_status" VARCHAR(255),
  "order_subtotal" DECIMAL(19, 5),
  "submit_date" TIMESTAMP,
  "tax_override" INTEGER,
  "order_total" DECIMAL(19, 5),
  "total_shipping" DECIMAL(19, 5),
  "total_tax" DECIMAL(19, 5),
  "currency_code" VARCHAR(255),
  "customer_id" BIGINT NOT NULL,
  "locale_code" VARCHAR(255),
  PRIMARY KEY ("order_id")
);

CREATE TABLE "blc_order_adjustment" (
  "order_adjustment_id" BIGINT NOT NULL,
  "adjustment_reason" VARCHAR(255) NOT NULL,
  "adjustment_value" DECIMAL(19, 5) NOT NULL,
  "offer_id" BIGINT NOT NULL,
  "order_id" BIGINT,
  PRIMARY KEY ("order_adjustment_id")
);

CREATE TABLE "blc_order_attribute" (
  "order_attribute_id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255),
  "order_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_attribute_id"),
  UNIQUE ("name", "order_id")
);

CREATE TABLE "blc_order_item" (
  "order_item_id" BIGINT NOT NULL,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "discounts_allowed" INTEGER,
  "has_validation_errors" INTEGER,
  "item_taxable_flag" INTEGER,
  "name" VARCHAR(255),
  "order_item_type" VARCHAR(255),
  "price" DECIMAL(19, 5),
  "quantity" INTEGER NOT NULL,
  "retail_price" DECIMAL(19, 5),
  "retail_price_override" INTEGER,
  "sale_price" DECIMAL(19, 5),
  "sale_price_override" INTEGER,
  "total_tax" DECIMAL(19, 2),
  "category_id" BIGINT,
  "gift_wrap_item_id" BIGINT,
  "order_id" BIGINT,
  "parent_order_item_id" BIGINT,
  "personal_message_id" BIGINT,
  PRIMARY KEY ("order_item_id")
);

CREATE TABLE "blc_order_item_add_attr" (
  "order_item_id" BIGINT NOT NULL,
  "value" VARCHAR(255),
  "name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("order_item_id", "name")
);

CREATE TABLE "blc_order_item_adjustment" (
  "order_item_adjustment_id" BIGINT NOT NULL,
  "applied_to_sale_price" INTEGER,
  "adjustment_reason" VARCHAR(255) NOT NULL,
  "adjustment_value" DECIMAL(19, 5) NOT NULL,
  "offer_id" BIGINT NOT NULL,
  "order_item_id" BIGINT,
  PRIMARY KEY ("order_item_adjustment_id")
);

CREATE TABLE "blc_order_item_attribute" (
  "order_item_attribute_id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255) NOT NULL,
  "order_item_id" BIGINT NOT NULL,
  PRIMARY KEY ("order_item_attribute_id"),
  UNIQUE ("name", "order_item_id")
);

CREATE TABLE "blc_order_item_cart_message" (
  "order_item_id" BIGINT NOT NULL,
  "cart_message" VARCHAR(255)
);

CREATE TABLE "blc_order_item_dtl_adj" (
  "order_item_dtl_adj_id" BIGINT NOT NULL,
  "applied_to_sale_price" INTEGER,
  "offer_name" VARCHAR(255),
  "adjustment_reason" VARCHAR(255) NOT NULL,
  "adjustment_value" DECIMAL(19, 5) NOT NULL,
  "offer_id" BIGINT NOT NULL,
  "order_item_price_dtl_id" BIGINT,
  PRIMARY KEY ("order_item_dtl_adj_id")
);

CREATE TABLE "blc_order_item_price_dtl" (
  "order_item_price_dtl_id" BIGINT NOT NULL,
  "quantity" INTEGER NOT NULL,
  "use_sale_price" INTEGER,
  "order_item_id" BIGINT,
  PRIMARY KEY ("order_item_price_dtl_id")
);

CREATE TABLE "blc_order_lock" (
  "lock_key" VARCHAR(255) NOT NULL,
  "order_id" BIGINT NOT NULL,
  "last_updated" BIGINT,
  "locked" CHAR(1),
  PRIMARY KEY ("lock_key", "order_id")
);

CREATE TABLE "blc_order_multiship_option" (
  "order_multiship_option_id" BIGINT NOT NULL,
  "address_id" BIGINT,
  "fulfillment_option_id" BIGINT,
  "order_id" BIGINT,
  "order_item_id" BIGINT,
  PRIMARY KEY ("order_multiship_option_id")
);

CREATE TABLE "blc_order_offer_code_xref" (
  "order_id" BIGINT NOT NULL,
  "offer_code_id" BIGINT NOT NULL
);

CREATE TABLE "blc_order_payment" (
  "order_payment_id" BIGINT NOT NULL,
  "amount" DECIMAL(19, 5),
  "archived" CHAR(1),
  "gateway_type" VARCHAR(255),
  "reference_number" VARCHAR(255),
  "payment_type" VARCHAR(255) NOT NULL,
  "address_id" BIGINT,
  "order_id" BIGINT,
  PRIMARY KEY ("order_payment_id")
);

CREATE TABLE "blc_order_payment_transaction" (
  "payment_transaction_id" BIGINT NOT NULL,
  "transaction_amount" DECIMAL(19, 2),
  "archived" CHAR(1),
  "customer_ip_address" VARCHAR(255),
  "date_recorded" TIMESTAMP,
  "raw_response" TEXT,
  "save_token" INTEGER,
  "success" INTEGER,
  "transaction_type" VARCHAR(255),
  "order_payment" BIGINT NOT NULL,
  "parent_transaction" BIGINT,
  PRIMARY KEY ("payment_transaction_id")
);

CREATE TABLE "blc_page" (
  "page_id" BIGINT NOT NULL,
  "active_end_date" TIMESTAMP,
  "active_start_date" TIMESTAMP,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "description" VARCHAR(255),
  "exclude_from_site_map" INTEGER,
  "full_url" VARCHAR(255),
  "meta_description" VARCHAR(255),
  "meta_title" VARCHAR(255),
  "offline_flag" INTEGER,
  "priority" INTEGER,
  "page_tmplt_id" BIGINT,
  PRIMARY KEY ("page_id")
);

CREATE TABLE "blc_page_attributes" (
  "attribute_id" BIGINT NOT NULL,
  "field_name" VARCHAR(255) NOT NULL,
  "field_value" VARCHAR(255),
  "page_id" BIGINT NOT NULL,
  PRIMARY KEY ("attribute_id")
);

CREATE TABLE "blc_page_fld" (
  "page_fld_id" BIGINT NOT NULL,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "fld_key" VARCHAR(255),
  "lob_value" TEXT,
  "value" VARCHAR(255),
  "page_id" BIGINT NOT NULL,
  PRIMARY KEY ("page_fld_id")
);

CREATE TABLE "blc_page_item_criteria" (
  "page_item_criteria_id" BIGINT NOT NULL,
  "order_item_match_rule" TEXT,
  "quantity" INTEGER NOT NULL,
  PRIMARY KEY ("page_item_criteria_id")
);

CREATE TABLE "blc_page_rule" (
  "page_rule_id" BIGINT NOT NULL,
  "match_rule" TEXT,
  PRIMARY KEY ("page_rule_id")
);

CREATE TABLE "blc_page_rule_map" (
  "blc_page_page_id" BIGINT NOT NULL,
  "page_rule_id" BIGINT NOT NULL,
  "map_key" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("blc_page_page_id", "map_key")
);

CREATE TABLE "blc_page_tmplt" (
  "page_tmplt_id" BIGINT NOT NULL,
  "tmplt_descr" VARCHAR(255),
  "tmplt_name" VARCHAR(255),
  "tmplt_path" VARCHAR(255),
  "locale_code" VARCHAR(255),
  PRIMARY KEY ("page_tmplt_id")
);

CREATE TABLE "blc_payment_log" (
  "payment_log_id" BIGINT NOT NULL,
  "amount_paid" DECIMAL(19, 5),
  "exception_message" VARCHAR(255),
  "log_type" VARCHAR(255) NOT NULL,
  "order_payment_id" BIGINT,
  "order_payment_ref_num" VARCHAR(255),
  "transaction_success" INTEGER,
  "transaction_timestamp" TIMESTAMP NOT NULL,
  "transaction_type" VARCHAR(255) NOT NULL,
  "user_name" VARCHAR(255) NOT NULL,
  "currency_code" VARCHAR(255),
  "customer_id" BIGINT,
  PRIMARY KEY ("payment_log_id")
);

CREATE TABLE "blc_personal_message" (
  "personal_message_id" BIGINT NOT NULL,
  "message" VARCHAR(255),
  "message_from" VARCHAR(255),
  "message_to" VARCHAR(255),
  "occasion" VARCHAR(255),
  PRIMARY KEY ("personal_message_id")
);

CREATE TABLE "blc_pgtmplt_fldgrp_xref" (
  "pg_tmplt_fld_grp_id" BIGINT NOT NULL,
  "group_order" DECIMAL(10, 6),
  "fld_group_id" BIGINT,
  "page_tmplt_id" BIGINT,
  PRIMARY KEY ("pg_tmplt_fld_grp_id")
);

CREATE TABLE "blc_phone" (
  "phone_id" BIGINT NOT NULL,
  "country_code" VARCHAR(255),
  "extension" VARCHAR(255),
  "is_active" INTEGER,
  "is_default" INTEGER,
  "phone_number" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("phone_id")
);

CREATE TABLE "blc_product" (
  "product_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "can_sell_without_options" INTEGER,
  "canonical_url" VARCHAR(255),
  "display_template" VARCHAR(255),
  "is_featured_product" INTEGER NOT NULL,
  "manufacture" VARCHAR(255),
  "meta_desc" VARCHAR(255),
  "meta_title" VARCHAR(255),
  "model" VARCHAR(255),
  "override_generated_url" INTEGER,
  "url" VARCHAR(255),
  "url_key" VARCHAR(255),
  "default_category_id" BIGINT,
  "default_sku_id" BIGINT,
  PRIMARY KEY ("product_id")
);

CREATE TABLE "blc_product_attribute" (
  "product_attribute_id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255),
  "product_id" BIGINT NOT NULL,
  PRIMARY KEY ("product_attribute_id")
);

CREATE TABLE "blc_product_bundle" (
  "auto_bundle" INTEGER,
  "bundle_promotable" INTEGER,
  "items_promotable" INTEGER,
  "pricing_model" VARCHAR(255),
  "bundle_priority" INTEGER,
  "product_id" BIGINT NOT NULL,
  PRIMARY KEY ("product_id")
);

CREATE TABLE "blc_product_cross_sale" (
  "cross_sale_product_id" BIGINT NOT NULL,
  "promotion_message" VARCHAR(255),
  "sequence" DECIMAL(10, 6),
  "category_id" BIGINT,
  "product_id" BIGINT,
  "related_sale_product_id" BIGINT NOT NULL,
  PRIMARY KEY ("cross_sale_product_id")
);

CREATE TABLE "blc_product_featured" (
  "featured_product_id" BIGINT NOT NULL,
  "promotion_message" VARCHAR(255),
  "sequence" DECIMAL(10, 6),
  "category_id" BIGINT,
  "product_id" BIGINT,
  PRIMARY KEY ("featured_product_id")
);

CREATE TABLE "blc_product_option" (
  "product_option_id" BIGINT NOT NULL,
  "attribute_name" VARCHAR(255),
  "display_order" INTEGER,
  "error_code" VARCHAR(255),
  "error_message" VARCHAR(255),
  "label" VARCHAR(255),
  "name" VARCHAR(255),
  "validation_strategy_type" VARCHAR(255),
  "validation_type" VARCHAR(255),
  "required" INTEGER,
  "option_type" VARCHAR(255),
  "use_in_sku_generation" INTEGER,
  "validation_string" VARCHAR(255),
  PRIMARY KEY ("product_option_id")
);

CREATE TABLE "blc_product_option_value" (
  "product_option_value_id" BIGINT NOT NULL,
  "attribute_value" VARCHAR(255),
  "display_order" BIGINT,
  "price_adjustment" DECIMAL(19, 5),
  "product_option_id" BIGINT,
  PRIMARY KEY ("product_option_value_id")
);

CREATE TABLE "blc_product_option_xref" (
  "product_option_xref_id" BIGINT NOT NULL,
  "product_id" BIGINT NOT NULL,
  "product_option_id" BIGINT NOT NULL,
  PRIMARY KEY ("product_option_xref_id")
);

CREATE TABLE "blc_product_up_sale" (
  "up_sale_product_id" BIGINT NOT NULL,
  "promotion_message" VARCHAR(255),
  "sequence" DECIMAL(10, 6),
  "category_id" BIGINT,
  "product_id" BIGINT,
  "related_sale_product_id" BIGINT,
  PRIMARY KEY ("up_sale_product_id")
);

CREATE TABLE "blc_promotion_message" (
  "promotion_message_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "end_date" TIMESTAMP,
  "promotion_messasge" VARCHAR(255),
  "message_placement" VARCHAR(255),
  "name" VARCHAR(255),
  "promotion_message_priority" INTEGER,
  "start_date" TIMESTAMP,
  "locale_code" VARCHAR(255),
  "media_id" BIGINT,
  PRIMARY KEY ("promotion_message_id")
);

CREATE TABLE "blc_prorated_order_item_adjust" (
  "prorated_order_item_adjust_id" BIGINT NOT NULL,
  "prorated_quantity" INTEGER NOT NULL,
  "adjustment_reason" VARCHAR(255) NOT NULL,
  "prorated_adjustment_value" DECIMAL(19, 5) NOT NULL,
  "offer_id" BIGINT NOT NULL,
  "order_item_id" BIGINT,
  PRIMARY KEY ("prorated_order_item_adjust_id")
);

CREATE TABLE "blc_qual_crit_offer_xref" (
  "offer_qual_crit_id" BIGINT NOT NULL,
  "offer_id" BIGINT NOT NULL,
  "offer_item_criteria_id" BIGINT,
  PRIMARY KEY ("offer_qual_crit_id")
);

CREATE TABLE "blc_qual_crit_page_xref" (
  "page_id" BIGINT NOT NULL,
  "page_item_criteria_id" BIGINT NOT NULL,
  PRIMARY KEY ("page_id", "page_item_criteria_id"),
  UNIQUE ("page_item_criteria_id")
);

CREATE TABLE "blc_qual_crit_sc_xref" (
  "sc_id" BIGINT NOT NULL,
  "sc_item_criteria_id" BIGINT NOT NULL,
  PRIMARY KEY ("sc_id", "sc_item_criteria_id"),
  UNIQUE ("sc_item_criteria_id")
);

CREATE TABLE "blc_rating_detail" (
  "rating_detail_id" BIGINT NOT NULL,
  "rating" DOUBLE PRECISION NOT NULL,
  "rating_submitted_date" TIMESTAMP NOT NULL,
  "customer_id" BIGINT NOT NULL,
  "rating_summary_id" BIGINT NOT NULL,
  PRIMARY KEY ("rating_detail_id")
);

CREATE TABLE "blc_rating_summary" (
  "rating_summary_id" BIGINT NOT NULL,
  "average_rating" DOUBLE PRECISION NOT NULL,
  "item_id" VARCHAR(255) NOT NULL,
  "rating_type" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("rating_summary_id")
);

CREATE TABLE "blc_review_detail" (
  "review_detail_id" BIGINT NOT NULL,
  "helpful_count" INTEGER NOT NULL,
  "not_helpful_count" INTEGER NOT NULL,
  "review_submitted_date" TIMESTAMP NOT NULL,
  "review_status" VARCHAR(255) NOT NULL,
  "review_text" VARCHAR(255) NOT NULL,
  "customer_id" BIGINT NOT NULL,
  "rating_detail_id" BIGINT,
  "rating_summary_id" BIGINT NOT NULL,
  PRIMARY KEY ("review_detail_id")
);

CREATE TABLE "blc_review_feedback" (
  "review_feedback_id" BIGINT NOT NULL,
  "is_helpful" INTEGER NOT NULL,
  "customer_id" BIGINT NOT NULL,
  "review_detail_id" BIGINT NOT NULL,
  PRIMARY KEY ("review_feedback_id")
);

CREATE TABLE "blc_role" (
  "role_id" BIGINT NOT NULL,
  "role_name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("role_id")
);

CREATE TABLE "blc_sandbox" (
  "sandbox_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "author" BIGINT,
  "color" VARCHAR(255),
  "description" VARCHAR(255),
  "go_live_date" TIMESTAMP,
  "sandbox_name" VARCHAR(255),
  "sandbox_type" VARCHAR(255),
  "parent_sandbox_id" BIGINT,
  PRIMARY KEY ("sandbox_id")
);

CREATE TABLE "blc_sandbox_mgmt" (
  "sandbox_mgmt_id" BIGINT NOT NULL,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "sandbox_id" BIGINT NOT NULL,
  PRIMARY KEY ("sandbox_mgmt_id"),
  UNIQUE ("sandbox_id")
);

CREATE TABLE "blc_sc" (
  "sc_id" BIGINT NOT NULL,
  "content_name" VARCHAR(255) NOT NULL,
  "offline_flag" INTEGER,
  "priority" INTEGER NOT NULL,
  "locale_code" VARCHAR(255) NOT NULL,
  "sc_type_id" BIGINT,
  PRIMARY KEY ("sc_id")
);

CREATE TABLE "blc_sc_fld" (
  "sc_fld_id" BIGINT NOT NULL,
  "fld_key" VARCHAR(255),
  "lob_value" TEXT,
  "value" VARCHAR(255),
  PRIMARY KEY ("sc_fld_id")
);

CREATE TABLE "blc_sc_fld_map" (
  "blc_sc_sc_field_id" BIGINT NOT NULL,
  "map_key" VARCHAR(255) NOT NULL,
  "sc_id" BIGINT NOT NULL,
  "sc_fld_id" BIGINT,
  PRIMARY KEY ("blc_sc_sc_field_id")
);

CREATE TABLE "blc_sc_fld_tmplt" (
  "sc_fld_tmplt_id" BIGINT NOT NULL,
  "name" VARCHAR(255),
  PRIMARY KEY ("sc_fld_tmplt_id")
);

CREATE TABLE "blc_sc_fldgrp_xref" (
  "blc_sc_fldgrp_xref_id" BIGINT NOT NULL,
  "group_order" INTEGER,
  "fld_group_id" BIGINT,
  "sc_fld_tmplt_id" BIGINT,
  PRIMARY KEY ("blc_sc_fldgrp_xref_id")
);

CREATE TABLE "blc_sc_item_criteria" (
  "sc_item_criteria_id" BIGINT NOT NULL,
  "order_item_match_rule" TEXT,
  "quantity" INTEGER NOT NULL,
  "sc_id" BIGINT,
  PRIMARY KEY ("sc_item_criteria_id")
);

CREATE TABLE "blc_sc_rule" (
  "sc_rule_id" BIGINT NOT NULL,
  "match_rule" TEXT,
  PRIMARY KEY ("sc_rule_id")
);

CREATE TABLE "blc_sc_rule_map" (
  "blc_sc_sc_id" BIGINT NOT NULL,
  "sc_rule_id" BIGINT NOT NULL,
  "map_key" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("blc_sc_sc_id", "map_key")
);

CREATE TABLE "blc_sc_type" (
  "sc_type_id" BIGINT NOT NULL,
  "description" VARCHAR(255),
  "name" VARCHAR(255),
  "sc_fld_tmplt_id" BIGINT,
  PRIMARY KEY ("sc_type_id")
);

CREATE TABLE "blc_search_facet" (
  "search_facet_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "multiselect" INTEGER,
  "label" VARCHAR(255),
  "name" VARCHAR(255),
  "requires_all_dependent" INTEGER,
  "search_display_priority" INTEGER,
  "show_on_search" INTEGER,
  "use_facet_ranges" INTEGER,
  "index_field_type_id" BIGINT,
  PRIMARY KEY ("search_facet_id")
);

CREATE TABLE "blc_search_facet_range" (
  "search_facet_range_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "max_value" DECIMAL(19, 5),
  "min_value" DECIMAL(19, 5) NOT NULL,
  "search_facet_id" BIGINT,
  PRIMARY KEY ("search_facet_range_id")
);

CREATE TABLE "blc_search_facet_xref" (
  "id" BIGINT NOT NULL,
  "required_facet_id" BIGINT NOT NULL,
  "search_facet_id" BIGINT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "blc_search_intercept" (
  "search_redirect_id" BIGINT NOT NULL,
  "active_end_date" TIMESTAMP,
  "active_start_date" TIMESTAMP,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "priority" INTEGER,
  "search_term" VARCHAR(255) NOT NULL,
  "url" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("search_redirect_id")
);

CREATE TABLE "blc_search_synonym" (
  "search_synonym_id" BIGINT NOT NULL,
  "synonyms" VARCHAR(255),
  "term" VARCHAR(255),
  PRIMARY KEY ("search_synonym_id")
);

CREATE TABLE "blc_shipping_rate" (
  "id" BIGINT NOT NULL,
  "band_result_pct" INTEGER NOT NULL,
  "band_result_qty" DECIMAL(19, 2) NOT NULL,
  "band_unit_qty" DECIMAL(19, 2) NOT NULL,
  "fee_band" INTEGER NOT NULL,
  "fee_sub_type" VARCHAR(255),
  "fee_type" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "blc_site" (
  "site_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "deactivated" INTEGER,
  "name" VARCHAR(255),
  "site_identifier_type" VARCHAR(255),
  "site_identifier_value" VARCHAR(255),
  PRIMARY KEY ("site_id")
);

CREATE TABLE "blc_site_catalog" (
  "site_catalog_xref_id" BIGINT NOT NULL,
  "catalog_id" BIGINT NOT NULL,
  "site_id" BIGINT NOT NULL,
  PRIMARY KEY ("site_catalog_xref_id")
);

CREATE TABLE "blc_site_map_cfg" (
  "indexed_site_map_file_name" VARCHAR(255),
  "indexed_site_map_file_pattern" VARCHAR(255),
  "max_url_entries_per_file" INTEGER,
  "site_map_file_name" VARCHAR(255),
  "module_config_id" BIGINT NOT NULL,
  PRIMARY KEY ("module_config_id")
);

CREATE TABLE "blc_site_map_gen_cfg" (
  "gen_config_id" BIGINT NOT NULL,
  "change_freq" VARCHAR(255) NOT NULL,
  "disabled" INTEGER NOT NULL,
  "generator_type" VARCHAR(255) NOT NULL,
  "priority" VARCHAR(255),
  "module_config_id" BIGINT NOT NULL,
  PRIMARY KEY ("gen_config_id")
);

CREATE TABLE "blc_site_map_url_entry" (
  "url_entry_id" BIGINT NOT NULL,
  "change_freq" VARCHAR(255) NOT NULL,
  "last_modified" TIMESTAMP NOT NULL,
  "location" VARCHAR(255) NOT NULL,
  "priority" VARCHAR(255) NOT NULL,
  "gen_config_id" BIGINT NOT NULL,
  PRIMARY KEY ("url_entry_id")
);

CREATE TABLE "blc_sku" (
  "sku_id" BIGINT NOT NULL,
  "active_end_date" TIMESTAMP,
  "active_start_date" TIMESTAMP,
  "available_flag" CHAR(1),
  "cost" DECIMAL(19, 5),
  "description" VARCHAR(255),
  "container_shape" VARCHAR(255),
  "depth" DECIMAL(19, 2),
  "dimension_unit_of_measure" VARCHAR(255),
  "girth" DECIMAL(19, 2),
  "height" DECIMAL(19, 2),
  "container_size" VARCHAR(255),
  "width" DECIMAL(19, 2),
  "discountable_flag" CHAR(1),
  "display_template" VARCHAR(255),
  "external_id" VARCHAR(255),
  "fulfillment_type" VARCHAR(255),
  "inventory_type" VARCHAR(255),
  "is_machine_sortable" INTEGER,
  "long_description" TEXT,
  "name" VARCHAR(255),
  "quantity_available" INTEGER,
  "retail_price" DECIMAL(19, 5),
  "sale_price" DECIMAL(19, 5),
  "tax_code" VARCHAR(255),
  "taxable_flag" CHAR(1),
  "upc" VARCHAR(255),
  "url_key" VARCHAR(255),
  "weight" DECIMAL(19, 2),
  "weight_unit_of_measure" VARCHAR(255),
  "currency_code" VARCHAR(255),
  "default_product_id" BIGINT,
  "addl_product_id" BIGINT,
  PRIMARY KEY ("sku_id")
);

CREATE TABLE "blc_sku_attribute" (
  "sku_attr_id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255) NOT NULL,
  "sku_id" BIGINT NOT NULL,
  PRIMARY KEY ("sku_attr_id")
);

CREATE TABLE "blc_sku_availability" (
  "sku_availability_id" BIGINT NOT NULL,
  "availability_date" TIMESTAMP,
  "availability_status" VARCHAR(255),
  "location_id" BIGINT,
  "qty_on_hand" INTEGER,
  "reserve_qty" INTEGER,
  "sku_id" BIGINT,
  PRIMARY KEY ("sku_availability_id")
);

CREATE TABLE "blc_sku_bundle_item" (
  "sku_bundle_item_id" BIGINT NOT NULL,
  "item_sale_price" DECIMAL(19, 5),
  "quantity" INTEGER NOT NULL,
  "sequence" DECIMAL(10, 6),
  "product_bundle_id" BIGINT NOT NULL,
  "sku_id" BIGINT NOT NULL,
  PRIMARY KEY ("sku_bundle_item_id")
);

CREATE TABLE "blc_sku_fee" (
  "sku_fee_id" BIGINT NOT NULL,
  "amount" DECIMAL(19, 5) NOT NULL,
  "description" VARCHAR(255),
  "expression" TEXT,
  "fee_type" VARCHAR(255),
  "name" VARCHAR(255),
  "taxable" INTEGER,
  "currency_code" VARCHAR(255),
  PRIMARY KEY ("sku_fee_id")
);

CREATE TABLE "blc_sku_fee_xref" (
  "sku_fee_id" BIGINT NOT NULL,
  "sku_id" BIGINT NOT NULL
);

CREATE TABLE "blc_sku_fulfillment_excluded" (
  "sku_id" BIGINT NOT NULL,
  "fulfillment_option_id" BIGINT NOT NULL
);

CREATE TABLE "blc_sku_fulfillment_flat_rates" (
  "sku_id" BIGINT NOT NULL,
  "rate" DECIMAL(19, 5),
  "fulfillment_option_id" BIGINT NOT NULL,
  PRIMARY KEY ("sku_id", "fulfillment_option_id")
);

CREATE TABLE "blc_sku_media_map" (
  "sku_media_id" BIGINT NOT NULL,
  "map_key" VARCHAR(255) NOT NULL,
  "media_id" BIGINT,
  "blc_sku_sku_id" BIGINT NOT NULL,
  PRIMARY KEY ("sku_media_id")
);

CREATE TABLE "blc_sku_option_value_xref" (
  "sku_option_value_xref_id" BIGINT NOT NULL,
  "product_option_value_id" BIGINT NOT NULL,
  "sku_id" BIGINT NOT NULL,
  PRIMARY KEY ("sku_option_value_xref_id")
);

CREATE TABLE "blc_state" (
  "abbreviation" VARCHAR(255) NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "country" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("abbreviation")
);

CREATE TABLE "blc_static_asset" (
  "static_asset_id" BIGINT NOT NULL,
  "alt_text" VARCHAR(255),
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "file_extension" VARCHAR(255),
  "file_size" BIGINT,
  "full_url" VARCHAR(255) NOT NULL,
  "mime_type" VARCHAR(255),
  "name" VARCHAR(255) NOT NULL,
  "storage_type" VARCHAR(255),
  "title" VARCHAR(255),
  PRIMARY KEY ("static_asset_id")
);

CREATE TABLE "blc_static_asset_desc" (
  "static_asset_desc_id" BIGINT NOT NULL,
  "created_by" BIGINT,
  "date_created" TIMESTAMP,
  "date_updated" TIMESTAMP,
  "updated_by" BIGINT,
  "description" VARCHAR(255),
  "long_description" VARCHAR(255),
  PRIMARY KEY ("static_asset_desc_id")
);

CREATE TABLE "blc_static_asset_strg" (
  "static_asset_strg_id" BIGINT NOT NULL,
  "file_data" BYTEA,
  "static_asset_id" BIGINT NOT NULL,
  PRIMARY KEY ("static_asset_strg_id")
);

CREATE TABLE "blc_store" (
  "store_id" BIGINT NOT NULL,
  "archived" CHAR(1),
  "latitude" DOUBLE PRECISION,
  "longitude" DOUBLE PRECISION,
  "store_name" VARCHAR(255) NOT NULL,
  "store_open" INTEGER,
  "store_hours" VARCHAR(255),
  "store_number" VARCHAR(255),
  "address_id" BIGINT,
  PRIMARY KEY ("store_id")
);

CREATE TABLE "blc_system_property" (
  "blc_system_property_id" BIGINT NOT NULL,
  "friendly_group" VARCHAR(255),
  "friendly_name" VARCHAR(255),
  "friendly_tab" VARCHAR(255),
  "property_name" VARCHAR(255) NOT NULL,
  "override_generated_prop_name" INTEGER,
  "property_type" VARCHAR(255),
  "property_value" VARCHAR(255),
  PRIMARY KEY ("blc_system_property_id")
);

CREATE TABLE "blc_tar_crit_offer_xref" (
  "offer_tar_crit_id" BIGINT NOT NULL,
  "offer_id" BIGINT NOT NULL,
  "offer_item_criteria_id" BIGINT,
  PRIMARY KEY ("offer_tar_crit_id")
);

CREATE TABLE "blc_tax_detail" (
  "tax_detail_id" BIGINT NOT NULL,
  "amount" DECIMAL(19, 5),
  "tax_country" VARCHAR(255),
  "jurisdiction_name" VARCHAR(255),
  "rate" DECIMAL(19, 5),
  "tax_region" VARCHAR(255),
  "tax_name" VARCHAR(255),
  "type" VARCHAR(255),
  "currency_code" VARCHAR(255),
  "module_config_id" BIGINT,
  PRIMARY KEY ("tax_detail_id")
);

CREATE TABLE "blc_trans_additnl_fields" (
  "payment_transaction_id" BIGINT NOT NULL,
  "field_value" TEXT,
  "field_name" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("payment_transaction_id", "field_name")
);

CREATE TABLE "blc_translation" (
  "translation_id" BIGINT NOT NULL,
  "entity_id" VARCHAR(255),
  "entity_type" VARCHAR(255),
  "field_name" VARCHAR(255),
  "locale_code" VARCHAR(255),
  "translated_value" TEXT,
  PRIMARY KEY ("translation_id")
);

CREATE TABLE "blc_url_handler" (
  "url_handler_id" BIGINT NOT NULL,
  "incoming_url" VARCHAR(255) NOT NULL,
  "is_regex" INTEGER,
  "new_url" VARCHAR(255) NOT NULL,
  "url_redirect_type" VARCHAR(255),
  PRIMARY KEY ("url_handler_id")
);

CREATE TABLE "blc_userconnection" (
  "providerid" VARCHAR(255) NOT NULL,
  "provideruserid" VARCHAR(255) NOT NULL,
  "userid" VARCHAR(255) NOT NULL,
  "accesstoken" VARCHAR(255) NOT NULL,
  "displayname" VARCHAR(255),
  "expiretime" BIGINT,
  "imageurl" VARCHAR(255),
  "profileurl" VARCHAR(255),
  "rank" INTEGER NOT NULL,
  "refreshtoken" VARCHAR(255),
  "secret" VARCHAR(255),
  PRIMARY KEY ("providerid", "provideruserid", "userid")
);

CREATE TABLE "blc_zip_code" (
  "zip_code_id" VARCHAR(255) NOT NULL,
  "zip_city" VARCHAR(255),
  "zip_latitude" DOUBLE PRECISION,
  "zip_longitude" DOUBLE PRECISION,
  "zip_state" VARCHAR(255),
  "zipcode" INTEGER,
  PRIMARY KEY ("zip_code_id")
);

CREATE TABLE "sequence_generator" (
  "id_name" VARCHAR(255) NOT NULL,
  "id_val" BIGINT,
  PRIMARY KEY ("id_name")
);

ALTER TABLE "blc_additional_offer_info" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_additional_offer_info" ADD FOREIGN KEY ("offer_info_id") REFERENCES "blc_offer_info" ("offer_info_id");

ALTER TABLE "blc_additional_offer_info" ADD FOREIGN KEY ("blc_order_order_id") REFERENCES "blc_order" ("order_id");

ALTER TABLE "blc_address" ADD FOREIGN KEY ("country") REFERENCES "blc_country" ("abbreviation");

ALTER TABLE "blc_address" ADD FOREIGN KEY ("state_prov_region") REFERENCES "blc_state" ("abbreviation");

ALTER TABLE "blc_address" ADD FOREIGN KEY ("phone_primary_id") REFERENCES "blc_phone" ("phone_id");

ALTER TABLE "blc_address" ADD FOREIGN KEY ("phone_secondary_id") REFERENCES "blc_phone" ("phone_id");

ALTER TABLE "blc_address" ADD FOREIGN KEY ("iso_country_alpha2") REFERENCES "blc_iso_country" ("alpha_2");

ALTER TABLE "blc_address" ADD FOREIGN KEY ("phone_fax_id") REFERENCES "blc_phone" ("phone_id");

ALTER TABLE "blc_admin_permission_entity" ADD FOREIGN KEY ("admin_permission_id") REFERENCES "blc_admin_permission" ("admin_permission_id");

ALTER TABLE "blc_admin_permission_xref" ADD FOREIGN KEY ("admin_permission_id") REFERENCES "blc_admin_permission" ("admin_permission_id");

ALTER TABLE "blc_admin_permission_xref" ADD FOREIGN KEY ("child_permission_id") REFERENCES "blc_admin_permission" ("admin_permission_id");

ALTER TABLE "blc_admin_role_permission_xref" ADD FOREIGN KEY ("admin_role_id") REFERENCES "blc_admin_role" ("admin_role_id");

ALTER TABLE "blc_admin_role_permission_xref" ADD FOREIGN KEY ("admin_permission_id") REFERENCES "blc_admin_permission" ("admin_permission_id");

ALTER TABLE "blc_admin_sec_perm_xref" ADD FOREIGN KEY ("admin_section_id") REFERENCES "blc_admin_section" ("admin_section_id");

ALTER TABLE "blc_admin_sec_perm_xref" ADD FOREIGN KEY ("admin_permission_id") REFERENCES "blc_admin_permission" ("admin_permission_id");

ALTER TABLE "blc_admin_section" ADD FOREIGN KEY ("admin_module_id") REFERENCES "blc_admin_module" ("admin_module_id");

ALTER TABLE "blc_admin_user_addtl_fields" ADD FOREIGN KEY ("admin_user_id") REFERENCES "blc_admin_user" ("admin_user_id");

ALTER TABLE "blc_admin_user_permission_xref" ADD FOREIGN KEY ("admin_permission_id") REFERENCES "blc_admin_permission" ("admin_permission_id");

ALTER TABLE "blc_admin_user_permission_xref" ADD FOREIGN KEY ("admin_user_id") REFERENCES "blc_admin_user" ("admin_user_id");

ALTER TABLE "blc_admin_user_role_xref" ADD FOREIGN KEY ("admin_role_id") REFERENCES "blc_admin_role" ("admin_role_id");

ALTER TABLE "blc_admin_user_role_xref" ADD FOREIGN KEY ("admin_user_id") REFERENCES "blc_admin_user" ("admin_user_id");

ALTER TABLE "blc_admin_user_sandbox" ADD FOREIGN KEY ("admin_user_id") REFERENCES "blc_admin_user" ("admin_user_id");

ALTER TABLE "blc_admin_user_sandbox" ADD FOREIGN KEY ("sandbox_id") REFERENCES "blc_sandbox" ("sandbox_id");

ALTER TABLE "blc_asset_desc_map" ADD FOREIGN KEY ("static_asset_desc_id") REFERENCES "blc_static_asset_desc" ("static_asset_desc_id");

ALTER TABLE "blc_asset_desc_map" ADD FOREIGN KEY ("static_asset_id") REFERENCES "blc_static_asset" ("static_asset_id");

ALTER TABLE "blc_bund_item_fee_price" ADD FOREIGN KEY ("bund_order_item_id") REFERENCES "blc_bundle_order_item" ("order_item_id");

ALTER TABLE "blc_bundle_order_item" ADD FOREIGN KEY ("sku_id") REFERENCES "blc_sku" ("sku_id");

ALTER TABLE "blc_bundle_order_item" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_bundle_order_item" ADD FOREIGN KEY ("product_bundle_id") REFERENCES "blc_product_bundle" ("product_id");

ALTER TABLE "blc_candidate_fg_offer" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_candidate_fg_offer" ADD FOREIGN KEY ("fulfillment_group_id") REFERENCES "blc_fulfillment_group" ("fulfillment_group_id");

ALTER TABLE "blc_candidate_item_offer" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_candidate_item_offer" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_candidate_order_offer" ADD FOREIGN KEY ("order_id") REFERENCES "blc_order" ("order_id");

ALTER TABLE "blc_candidate_order_offer" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_cat_search_facet_excl_xref" ADD FOREIGN KEY ("category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_cat_search_facet_excl_xref" ADD FOREIGN KEY ("search_facet_id") REFERENCES "blc_search_facet" ("search_facet_id");

ALTER TABLE "blc_cat_search_facet_xref" ADD FOREIGN KEY ("category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_cat_search_facet_xref" ADD FOREIGN KEY ("search_facet_id") REFERENCES "blc_search_facet" ("search_facet_id");

ALTER TABLE "blc_cat_site_map_gen_cfg" ADD FOREIGN KEY ("root_category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_cat_site_map_gen_cfg" ADD FOREIGN KEY ("gen_config_id") REFERENCES "blc_site_map_gen_cfg" ("gen_config_id");

ALTER TABLE "blc_category" ADD FOREIGN KEY ("default_parent_category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_category_attribute" ADD FOREIGN KEY ("category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_category_media_map" ADD FOREIGN KEY ("media_id") REFERENCES "blc_media" ("media_id");

ALTER TABLE "blc_category_media_map" ADD FOREIGN KEY ("blc_category_category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_category_product_xref" ADD FOREIGN KEY ("category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_category_product_xref" ADD FOREIGN KEY ("product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_category_xref" ADD FOREIGN KEY ("category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_category_xref" ADD FOREIGN KEY ("sub_category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_cms_menu_item" ADD FOREIGN KEY ("parent_menu_id") REFERENCES "blc_cms_menu" ("menu_id");

ALTER TABLE "blc_cms_menu_item" ADD FOREIGN KEY ("linked_page_id") REFERENCES "blc_page" ("page_id");

ALTER TABLE "blc_cms_menu_item" ADD FOREIGN KEY ("linked_menu_id") REFERENCES "blc_cms_menu" ("menu_id");

ALTER TABLE "blc_country_sub" ADD FOREIGN KEY ("country") REFERENCES "blc_country" ("abbreviation");

ALTER TABLE "blc_country_sub" ADD FOREIGN KEY ("country_sub_cat") REFERENCES "blc_country_sub_cat" ("country_sub_cat_id");

ALTER TABLE "blc_cust_site_map_gen_cfg" ADD FOREIGN KEY ("gen_config_id") REFERENCES "blc_site_map_gen_cfg" ("gen_config_id");

ALTER TABLE "blc_customer" ADD FOREIGN KEY ("locale_code") REFERENCES "blc_locale" ("locale_code");

ALTER TABLE "blc_customer" ADD FOREIGN KEY ("challenge_question_id") REFERENCES "blc_challenge_question" ("question_id");

ALTER TABLE "blc_customer_address" ADD FOREIGN KEY ("address_id") REFERENCES "blc_address" ("address_id");

ALTER TABLE "blc_customer_address" ADD FOREIGN KEY ("customer_id") REFERENCES "blc_customer" ("customer_id");

ALTER TABLE "blc_customer_attribute" ADD FOREIGN KEY ("customer_id") REFERENCES "blc_customer" ("customer_id");

ALTER TABLE "blc_customer_offer_xref" ADD FOREIGN KEY ("customer_id") REFERENCES "blc_customer" ("customer_id");

ALTER TABLE "blc_customer_offer_xref" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_customer_payment" ADD FOREIGN KEY ("customer_id") REFERENCES "blc_customer" ("customer_id");

ALTER TABLE "blc_customer_payment" ADD FOREIGN KEY ("address_id") REFERENCES "blc_address" ("address_id");

ALTER TABLE "blc_customer_payment_fields" ADD FOREIGN KEY ("customer_payment_id") REFERENCES "blc_customer_payment" ("customer_payment_id");

ALTER TABLE "blc_customer_phone" ADD FOREIGN KEY ("phone_id") REFERENCES "blc_phone" ("phone_id");

ALTER TABLE "blc_customer_phone" ADD FOREIGN KEY ("customer_id") REFERENCES "blc_customer" ("customer_id");

ALTER TABLE "blc_customer_role" ADD FOREIGN KEY ("role_id") REFERENCES "blc_role" ("role_id");

ALTER TABLE "blc_customer_role" ADD FOREIGN KEY ("customer_id") REFERENCES "blc_customer" ("customer_id");

ALTER TABLE "blc_data_drvn_enum_val" ADD FOREIGN KEY ("enum_type") REFERENCES "blc_data_drvn_enum" ("enum_id");

ALTER TABLE "blc_disc_item_fee_price" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_discrete_order_item" ("order_item_id");

ALTER TABLE "blc_discrete_order_item" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_discrete_order_item" ADD FOREIGN KEY ("sku_bundle_item_id") REFERENCES "blc_sku_bundle_item" ("sku_bundle_item_id");

ALTER TABLE "blc_discrete_order_item" ADD FOREIGN KEY ("product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_discrete_order_item" ADD FOREIGN KEY ("sku_id") REFERENCES "blc_sku" ("sku_id");

ALTER TABLE "blc_discrete_order_item" ADD FOREIGN KEY ("bundle_order_item_id") REFERENCES "blc_bundle_order_item" ("order_item_id");

ALTER TABLE "blc_dyn_discrete_order_item" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_discrete_order_item" ("order_item_id");

ALTER TABLE "blc_email_tracking_clicks" ADD FOREIGN KEY ("email_tracking_id") REFERENCES "blc_email_tracking" ("email_tracking_id");

ALTER TABLE "blc_email_tracking_opens" ADD FOREIGN KEY ("email_tracking_id") REFERENCES "blc_email_tracking" ("email_tracking_id");

ALTER TABLE "blc_fg_adjustment" ADD FOREIGN KEY ("fulfillment_group_id") REFERENCES "blc_fulfillment_group" ("fulfillment_group_id");

ALTER TABLE "blc_fg_adjustment" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_fg_fee_tax_xref" ADD FOREIGN KEY ("tax_detail_id") REFERENCES "blc_tax_detail" ("tax_detail_id");

ALTER TABLE "blc_fg_fee_tax_xref" ADD FOREIGN KEY ("fulfillment_group_fee_id") REFERENCES "blc_fulfillment_group_fee" ("fulfillment_group_fee_id");

ALTER TABLE "blc_fg_fg_tax_xref" ADD FOREIGN KEY ("tax_detail_id") REFERENCES "blc_tax_detail" ("tax_detail_id");

ALTER TABLE "blc_fg_fg_tax_xref" ADD FOREIGN KEY ("fulfillment_group_id") REFERENCES "blc_fulfillment_group" ("fulfillment_group_id");

ALTER TABLE "blc_fg_item_tax_xref" ADD FOREIGN KEY ("tax_detail_id") REFERENCES "blc_tax_detail" ("tax_detail_id");

ALTER TABLE "blc_fg_item_tax_xref" ADD FOREIGN KEY ("fulfillment_group_item_id") REFERENCES "blc_fulfillment_group_item" ("fulfillment_group_item_id");

ALTER TABLE "blc_fld_def" ADD FOREIGN KEY ("enum_id") REFERENCES "blc_data_drvn_enum" ("enum_id");

ALTER TABLE "blc_fld_def" ADD FOREIGN KEY ("fld_group_id") REFERENCES "blc_fld_group" ("fld_group_id");

ALTER TABLE "blc_fld_enum_item" ADD FOREIGN KEY ("fld_enum_id") REFERENCES "blc_fld_enum" ("fld_enum_id");

ALTER TABLE "blc_fulfillment_group" ADD FOREIGN KEY ("fulfillment_option_id") REFERENCES "blc_fulfillment_option" ("fulfillment_option_id");

ALTER TABLE "blc_fulfillment_group" ADD FOREIGN KEY ("phone_id") REFERENCES "blc_phone" ("phone_id");

ALTER TABLE "blc_fulfillment_group" ADD FOREIGN KEY ("personal_message_id") REFERENCES "blc_personal_message" ("personal_message_id");

ALTER TABLE "blc_fulfillment_group" ADD FOREIGN KEY ("address_id") REFERENCES "blc_address" ("address_id");

ALTER TABLE "blc_fulfillment_group" ADD FOREIGN KEY ("order_id") REFERENCES "blc_order" ("order_id");

ALTER TABLE "blc_fulfillment_group_fee" ADD FOREIGN KEY ("fulfillment_group_id") REFERENCES "blc_fulfillment_group" ("fulfillment_group_id");

ALTER TABLE "blc_fulfillment_group_item" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_fulfillment_group_item" ADD FOREIGN KEY ("fulfillment_group_id") REFERENCES "blc_fulfillment_group" ("fulfillment_group_id");

ALTER TABLE "blc_fulfillment_opt_banded_prc" ADD FOREIGN KEY ("fulfillment_option_id") REFERENCES "blc_fulfillment_option" ("fulfillment_option_id");

ALTER TABLE "blc_fulfillment_opt_banded_wgt" ADD FOREIGN KEY ("fulfillment_option_id") REFERENCES "blc_fulfillment_option" ("fulfillment_option_id");

ALTER TABLE "blc_fulfillment_option_fixed" ADD FOREIGN KEY ("currency_code") REFERENCES "blc_currency" ("currency_code");

ALTER TABLE "blc_fulfillment_option_fixed" ADD FOREIGN KEY ("fulfillment_option_id") REFERENCES "blc_fulfillment_option" ("fulfillment_option_id");

ALTER TABLE "blc_fulfillment_price_band" ADD FOREIGN KEY ("fulfillment_option_id") REFERENCES "blc_fulfillment_opt_banded_prc" ("fulfillment_option_id");

ALTER TABLE "blc_fulfillment_weight_band" ADD FOREIGN KEY ("fulfillment_option_id") REFERENCES "blc_fulfillment_opt_banded_wgt" ("fulfillment_option_id");

ALTER TABLE "blc_giftwrap_order_item" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_discrete_order_item" ("order_item_id");

ALTER TABLE "blc_img_static_asset" ADD FOREIGN KEY ("static_asset_id") REFERENCES "blc_static_asset" ("static_asset_id");

ALTER TABLE "blc_index_field" ADD FOREIGN KEY ("field_id") REFERENCES "blc_field" ("field_id");

ALTER TABLE "blc_index_field_type" ADD FOREIGN KEY ("index_field_id") REFERENCES "blc_index_field" ("index_field_id");

ALTER TABLE "blc_item_offer_qualifier" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_item_offer_qualifier" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_locale" ADD FOREIGN KEY ("currency_code") REFERENCES "blc_currency" ("currency_code");

ALTER TABLE "blc_offer_code" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_offer_info_fields" ADD FOREIGN KEY ("offer_info_fields_id") REFERENCES "blc_offer_info" ("offer_info_id");

ALTER TABLE "blc_offer_rule_map" ADD FOREIGN KEY ("offer_rule_id") REFERENCES "blc_offer_rule" ("offer_rule_id");

ALTER TABLE "blc_offer_rule_map" ADD FOREIGN KEY ("blc_offer_offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_order" ADD FOREIGN KEY ("locale_code") REFERENCES "blc_locale" ("locale_code");

ALTER TABLE "blc_order" ADD FOREIGN KEY ("customer_id") REFERENCES "blc_customer" ("customer_id");

ALTER TABLE "blc_order" ADD FOREIGN KEY ("currency_code") REFERENCES "blc_currency" ("currency_code");

ALTER TABLE "blc_order_adjustment" ADD FOREIGN KEY ("order_id") REFERENCES "blc_order" ("order_id");

ALTER TABLE "blc_order_adjustment" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_order_attribute" ADD FOREIGN KEY ("order_id") REFERENCES "blc_order" ("order_id");

ALTER TABLE "blc_order_item" ADD FOREIGN KEY ("gift_wrap_item_id") REFERENCES "blc_giftwrap_order_item" ("order_item_id");

ALTER TABLE "blc_order_item" ADD FOREIGN KEY ("parent_order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_order_item" ADD FOREIGN KEY ("category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_order_item" ADD FOREIGN KEY ("order_id") REFERENCES "blc_order" ("order_id");

ALTER TABLE "blc_order_item" ADD FOREIGN KEY ("personal_message_id") REFERENCES "blc_personal_message" ("personal_message_id");

ALTER TABLE "blc_order_item_add_attr" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_discrete_order_item" ("order_item_id");

ALTER TABLE "blc_order_item_adjustment" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_order_item_adjustment" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_order_item_attribute" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_order_item_cart_message" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_order_item_dtl_adj" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_order_item_dtl_adj" ADD FOREIGN KEY ("order_item_price_dtl_id") REFERENCES "blc_order_item_price_dtl" ("order_item_price_dtl_id");

ALTER TABLE "blc_order_item_price_dtl" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_order_multiship_option" ADD FOREIGN KEY ("fulfillment_option_id") REFERENCES "blc_fulfillment_option" ("fulfillment_option_id");

ALTER TABLE "blc_order_multiship_option" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_order_multiship_option" ADD FOREIGN KEY ("order_id") REFERENCES "blc_order" ("order_id");

ALTER TABLE "blc_order_multiship_option" ADD FOREIGN KEY ("address_id") REFERENCES "blc_address" ("address_id");

ALTER TABLE "blc_order_offer_code_xref" ADD FOREIGN KEY ("offer_code_id") REFERENCES "blc_offer_code" ("offer_code_id");

ALTER TABLE "blc_order_offer_code_xref" ADD FOREIGN KEY ("order_id") REFERENCES "blc_order" ("order_id");

ALTER TABLE "blc_order_payment" ADD FOREIGN KEY ("address_id") REFERENCES "blc_address" ("address_id");

ALTER TABLE "blc_order_payment" ADD FOREIGN KEY ("order_id") REFERENCES "blc_order" ("order_id");

ALTER TABLE "blc_order_payment_transaction" ADD FOREIGN KEY ("parent_transaction") REFERENCES "blc_order_payment_transaction" ("payment_transaction_id");

ALTER TABLE "blc_order_payment_transaction" ADD FOREIGN KEY ("order_payment") REFERENCES "blc_order_payment" ("order_payment_id");

ALTER TABLE "blc_page" ADD FOREIGN KEY ("page_tmplt_id") REFERENCES "blc_page_tmplt" ("page_tmplt_id");

ALTER TABLE "blc_page_attributes" ADD FOREIGN KEY ("page_id") REFERENCES "blc_page" ("page_id");

ALTER TABLE "blc_page_fld" ADD FOREIGN KEY ("page_id") REFERENCES "blc_page" ("page_id");

ALTER TABLE "blc_page_rule_map" ADD FOREIGN KEY ("page_rule_id") REFERENCES "blc_page_rule" ("page_rule_id");

ALTER TABLE "blc_page_rule_map" ADD FOREIGN KEY ("blc_page_page_id") REFERENCES "blc_page" ("page_id");

ALTER TABLE "blc_page_tmplt" ADD FOREIGN KEY ("locale_code") REFERENCES "blc_locale" ("locale_code");

ALTER TABLE "blc_payment_log" ADD FOREIGN KEY ("currency_code") REFERENCES "blc_currency" ("currency_code");

ALTER TABLE "blc_payment_log" ADD FOREIGN KEY ("customer_id") REFERENCES "blc_customer" ("customer_id");

ALTER TABLE "blc_pgtmplt_fldgrp_xref" ADD FOREIGN KEY ("fld_group_id") REFERENCES "blc_fld_group" ("fld_group_id");

ALTER TABLE "blc_pgtmplt_fldgrp_xref" ADD FOREIGN KEY ("page_tmplt_id") REFERENCES "blc_page_tmplt" ("page_tmplt_id");

ALTER TABLE "blc_product" ADD FOREIGN KEY ("default_category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_product" ADD FOREIGN KEY ("default_sku_id") REFERENCES "blc_sku" ("sku_id");

ALTER TABLE "blc_product_attribute" ADD FOREIGN KEY ("product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_product_bundle" ADD FOREIGN KEY ("product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_product_cross_sale" ADD FOREIGN KEY ("category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_product_cross_sale" ADD FOREIGN KEY ("product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_product_cross_sale" ADD FOREIGN KEY ("related_sale_product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_product_featured" ADD FOREIGN KEY ("category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_product_featured" ADD FOREIGN KEY ("product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_product_option_value" ADD FOREIGN KEY ("product_option_id") REFERENCES "blc_product_option" ("product_option_id");

ALTER TABLE "blc_product_option_xref" ADD FOREIGN KEY ("product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_product_option_xref" ADD FOREIGN KEY ("product_option_id") REFERENCES "blc_product_option" ("product_option_id");

ALTER TABLE "blc_product_up_sale" ADD FOREIGN KEY ("product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_product_up_sale" ADD FOREIGN KEY ("category_id") REFERENCES "blc_category" ("category_id");

ALTER TABLE "blc_product_up_sale" ADD FOREIGN KEY ("related_sale_product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_promotion_message" ADD FOREIGN KEY ("locale_code") REFERENCES "blc_locale" ("locale_code");

ALTER TABLE "blc_promotion_message" ADD FOREIGN KEY ("media_id") REFERENCES "blc_media" ("media_id");

ALTER TABLE "blc_prorated_order_item_adjust" ADD FOREIGN KEY ("order_item_id") REFERENCES "blc_order_item" ("order_item_id");

ALTER TABLE "blc_prorated_order_item_adjust" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_qual_crit_offer_xref" ADD FOREIGN KEY ("offer_item_criteria_id") REFERENCES "blc_offer_item_criteria" ("offer_item_criteria_id");

ALTER TABLE "blc_qual_crit_offer_xref" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_qual_crit_page_xref" ADD FOREIGN KEY ("page_id") REFERENCES "blc_page" ("page_id");

ALTER TABLE "blc_qual_crit_page_xref" ADD FOREIGN KEY ("page_item_criteria_id") REFERENCES "blc_page_item_criteria" ("page_item_criteria_id");

ALTER TABLE "blc_qual_crit_sc_xref" ADD FOREIGN KEY ("sc_item_criteria_id") REFERENCES "blc_sc_item_criteria" ("sc_item_criteria_id");

ALTER TABLE "blc_qual_crit_sc_xref" ADD FOREIGN KEY ("sc_id") REFERENCES "blc_sc" ("sc_id");

ALTER TABLE "blc_rating_detail" ADD FOREIGN KEY ("customer_id") REFERENCES "blc_customer" ("customer_id");

ALTER TABLE "blc_rating_detail" ADD FOREIGN KEY ("rating_summary_id") REFERENCES "blc_rating_summary" ("rating_summary_id");

ALTER TABLE "blc_review_detail" ADD FOREIGN KEY ("customer_id") REFERENCES "blc_customer" ("customer_id");

ALTER TABLE "blc_review_detail" ADD FOREIGN KEY ("rating_detail_id") REFERENCES "blc_rating_detail" ("rating_detail_id");

ALTER TABLE "blc_review_detail" ADD FOREIGN KEY ("rating_summary_id") REFERENCES "blc_rating_summary" ("rating_summary_id");

ALTER TABLE "blc_review_feedback" ADD FOREIGN KEY ("review_detail_id") REFERENCES "blc_review_detail" ("review_detail_id");

ALTER TABLE "blc_review_feedback" ADD FOREIGN KEY ("customer_id") REFERENCES "blc_customer" ("customer_id");

ALTER TABLE "blc_sandbox" ADD FOREIGN KEY ("parent_sandbox_id") REFERENCES "blc_sandbox" ("sandbox_id");

ALTER TABLE "blc_sandbox_mgmt" ADD FOREIGN KEY ("sandbox_id") REFERENCES "blc_sandbox" ("sandbox_id");

ALTER TABLE "blc_sc" ADD FOREIGN KEY ("locale_code") REFERENCES "blc_locale" ("locale_code");

ALTER TABLE "blc_sc" ADD FOREIGN KEY ("sc_type_id") REFERENCES "blc_sc_type" ("sc_type_id");

ALTER TABLE "blc_sc_fld_map" ADD FOREIGN KEY ("sc_id") REFERENCES "blc_sc" ("sc_id");

ALTER TABLE "blc_sc_fld_map" ADD FOREIGN KEY ("sc_fld_id") REFERENCES "blc_sc_fld" ("sc_fld_id");

ALTER TABLE "blc_sc_fldgrp_xref" ADD FOREIGN KEY ("fld_group_id") REFERENCES "blc_fld_group" ("fld_group_id");

ALTER TABLE "blc_sc_fldgrp_xref" ADD FOREIGN KEY ("sc_fld_tmplt_id") REFERENCES "blc_sc_fld_tmplt" ("sc_fld_tmplt_id");

ALTER TABLE "blc_sc_item_criteria" ADD FOREIGN KEY ("sc_id") REFERENCES "blc_sc" ("sc_id");

ALTER TABLE "blc_sc_rule_map" ADD FOREIGN KEY ("sc_rule_id") REFERENCES "blc_sc_rule" ("sc_rule_id");

ALTER TABLE "blc_sc_rule_map" ADD FOREIGN KEY ("blc_sc_sc_id") REFERENCES "blc_sc" ("sc_id");

ALTER TABLE "blc_sc_type" ADD FOREIGN KEY ("sc_fld_tmplt_id") REFERENCES "blc_sc_fld_tmplt" ("sc_fld_tmplt_id");

ALTER TABLE "blc_search_facet" ADD FOREIGN KEY ("index_field_type_id") REFERENCES "blc_index_field_type" ("index_field_type_id");

ALTER TABLE "blc_search_facet_range" ADD FOREIGN KEY ("search_facet_id") REFERENCES "blc_search_facet" ("search_facet_id");

ALTER TABLE "blc_search_facet_xref" ADD FOREIGN KEY ("required_facet_id") REFERENCES "blc_search_facet" ("search_facet_id");

ALTER TABLE "blc_search_facet_xref" ADD FOREIGN KEY ("search_facet_id") REFERENCES "blc_search_facet" ("search_facet_id");

ALTER TABLE "blc_site_catalog" ADD FOREIGN KEY ("catalog_id") REFERENCES "blc_catalog" ("catalog_id");

ALTER TABLE "blc_site_catalog" ADD FOREIGN KEY ("site_id") REFERENCES "blc_site" ("site_id");

ALTER TABLE "blc_site_map_cfg" ADD FOREIGN KEY ("module_config_id") REFERENCES "blc_module_configuration" ("module_config_id");

ALTER TABLE "blc_site_map_gen_cfg" ADD FOREIGN KEY ("module_config_id") REFERENCES "blc_site_map_cfg" ("module_config_id");

ALTER TABLE "blc_site_map_url_entry" ADD FOREIGN KEY ("gen_config_id") REFERENCES "blc_cust_site_map_gen_cfg" ("gen_config_id");

ALTER TABLE "blc_sku" ADD FOREIGN KEY ("default_product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_sku" ADD FOREIGN KEY ("currency_code") REFERENCES "blc_currency" ("currency_code");

ALTER TABLE "blc_sku" ADD FOREIGN KEY ("addl_product_id") REFERENCES "blc_product" ("product_id");

ALTER TABLE "blc_sku_attribute" ADD FOREIGN KEY ("sku_id") REFERENCES "blc_sku" ("sku_id");

ALTER TABLE "blc_sku_bundle_item" ADD FOREIGN KEY ("product_bundle_id") REFERENCES "blc_product_bundle" ("product_id");

ALTER TABLE "blc_sku_bundle_item" ADD FOREIGN KEY ("sku_id") REFERENCES "blc_sku" ("sku_id");

ALTER TABLE "blc_sku_fee" ADD FOREIGN KEY ("currency_code") REFERENCES "blc_currency" ("currency_code");

ALTER TABLE "blc_sku_fee_xref" ADD FOREIGN KEY ("sku_id") REFERENCES "blc_sku" ("sku_id");

ALTER TABLE "blc_sku_fee_xref" ADD FOREIGN KEY ("sku_fee_id") REFERENCES "blc_sku_fee" ("sku_fee_id");

ALTER TABLE "blc_sku_fulfillment_excluded" ADD FOREIGN KEY ("fulfillment_option_id") REFERENCES "blc_fulfillment_option" ("fulfillment_option_id");

ALTER TABLE "blc_sku_fulfillment_excluded" ADD FOREIGN KEY ("sku_id") REFERENCES "blc_sku" ("sku_id");

ALTER TABLE "blc_sku_fulfillment_flat_rates" ADD FOREIGN KEY ("sku_id") REFERENCES "blc_sku" ("sku_id");

ALTER TABLE "blc_sku_fulfillment_flat_rates" ADD FOREIGN KEY ("fulfillment_option_id") REFERENCES "blc_fulfillment_option" ("fulfillment_option_id");

ALTER TABLE "blc_sku_media_map" ADD FOREIGN KEY ("blc_sku_sku_id") REFERENCES "blc_sku" ("sku_id");

ALTER TABLE "blc_sku_media_map" ADD FOREIGN KEY ("media_id") REFERENCES "blc_media" ("media_id");

ALTER TABLE "blc_sku_option_value_xref" ADD FOREIGN KEY ("product_option_value_id") REFERENCES "blc_product_option_value" ("product_option_value_id");

ALTER TABLE "blc_sku_option_value_xref" ADD FOREIGN KEY ("sku_id") REFERENCES "blc_sku" ("sku_id");

ALTER TABLE "blc_state" ADD FOREIGN KEY ("country") REFERENCES "blc_country" ("abbreviation");

ALTER TABLE "blc_store" ADD FOREIGN KEY ("address_id") REFERENCES "blc_address" ("address_id");

ALTER TABLE "blc_tar_crit_offer_xref" ADD FOREIGN KEY ("offer_id") REFERENCES "blc_offer" ("offer_id");

ALTER TABLE "blc_tar_crit_offer_xref" ADD FOREIGN KEY ("offer_item_criteria_id") REFERENCES "blc_offer_item_criteria" ("offer_item_criteria_id");

ALTER TABLE "blc_tax_detail" ADD FOREIGN KEY ("module_config_id") REFERENCES "blc_module_configuration" ("module_config_id");

ALTER TABLE "blc_tax_detail" ADD FOREIGN KEY ("currency_code") REFERENCES "blc_currency" ("currency_code");

ALTER TABLE "blc_trans_additnl_fields" ADD FOREIGN KEY ("payment_transaction_id") REFERENCES "blc_order_payment_transaction" ("payment_transaction_id");

-- WeTune schema patches
ALTER TABLE "blc_order_item_adjustment" ALTER COLUMN "order_item_id" SET NOT NULL;
ALTER TABLE "blc_page" ALTER COLUMN "full_url" SET NOT NULL;
ALTER TABLE "blc_page" ALTER COLUMN "page_tmplt_id" SET NOT NULL;
ALTER TABLE "blc_order_item_dtl_adj" ALTER COLUMN "order_item_price_dtl_id" SET NOT NULL;
ALTER TABLE "blc_product" ALTER COLUMN "url" SET NOT NULL;
ALTER TABLE "blc_product" ALTER COLUMN "url_key" SET NOT NULL;
ALTER TABLE "blc_product" ALTER COLUMN "default_category_id" SET NOT NULL;
ALTER TABLE "blc_product" ALTER COLUMN "default_sku_id" SET NOT NULL;
ALTER TABLE "blc_locale" ALTER COLUMN "currency_code" SET NOT NULL;
ALTER TABLE "blc_bundle_order_item" ALTER COLUMN "sku_id" SET NOT NULL;
ALTER TABLE "blc_bundle_order_item" ALTER COLUMN "product_bundle_id" SET NOT NULL;
ALTER TABLE "blc_tar_crit_offer_xref" ALTER COLUMN "offer_item_criteria_id" SET NOT NULL;
ALTER TABLE "blc_country_sub" ALTER COLUMN "alt_abbreviation" SET NOT NULL;
ALTER TABLE "blc_country_sub" ALTER COLUMN "country_sub_cat" SET NOT NULL;
ALTER TABLE "blc_search_facet" ALTER COLUMN "index_field_type_id" SET NOT NULL;
ALTER TABLE "blc_product_cross_sale" ALTER COLUMN "category_id" SET NOT NULL;
ALTER TABLE "blc_product_cross_sale" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "blc_address" ALTER COLUMN "country" SET NOT NULL;
ALTER TABLE "blc_address" ALTER COLUMN "state_prov_region" SET NOT NULL;
ALTER TABLE "blc_address" ALTER COLUMN "phone_primary_id" SET NOT NULL;
ALTER TABLE "blc_address" ALTER COLUMN "phone_secondary_id" SET NOT NULL;
ALTER TABLE "blc_address" ALTER COLUMN "iso_country_alpha2" SET NOT NULL;
ALTER TABLE "blc_address" ALTER COLUMN "phone_fax_id" SET NOT NULL;
ALTER TABLE "blc_product_option_value" ALTER COLUMN "product_option_id" SET NOT NULL;
ALTER TABLE "blc_category_media_map" ALTER COLUMN "media_id" SET NOT NULL;
ALTER TABLE "blc_product_option" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "active_end_date" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "active_start_date" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "available_flag" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "discountable_flag" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "external_id" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "taxable_flag" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "upc" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "url_key" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "default_product_id" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "currency_code" SET NOT NULL;
ALTER TABLE "blc_sku" ALTER COLUMN "addl_product_id" SET NOT NULL;
ALTER TABLE "blc_order_item_price_dtl" ALTER COLUMN "order_item_id" SET NOT NULL;
ALTER TABLE "blc_page_tmplt" ALTER COLUMN "locale_code" SET NOT NULL;
ALTER TABLE "blc_order" ALTER COLUMN "email_address" SET NOT NULL;
ALTER TABLE "blc_order" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "blc_order" ALTER COLUMN "order_number" SET NOT NULL;
ALTER TABLE "blc_order" ALTER COLUMN "order_status" SET NOT NULL;
ALTER TABLE "blc_order" ALTER COLUMN "locale_code" SET NOT NULL;
ALTER TABLE "blc_order" ALTER COLUMN "currency_code" SET NOT NULL;
ALTER TABLE "blc_order_adjustment" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "blc_email_tracking_clicks" ALTER COLUMN "customer_id" SET NOT NULL;
ALTER TABLE "blc_sku_media_map" ALTER COLUMN "media_id" SET NOT NULL;
ALTER TABLE "blc_customer_phone" ALTER COLUMN "phone_name" SET NOT NULL;
ALTER TABLE "blc_tax_detail" ALTER COLUMN "module_config_id" SET NOT NULL;
ALTER TABLE "blc_tax_detail" ALTER COLUMN "currency_code" SET NOT NULL;
ALTER TABLE "blc_sku_fee" ALTER COLUMN "currency_code" SET NOT NULL;
ALTER TABLE "blc_offer" ALTER COLUMN "offer_discount_type" SET NOT NULL;
ALTER TABLE "blc_offer" ALTER COLUMN "marketing_messasge" SET NOT NULL;
ALTER TABLE "blc_fld_enum_item" ALTER COLUMN "fld_enum_id" SET NOT NULL;
ALTER TABLE "blc_fg_adjustment" ALTER COLUMN "fulfillment_group_id" SET NOT NULL;
ALTER TABLE "blc_fulfillment_price_band" ALTER COLUMN "fulfillment_option_id" SET NOT NULL;
ALTER TABLE "blc_sandbox" ALTER COLUMN "sandbox_name" SET NOT NULL;
ALTER TABLE "blc_sandbox" ALTER COLUMN "parent_sandbox_id" SET NOT NULL;
ALTER TABLE "blc_qual_crit_offer_xref" ALTER COLUMN "offer_item_criteria_id" SET NOT NULL;
ALTER TABLE "blc_search_facet_range" ALTER COLUMN "search_facet_id" SET NOT NULL;
ALTER TABLE "blc_prorated_order_item_adjust" ALTER COLUMN "order_item_id" SET NOT NULL;
ALTER TABLE "blc_pgtmplt_fldgrp_xref" ALTER COLUMN "fld_group_id" SET NOT NULL;
ALTER TABLE "blc_pgtmplt_fldgrp_xref" ALTER COLUMN "page_tmplt_id" SET NOT NULL;
ALTER TABLE "blc_data_drvn_enum" ALTER COLUMN "enum_key" SET NOT NULL;
ALTER TABLE "blc_customer" ALTER COLUMN "email_address" SET NOT NULL;
ALTER TABLE "blc_customer" ALTER COLUMN "locale_code" SET NOT NULL;
ALTER TABLE "blc_customer" ALTER COLUMN "challenge_question_id" SET NOT NULL;
ALTER TABLE "blc_category" ALTER COLUMN "external_id" SET NOT NULL;
ALTER TABLE "blc_category" ALTER COLUMN "url" SET NOT NULL;
ALTER TABLE "blc_category" ALTER COLUMN "url_key" SET NOT NULL;
ALTER TABLE "blc_category" ALTER COLUMN "default_parent_category_id" SET NOT NULL;
ALTER TABLE "blc_email_tracking_opens" ALTER COLUMN "email_tracking_id" SET NOT NULL;
ALTER TABLE "blc_shipping_rate" ALTER COLUMN "fee_sub_type" SET NOT NULL;
ALTER TABLE "blc_email_tracking" ALTER COLUMN "email_address" SET NOT NULL;
ALTER TABLE "blc_fulfillment_group_item" ALTER COLUMN "status" SET NOT NULL;
ALTER TABLE "blc_search_intercept" ALTER COLUMN "active_end_date" SET NOT NULL;
ALTER TABLE "blc_review_detail" ALTER COLUMN "rating_detail_id" SET NOT NULL;
ALTER TABLE "blc_product_featured" ALTER COLUMN "category_id" SET NOT NULL;
ALTER TABLE "blc_product_featured" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "blc_index_field" ALTER COLUMN "searchable" SET NOT NULL;
ALTER TABLE "blc_promotion_message" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "blc_promotion_message" ALTER COLUMN "locale_code" SET NOT NULL;
ALTER TABLE "blc_promotion_message" ALTER COLUMN "media_id" SET NOT NULL;
ALTER TABLE "blc_offer_audit" ALTER COLUMN "customer_id" SET NOT NULL;
ALTER TABLE "blc_offer_audit" ALTER COLUMN "offer_code_id" SET NOT NULL;
ALTER TABLE "blc_offer_audit" ALTER COLUMN "offer_id" SET NOT NULL;
ALTER TABLE "blc_offer_audit" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "blc_url_handler" ALTER COLUMN "is_regex" SET NOT NULL;
ALTER TABLE "blc_offer_rule_map" ALTER COLUMN "offer_rule_id" SET NOT NULL;
ALTER TABLE "blc_store" ALTER COLUMN "address_id" SET NOT NULL;
ALTER TABLE "blc_item_offer_qualifier" ALTER COLUMN "order_item_id" SET NOT NULL;
ALTER TABLE "blc_data_drvn_enum_val" ALTER COLUMN "hidden" SET NOT NULL;
ALTER TABLE "blc_data_drvn_enum_val" ALTER COLUMN "enum_key" SET NOT NULL;
ALTER TABLE "blc_data_drvn_enum_val" ALTER COLUMN "enum_type" SET NOT NULL;
ALTER TABLE "blc_candidate_fg_offer" ALTER COLUMN "fulfillment_group_id" SET NOT NULL;
ALTER TABLE "blc_sc" ALTER COLUMN "offline_flag" SET NOT NULL;
ALTER TABLE "blc_sc" ALTER COLUMN "sc_type_id" SET NOT NULL;
ALTER TABLE "blc_cat_search_facet_xref" ALTER COLUMN "category_id" SET NOT NULL;
ALTER TABLE "blc_cat_search_facet_xref" ALTER COLUMN "search_facet_id" SET NOT NULL;
ALTER TABLE "blc_customer_payment" ALTER COLUMN "payment_token" SET NOT NULL;
ALTER TABLE "blc_customer_payment" ALTER COLUMN "payment_type" SET NOT NULL;
ALTER TABLE "blc_customer_payment" ALTER COLUMN "address_id" SET NOT NULL;
ALTER TABLE "blc_search_synonym" ALTER COLUMN "term" SET NOT NULL;
ALTER TABLE "blc_order_payment" ALTER COLUMN "reference_number" SET NOT NULL;
ALTER TABLE "blc_order_payment" ALTER COLUMN "address_id" SET NOT NULL;
ALTER TABLE "blc_order_payment" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "blc_admin_user_sandbox" ALTER COLUMN "sandbox_id" SET NOT NULL;
ALTER TABLE "blc_translation" ALTER COLUMN "entity_type" SET NOT NULL;
ALTER TABLE "blc_translation" ALTER COLUMN "entity_id" SET NOT NULL;
ALTER TABLE "blc_translation" ALTER COLUMN "field_name" SET NOT NULL;
ALTER TABLE "blc_translation" ALTER COLUMN "locale_code" SET NOT NULL;
ALTER TABLE "blc_sc_type" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "blc_sc_type" ALTER COLUMN "sc_fld_tmplt_id" SET NOT NULL;
ALTER TABLE "blc_admin_permission_entity" ALTER COLUMN "admin_permission_id" SET NOT NULL;
ALTER TABLE "blc_fulfillment_option_fixed" ALTER COLUMN "currency_code" SET NOT NULL;
ALTER TABLE "blc_sc_fld_map" ALTER COLUMN "sc_fld_id" SET NOT NULL;
ALTER TABLE "blc_offer_code" ALTER COLUMN "email_address" SET NOT NULL;
ALTER TABLE "blc_sc_fldgrp_xref" ALTER COLUMN "fld_group_id" SET NOT NULL;
ALTER TABLE "blc_sc_fldgrp_xref" ALTER COLUMN "sc_fld_tmplt_id" SET NOT NULL;
ALTER TABLE "blc_discrete_order_item" ALTER COLUMN "sku_bundle_item_id" SET NOT NULL;
ALTER TABLE "blc_discrete_order_item" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "blc_discrete_order_item" ALTER COLUMN "bundle_order_item_id" SET NOT NULL;
ALTER TABLE "blc_order_multiship_option" ALTER COLUMN "fulfillment_option_id" SET NOT NULL;
ALTER TABLE "blc_order_multiship_option" ALTER COLUMN "order_item_id" SET NOT NULL;
ALTER TABLE "blc_order_multiship_option" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "blc_order_multiship_option" ALTER COLUMN "address_id" SET NOT NULL;
ALTER TABLE "blc_site" ALTER COLUMN "site_identifier_value" SET NOT NULL;
ALTER TABLE "blc_fulfillment_weight_band" ALTER COLUMN "fulfillment_option_id" SET NOT NULL;
ALTER TABLE "blc_order_item" ALTER COLUMN "order_item_type" SET NOT NULL;
ALTER TABLE "blc_order_item" ALTER COLUMN "gift_wrap_item_id" SET NOT NULL;
ALTER TABLE "blc_order_item" ALTER COLUMN "parent_order_item_id" SET NOT NULL;
ALTER TABLE "blc_order_item" ALTER COLUMN "category_id" SET NOT NULL;
ALTER TABLE "blc_order_item" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "blc_order_item" ALTER COLUMN "personal_message_id" SET NOT NULL;
ALTER TABLE "blc_fulfillment_group" ALTER COLUMN "method" SET NOT NULL;
ALTER TABLE "blc_fulfillment_group" ALTER COLUMN "is_primary" SET NOT NULL;
ALTER TABLE "blc_fulfillment_group" ALTER COLUMN "reference_number" SET NOT NULL;
ALTER TABLE "blc_fulfillment_group" ALTER COLUMN "service" SET NOT NULL;
ALTER TABLE "blc_fulfillment_group" ALTER COLUMN "status" SET NOT NULL;
ALTER TABLE "blc_fulfillment_group" ALTER COLUMN "fulfillment_option_id" SET NOT NULL;
ALTER TABLE "blc_fulfillment_group" ALTER COLUMN "phone_id" SET NOT NULL;
ALTER TABLE "blc_fulfillment_group" ALTER COLUMN "personal_message_id" SET NOT NULL;
ALTER TABLE "blc_fulfillment_group" ALTER COLUMN "address_id" SET NOT NULL;
ALTER TABLE "blc_product_up_sale" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "blc_product_up_sale" ALTER COLUMN "category_id" SET NOT NULL;
ALTER TABLE "blc_product_up_sale" ALTER COLUMN "related_sale_product_id" SET NOT NULL;
ALTER TABLE "blc_cms_menu_item" ALTER COLUMN "parent_menu_id" SET NOT NULL;
ALTER TABLE "blc_cms_menu_item" ALTER COLUMN "linked_page_id" SET NOT NULL;
ALTER TABLE "blc_cms_menu_item" ALTER COLUMN "linked_menu_id" SET NOT NULL;
ALTER TABLE "blc_cat_search_facet_excl_xref" ALTER COLUMN "category_id" SET NOT NULL;
ALTER TABLE "blc_cat_search_facet_excl_xref" ALTER COLUMN "search_facet_id" SET NOT NULL;
ALTER TABLE "blc_candidate_order_offer" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "blc_candidate_item_offer" ALTER COLUMN "order_item_id" SET NOT NULL;
ALTER TABLE "blc_sc_item_criteria" ALTER COLUMN "sc_id" SET NOT NULL;
ALTER TABLE "blc_sku_availability" ALTER COLUMN "availability_status" SET NOT NULL;
ALTER TABLE "blc_sku_availability" ALTER COLUMN "location_id" SET NOT NULL;
ALTER TABLE "blc_sku_availability" ALTER COLUMN "sku_id" SET NOT NULL;
ALTER TABLE "blc_media" ALTER COLUMN "title" SET NOT NULL;
ALTER TABLE "blc_payment_log" ALTER COLUMN "order_payment_id" SET NOT NULL;
ALTER TABLE "blc_payment_log" ALTER COLUMN "order_payment_ref_num" SET NOT NULL;
ALTER TABLE "blc_payment_log" ALTER COLUMN "currency_code" SET NOT NULL;
ALTER TABLE "blc_payment_log" ALTER COLUMN "customer_id" SET NOT NULL;
ALTER TABLE "blc_order_payment_transaction" ALTER COLUMN "parent_transaction" SET NOT NULL;
ALTER TABLE "blc_fld_def" ALTER COLUMN "enum_id" SET NOT NULL;
ALTER TABLE "blc_fld_def" ALTER COLUMN "fld_group_id" SET NOT NULL;
ALTER TABLE "blc_zip_code" ALTER COLUMN "zip_city" SET NOT NULL;
ALTER TABLE "blc_zip_code" ALTER COLUMN "zip_latitude" SET NOT NULL;
ALTER TABLE "blc_zip_code" ALTER COLUMN "zip_longitude" SET NOT NULL;
ALTER TABLE "blc_zip_code" ALTER COLUMN "zip_state" SET NOT NULL;
ALTER TABLE "blc_zip_code" ALTER COLUMN "zipcode" SET NOT NULL;
ALTER TABLE "blc_admin_permission_xref" ADD CONSTRAINT "wetune_u_38c8262f2cb344eb" UNIQUE ("child_permission_id", "admin_permission_id");
ALTER TABLE "blc_sku_fulfillment_excluded" ADD CONSTRAINT "wetune_u_29150718d856a169" UNIQUE ("sku_id", "fulfillment_option_id");
ALTER TABLE "blc_admin_sec_perm_xref" ADD CONSTRAINT "wetune_u_df6d7bb91ceead9d" UNIQUE ("admin_section_id", "admin_permission_id");
ALTER TABLE "blc_sku_fee_xref" ADD CONSTRAINT "wetune_u_9eed6672ccc7b9fc" UNIQUE ("sku_fee_id", "sku_id");
ALTER TABLE "blc_order_offer_code_xref" ADD CONSTRAINT "wetune_u_d4ba4b771f4c0acf" UNIQUE ("order_id", "offer_code_id");
