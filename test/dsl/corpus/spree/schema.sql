CREATE TABLE "action_mailbox_inbound_emails" (
  "id" BIGINT NOT NULL,
  "status" INTEGER NOT NULL,
  "message_id" VARCHAR(255) NOT NULL,
  "message_checksum" VARCHAR(255) NOT NULL,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("message_id", "message_checksum")
);

CREATE TABLE "action_text_rich_texts" (
  "id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "body" TEXT,
  "record_type" VARCHAR(255) NOT NULL,
  "record_id" BIGINT NOT NULL,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("record_type", "record_id", "name")
);

CREATE TABLE "active_storage_attachments" (
  "id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "record_type" VARCHAR(255) NOT NULL,
  "record_id" BIGINT NOT NULL,
  "blob_id" BIGINT NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("record_type", "record_id", "name", "blob_id")
);

CREATE TABLE "active_storage_blobs" (
  "id" BIGINT NOT NULL,
  "key" VARCHAR(255) NOT NULL,
  "filename" VARCHAR(255) NOT NULL,
  "content_type" VARCHAR(255),
  "metadata" TEXT,
  "byte_size" BIGINT NOT NULL,
  "checksum" VARCHAR(255) NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("key")
);

CREATE TABLE "ar_internal_metadata" (
  "key" VARCHAR(255) NOT NULL,
  "value" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("key")
);

CREATE TABLE "friendly_id_slugs" (
  "id" INTEGER NOT NULL,
  "slug" VARCHAR(255) NOT NULL,
  "sluggable_id" INTEGER NOT NULL,
  "sluggable_type" VARCHAR(50),
  "scope" VARCHAR(255),
  "created_at" TIMESTAMP,
  "deleted_at" TIMESTAMP,
  PRIMARY KEY ("id"),
  UNIQUE ("slug", "sluggable_type", "scope")
);

CREATE TABLE "schema_migrations" (
  "version" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("version")
);

CREATE TABLE "spree_addresses" (
  "id" INTEGER NOT NULL,
  "firstname" VARCHAR(255),
  "lastname" VARCHAR(255),
  "address1" VARCHAR(255),
  "address2" VARCHAR(255),
  "city" VARCHAR(255),
  "zipcode" VARCHAR(255),
  "phone" VARCHAR(255),
  "state_name" VARCHAR(255),
  "alternative_phone" VARCHAR(255),
  "company" VARCHAR(255),
  "state_id" INTEGER,
  "country_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "user_id" INTEGER,
  "deleted_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_adjustments" (
  "id" INTEGER NOT NULL,
  "source_type" VARCHAR(255),
  "source_id" INTEGER,
  "adjustable_type" VARCHAR(255),
  "adjustable_id" INTEGER,
  "amount" DECIMAL(10, 2),
  "label" VARCHAR(255),
  "mandatory" SMALLINT,
  "eligible" SMALLINT,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "state" VARCHAR(255),
  "order_id" INTEGER NOT NULL,
  "included" SMALLINT,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_assets" (
  "id" INTEGER NOT NULL,
  "viewable_type" VARCHAR(255),
  "viewable_id" INTEGER,
  "attachment_width" INTEGER,
  "attachment_height" INTEGER,
  "attachment_file_size" INTEGER,
  "position" INTEGER,
  "attachment_content_type" VARCHAR(255),
  "attachment_file_name" VARCHAR(255),
  "type" VARCHAR(75),
  "attachment_updated_at" TIMESTAMP,
  "alt" TEXT,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_calculators" (
  "id" INTEGER NOT NULL,
  "type" VARCHAR(255),
  "calculable_type" VARCHAR(255),
  "calculable_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "preferences" TEXT,
  "deleted_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_countries" (
  "id" INTEGER NOT NULL,
  "iso_name" VARCHAR(255),
  "iso" VARCHAR(255) NOT NULL,
  "iso3" VARCHAR(255) NOT NULL,
  "name" VARCHAR(255),
  "numcode" INTEGER,
  "states_required" SMALLINT,
  "updated_at" TIMESTAMP,
  "zipcode_required" SMALLINT,
  PRIMARY KEY ("id"),
  UNIQUE ("iso"),
  UNIQUE ("iso3"),
  UNIQUE ("name"),
  UNIQUE ("iso_name")
);

CREATE TABLE "spree_credit_cards" (
  "id" INTEGER NOT NULL,
  "month" VARCHAR(255),
  "year" VARCHAR(255),
  "cc_type" VARCHAR(255),
  "last_digits" VARCHAR(255),
  "address_id" INTEGER,
  "gateway_customer_profile_id" VARCHAR(255),
  "gateway_payment_profile_id" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "name" VARCHAR(255),
  "user_id" INTEGER,
  "payment_method_id" INTEGER,
  "default" SMALLINT NOT NULL,
  "deleted_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_customer_returns" (
  "id" INTEGER NOT NULL,
  "number" VARCHAR(255),
  "stock_location_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("number")
);

CREATE TABLE "spree_dummy_models" (
  "id" BIGINT NOT NULL,
  "name" VARCHAR(255),
  "position" INTEGER,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_gateways" (
  "id" INTEGER NOT NULL,
  "type" VARCHAR(255),
  "name" VARCHAR(255),
  "description" TEXT,
  "active" SMALLINT,
  "environment" VARCHAR(255),
  "server" VARCHAR(255),
  "test_mode" SMALLINT,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "preferences" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_inventory_units" (
  "id" INTEGER NOT NULL,
  "state" VARCHAR(255),
  "variant_id" INTEGER,
  "order_id" INTEGER,
  "shipment_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "pending" SMALLINT,
  "line_item_id" INTEGER,
  "quantity" INTEGER,
  "original_return_item_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_line_items" (
  "id" INTEGER NOT NULL,
  "variant_id" INTEGER,
  "order_id" INTEGER,
  "quantity" INTEGER NOT NULL,
  "price" DECIMAL(10, 2) NOT NULL,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "currency" VARCHAR(255),
  "cost_price" DECIMAL(10, 2),
  "tax_category_id" INTEGER,
  "adjustment_total" DECIMAL(10, 2),
  "additional_tax_total" DECIMAL(10, 2),
  "promo_total" DECIMAL(10, 2),
  "included_tax_total" DECIMAL(10, 2) NOT NULL,
  "pre_tax_amount" DECIMAL(12, 4) NOT NULL,
  "taxable_adjustment_total" DECIMAL(10, 2) NOT NULL,
  "non_taxable_adjustment_total" DECIMAL(10, 2) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_log_entries" (
  "id" INTEGER NOT NULL,
  "source_type" VARCHAR(255),
  "source_id" INTEGER,
  "details" TEXT,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_oauth_access_grants" (
  "id" BIGINT NOT NULL,
  "resource_owner_id" INTEGER NOT NULL,
  "application_id" BIGINT NOT NULL,
  "token" VARCHAR(255) NOT NULL,
  "expires_in" INTEGER NOT NULL,
  "redirect_uri" TEXT NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "revoked_at" TIMESTAMP,
  "scopes" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("token")
);

CREATE TABLE "spree_oauth_access_tokens" (
  "id" BIGINT NOT NULL,
  "resource_owner_id" INTEGER,
  "application_id" BIGINT,
  "token" VARCHAR(255) NOT NULL,
  "refresh_token" VARCHAR(255),
  "expires_in" INTEGER,
  "revoked_at" TIMESTAMP,
  "created_at" TIMESTAMP NOT NULL,
  "scopes" VARCHAR(255),
  "previous_refresh_token" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("token"),
  UNIQUE ("refresh_token")
);

CREATE TABLE "spree_oauth_applications" (
  "id" BIGINT NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "uid" VARCHAR(255) NOT NULL,
  "secret" VARCHAR(255) NOT NULL,
  "redirect_uri" TEXT NOT NULL,
  "scopes" VARCHAR(255) NOT NULL,
  "confidential" SMALLINT NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("uid")
);

CREATE TABLE "spree_option_type_prototypes" (
  "prototype_id" INTEGER,
  "option_type_id" INTEGER,
  "id" INTEGER NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("prototype_id", "option_type_id")
);

CREATE TABLE "spree_option_types" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(100),
  "presentation" VARCHAR(100),
  "position" INTEGER NOT NULL,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_option_value_variants" (
  "variant_id" INTEGER,
  "option_value_id" INTEGER,
  "id" INTEGER NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("variant_id", "option_value_id")
);

CREATE TABLE "spree_option_values" (
  "id" INTEGER NOT NULL,
  "position" INTEGER,
  "name" VARCHAR(255),
  "presentation" VARCHAR(255),
  "option_type_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_order_promotions" (
  "order_id" INTEGER,
  "promotion_id" INTEGER,
  "id" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_orders" (
  "id" INTEGER NOT NULL,
  "number" VARCHAR(32),
  "item_total" DECIMAL(10, 2) NOT NULL,
  "total" DECIMAL(10, 2) NOT NULL,
  "state" VARCHAR(255),
  "adjustment_total" DECIMAL(10, 2) NOT NULL,
  "user_id" INTEGER,
  "completed_at" TIMESTAMP,
  "bill_address_id" INTEGER,
  "ship_address_id" INTEGER,
  "payment_total" DECIMAL(10, 2),
  "shipment_state" VARCHAR(255),
  "payment_state" VARCHAR(255),
  "email" VARCHAR(255),
  "special_instructions" TEXT,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "currency" VARCHAR(255),
  "last_ip_address" VARCHAR(255),
  "created_by_id" INTEGER,
  "shipment_total" DECIMAL(10, 2) NOT NULL,
  "additional_tax_total" DECIMAL(10, 2),
  "promo_total" DECIMAL(10, 2),
  "channel" VARCHAR(255),
  "included_tax_total" DECIMAL(10, 2) NOT NULL,
  "item_count" INTEGER,
  "approver_id" INTEGER,
  "approved_at" TIMESTAMP,
  "confirmation_delivered" SMALLINT,
  "considered_risky" SMALLINT,
  "token" VARCHAR(255),
  "canceled_at" TIMESTAMP,
  "canceler_id" INTEGER,
  "store_id" INTEGER,
  "state_lock_version" INTEGER NOT NULL,
  "taxable_adjustment_total" DECIMAL(10, 2) NOT NULL,
  "non_taxable_adjustment_total" DECIMAL(10, 2) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("number")
);

CREATE TABLE "spree_payment_capture_events" (
  "id" INTEGER NOT NULL,
  "amount" DECIMAL(10, 2),
  "payment_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_payment_methods" (
  "id" INTEGER NOT NULL,
  "type" VARCHAR(255),
  "name" VARCHAR(255),
  "description" TEXT,
  "active" SMALLINT,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "display_on" VARCHAR(255),
  "auto_capture" SMALLINT,
  "preferences" TEXT,
  "position" INTEGER,
  "store_id" BIGINT,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_payments" (
  "id" INTEGER NOT NULL,
  "amount" DECIMAL(10, 2) NOT NULL,
  "order_id" INTEGER,
  "source_type" VARCHAR(255),
  "source_id" INTEGER,
  "payment_method_id" INTEGER,
  "state" VARCHAR(255),
  "response_code" VARCHAR(255),
  "avs_response" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "number" VARCHAR(255),
  "cvv_response_code" VARCHAR(255),
  "cvv_response_message" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("number")
);

CREATE TABLE "spree_preferences" (
  "id" INTEGER NOT NULL,
  "value" TEXT,
  "key" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("key")
);

CREATE TABLE "spree_prices" (
  "id" INTEGER NOT NULL,
  "variant_id" INTEGER NOT NULL,
  "amount" DECIMAL(10, 2),
  "currency" VARCHAR(255),
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_product_option_types" (
  "id" INTEGER NOT NULL,
  "position" INTEGER,
  "product_id" INTEGER,
  "option_type_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_product_promotion_rules" (
  "product_id" INTEGER,
  "promotion_rule_id" INTEGER,
  "id" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_product_properties" (
  "id" INTEGER NOT NULL,
  "value" VARCHAR(255),
  "product_id" INTEGER,
  "property_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "position" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_products" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "description" TEXT,
  "available_on" TIMESTAMP,
  "discontinue_on" TIMESTAMP,
  "deleted_at" TIMESTAMP,
  "slug" VARCHAR(255),
  "meta_description" TEXT,
  "meta_keywords" VARCHAR(255),
  "tax_category_id" INTEGER,
  "shipping_category_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "promotionable" SMALLINT,
  "meta_title" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("slug")
);

CREATE TABLE "spree_products_taxons" (
  "product_id" INTEGER,
  "taxon_id" INTEGER,
  "id" INTEGER NOT NULL,
  "position" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_action_line_items" (
  "id" INTEGER NOT NULL,
  "promotion_action_id" INTEGER,
  "variant_id" INTEGER,
  "quantity" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_actions" (
  "id" INTEGER NOT NULL,
  "promotion_id" INTEGER,
  "position" INTEGER,
  "type" VARCHAR(255),
  "deleted_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_categories" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "code" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_rule_taxons" (
  "id" INTEGER NOT NULL,
  "taxon_id" INTEGER,
  "promotion_rule_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_rule_users" (
  "user_id" INTEGER,
  "promotion_rule_id" INTEGER,
  "id" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_rules" (
  "id" INTEGER NOT NULL,
  "promotion_id" INTEGER,
  "user_id" INTEGER,
  "product_group_id" INTEGER,
  "type" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "code" VARCHAR(255),
  "preferences" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotions" (
  "id" INTEGER NOT NULL,
  "description" VARCHAR(255),
  "expires_at" TIMESTAMP,
  "starts_at" TIMESTAMP,
  "name" VARCHAR(255),
  "type" VARCHAR(255),
  "usage_limit" INTEGER,
  "match_policy" VARCHAR(255),
  "code" VARCHAR(255),
  "advertise" SMALLINT,
  "path" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "promotion_category_id" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("code")
);

CREATE TABLE "spree_properties" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "presentation" VARCHAR(255) NOT NULL,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_property_prototypes" (
  "prototype_id" INTEGER,
  "property_id" INTEGER,
  "id" INTEGER NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("prototype_id", "property_id")
);

CREATE TABLE "spree_prototype_taxons" (
  "id" INTEGER NOT NULL,
  "taxon_id" INTEGER,
  "prototype_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_prototypes" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_refund_reasons" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "active" SMALLINT,
  "mutable" SMALLINT,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("name")
);

CREATE TABLE "spree_refunds" (
  "id" INTEGER NOT NULL,
  "payment_id" INTEGER,
  "amount" DECIMAL(10, 2) NOT NULL,
  "transaction_id" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "refund_reason_id" INTEGER,
  "reimbursement_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_reimbursement_credits" (
  "id" INTEGER NOT NULL,
  "amount" DECIMAL(10, 2) NOT NULL,
  "reimbursement_id" INTEGER,
  "creditable_id" INTEGER,
  "creditable_type" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_reimbursement_types" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "active" SMALLINT,
  "mutable" SMALLINT,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "type" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("name")
);

CREATE TABLE "spree_reimbursements" (
  "id" INTEGER NOT NULL,
  "number" VARCHAR(255),
  "reimbursement_status" VARCHAR(255),
  "customer_return_id" INTEGER,
  "order_id" INTEGER,
  "total" DECIMAL(10, 2),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("number")
);

CREATE TABLE "spree_return_authorization_reasons" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "active" SMALLINT,
  "mutable" SMALLINT,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("name")
);

CREATE TABLE "spree_return_authorizations" (
  "id" INTEGER NOT NULL,
  "number" VARCHAR(255),
  "state" VARCHAR(255),
  "order_id" INTEGER,
  "memo" TEXT,
  "created_at" TIMESTAMP,
  "updated_at" TIMESTAMP,
  "stock_location_id" INTEGER,
  "return_authorization_reason_id" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("number")
);

CREATE TABLE "spree_return_items" (
  "id" INTEGER NOT NULL,
  "return_authorization_id" INTEGER,
  "inventory_unit_id" INTEGER,
  "exchange_variant_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "pre_tax_amount" DECIMAL(12, 4) NOT NULL,
  "included_tax_total" DECIMAL(12, 4) NOT NULL,
  "additional_tax_total" DECIMAL(12, 4) NOT NULL,
  "reception_status" VARCHAR(255),
  "acceptance_status" VARCHAR(255),
  "customer_return_id" INTEGER,
  "reimbursement_id" INTEGER,
  "acceptance_status_errors" TEXT,
  "preferred_reimbursement_type_id" INTEGER,
  "override_reimbursement_type_id" INTEGER,
  "resellable" SMALLINT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_role_users" (
  "role_id" INTEGER,
  "user_id" INTEGER,
  "id" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_roles" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("name")
);

CREATE TABLE "spree_shipments" (
  "id" INTEGER NOT NULL,
  "tracking" VARCHAR(255),
  "number" VARCHAR(255),
  "cost" DECIMAL(10, 2),
  "shipped_at" TIMESTAMP,
  "order_id" INTEGER,
  "address_id" INTEGER,
  "state" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "stock_location_id" INTEGER,
  "adjustment_total" DECIMAL(10, 2),
  "additional_tax_total" DECIMAL(10, 2),
  "promo_total" DECIMAL(10, 2),
  "included_tax_total" DECIMAL(10, 2) NOT NULL,
  "pre_tax_amount" DECIMAL(12, 4) NOT NULL,
  "taxable_adjustment_total" DECIMAL(10, 2) NOT NULL,
  "non_taxable_adjustment_total" DECIMAL(10, 2) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("number")
);

CREATE TABLE "spree_shipping_categories" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_shipping_method_categories" (
  "id" INTEGER NOT NULL,
  "shipping_method_id" INTEGER NOT NULL,
  "shipping_category_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("shipping_category_id", "shipping_method_id")
);

CREATE TABLE "spree_shipping_method_zones" (
  "shipping_method_id" INTEGER,
  "zone_id" INTEGER,
  "id" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_shipping_methods" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "display_on" VARCHAR(255),
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "tracking_url" VARCHAR(255),
  "admin_name" VARCHAR(255),
  "tax_category_id" INTEGER,
  "code" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_shipping_rates" (
  "id" INTEGER NOT NULL,
  "shipment_id" INTEGER,
  "shipping_method_id" INTEGER,
  "selected" SMALLINT,
  "cost" DECIMAL(8, 2),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "tax_rate_id" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("shipment_id", "shipping_method_id")
);

CREATE TABLE "spree_state_changes" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "previous_state" VARCHAR(255),
  "stateful_id" INTEGER,
  "user_id" INTEGER,
  "stateful_type" VARCHAR(255),
  "next_state" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_states" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "abbr" VARCHAR(255),
  "country_id" INTEGER,
  "updated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_stock_items" (
  "id" INTEGER NOT NULL,
  "stock_location_id" INTEGER,
  "variant_id" INTEGER,
  "count_on_hand" INTEGER NOT NULL,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "backorderable" SMALLINT,
  "deleted_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_stock_locations" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "default" SMALLINT NOT NULL,
  "address1" VARCHAR(255),
  "address2" VARCHAR(255),
  "city" VARCHAR(255),
  "state_id" INTEGER,
  "state_name" VARCHAR(255),
  "country_id" INTEGER,
  "zipcode" VARCHAR(255),
  "phone" VARCHAR(255),
  "active" SMALLINT,
  "backorderable_default" SMALLINT,
  "propagate_all_variants" SMALLINT,
  "admin_name" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_stock_movements" (
  "id" INTEGER NOT NULL,
  "stock_item_id" INTEGER,
  "quantity" INTEGER,
  "action" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "originator_type" VARCHAR(255),
  "originator_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_stock_transfers" (
  "id" INTEGER NOT NULL,
  "type" VARCHAR(255),
  "reference" VARCHAR(255),
  "source_location_id" INTEGER,
  "destination_location_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "number" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("number")
);

CREATE TABLE "spree_store_credit_categories" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_store_credit_events" (
  "id" INTEGER NOT NULL,
  "store_credit_id" INTEGER NOT NULL,
  "action" VARCHAR(255) NOT NULL,
  "amount" DECIMAL(8, 2),
  "authorization_code" VARCHAR(255) NOT NULL,
  "user_total_amount" DECIMAL(8, 2) NOT NULL,
  "originator_id" INTEGER,
  "originator_type" VARCHAR(255),
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_store_credit_types" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "priority" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_store_credits" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "category_id" INTEGER,
  "created_by_id" INTEGER,
  "amount" DECIMAL(8, 2) NOT NULL,
  "amount_used" DECIMAL(8, 2) NOT NULL,
  "memo" TEXT,
  "deleted_at" TIMESTAMP,
  "currency" VARCHAR(255),
  "amount_authorized" DECIMAL(8, 2) NOT NULL,
  "originator_id" INTEGER,
  "originator_type" VARCHAR(255),
  "type_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_stores" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "url" VARCHAR(255),
  "meta_description" TEXT,
  "meta_keywords" TEXT,
  "seo_title" VARCHAR(255),
  "mail_from_address" VARCHAR(255),
  "default_currency" VARCHAR(255),
  "code" VARCHAR(255),
  "default" SMALLINT NOT NULL,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "facebook" VARCHAR(255),
  "twitter" VARCHAR(255),
  "instagram" VARCHAR(255),
  "default_locale" VARCHAR(255),
  "customer_support_email" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("code")
);

CREATE TABLE "spree_tax_categories" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "description" VARCHAR(255),
  "is_default" SMALLINT,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "tax_code" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_tax_rates" (
  "id" INTEGER NOT NULL,
  "amount" DECIMAL(8, 5),
  "zone_id" INTEGER,
  "tax_category_id" INTEGER,
  "included_in_price" SMALLINT,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "name" VARCHAR(255),
  "show_rate_in_label" SMALLINT,
  "deleted_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_taxonomies" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "position" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_taxons" (
  "id" INTEGER NOT NULL,
  "parent_id" INTEGER,
  "position" INTEGER,
  "name" VARCHAR(255) NOT NULL,
  "permalink" VARCHAR(255),
  "taxonomy_id" INTEGER,
  "lft" INTEGER,
  "rgt" INTEGER,
  "description" TEXT,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "meta_title" VARCHAR(255),
  "meta_description" VARCHAR(255),
  "meta_keywords" VARCHAR(255),
  "depth" INTEGER,
  "hide_from_nav" SMALLINT,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_trackers" (
  "id" INTEGER NOT NULL,
  "analytics_id" VARCHAR(255),
  "active" SMALLINT,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "engine" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_users" (
  "id" INTEGER NOT NULL,
  "encrypted_password" VARCHAR(128),
  "password_salt" VARCHAR(128),
  "email" VARCHAR(255),
  "remember_token" VARCHAR(255),
  "persistence_token" VARCHAR(255),
  "reset_password_token" VARCHAR(255),
  "perishable_token" VARCHAR(255),
  "sign_in_count" INTEGER NOT NULL,
  "failed_attempts" INTEGER NOT NULL,
  "last_request_at" TIMESTAMP,
  "current_sign_in_at" TIMESTAMP,
  "last_sign_in_at" TIMESTAMP,
  "current_sign_in_ip" VARCHAR(255),
  "last_sign_in_ip" VARCHAR(255),
  "login" VARCHAR(255),
  "ship_address_id" INTEGER,
  "bill_address_id" INTEGER,
  "authentication_token" VARCHAR(255),
  "unlock_token" VARCHAR(255),
  "locked_at" TIMESTAMP,
  "remember_created_at" TIMESTAMP,
  "reset_password_sent_at" TIMESTAMP,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "spree_api_key" VARCHAR(48),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_variants" (
  "id" INTEGER NOT NULL,
  "sku" VARCHAR(255) NOT NULL,
  "weight" DECIMAL(8, 2),
  "height" DECIMAL(8, 2),
  "width" DECIMAL(8, 2),
  "depth" DECIMAL(8, 2),
  "deleted_at" TIMESTAMP,
  "discontinue_on" TIMESTAMP,
  "is_master" SMALLINT,
  "product_id" INTEGER,
  "cost_price" DECIMAL(10, 2),
  "cost_currency" VARCHAR(255),
  "position" INTEGER,
  "track_inventory" SMALLINT,
  "tax_category_id" INTEGER,
  "updated_at" TIMESTAMP NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "count_on_hand" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_zone_members" (
  "id" INTEGER NOT NULL,
  "zoneable_type" VARCHAR(255),
  "zoneable_id" INTEGER,
  "zone_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_zones" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "description" VARCHAR(255),
  "default_tax" SMALLINT,
  "zone_members_count" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "kind" VARCHAR(255),
  PRIMARY KEY ("id")
);

ALTER TABLE "active_storage_attachments" ADD FOREIGN KEY ("blob_id") REFERENCES "active_storage_blobs" ("id");

ALTER TABLE "spree_oauth_access_grants" ADD FOREIGN KEY ("application_id") REFERENCES "spree_oauth_applications" ("id");

ALTER TABLE "spree_oauth_access_tokens" ADD FOREIGN KEY ("application_id") REFERENCES "spree_oauth_applications" ("id");

-- WeTune schema patches
ALTER TABLE "spree_orders" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_tax_rates" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_tax_rates" ALTER COLUMN "included_in_price" SET NOT NULL;
ALTER TABLE "spree_tax_rates" ALTER COLUMN "show_rate_in_label" SET NOT NULL;
ALTER TABLE "spree_tax_rates" ALTER COLUMN "tax_category_id" SET NOT NULL;
ALTER TABLE "spree_tax_rates" ALTER COLUMN "zone_id" SET NOT NULL;
ALTER TABLE "spree_payments" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "spree_payments" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_payments" ALTER COLUMN "payment_method_id" SET NOT NULL;
ALTER TABLE "spree_payments" ALTER COLUMN "source_id" SET NOT NULL;
ALTER TABLE "spree_payments" ALTER COLUMN "source_type" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "permalink" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "taxonomy_id" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "lft" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "rgt" SET NOT NULL;
ALTER TABLE "spree_option_type_prototypes" ALTER COLUMN "option_type_id" SET NOT NULL;
ALTER TABLE "spree_option_type_prototypes" ALTER COLUMN "prototype_id" SET NOT NULL;
ALTER TABLE "spree_roles" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "spree_assets" ALTER COLUMN "viewable_id" SET NOT NULL;
ALTER TABLE "spree_assets" ALTER COLUMN "viewable_type" SET NOT NULL;
ALTER TABLE "spree_assets" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_assets" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_option_value_variants" ALTER COLUMN "option_value_id" SET NOT NULL;
ALTER TABLE "spree_option_value_variants" ALTER COLUMN "variant_id" SET NOT NULL;
ALTER TABLE "spree_shipping_rates" ALTER COLUMN "selected" SET NOT NULL;
ALTER TABLE "spree_shipping_rates" ALTER COLUMN "tax_rate_id" SET NOT NULL;
ALTER TABLE "spree_shipping_rates" ALTER COLUMN "shipment_id" SET NOT NULL;
ALTER TABLE "spree_shipping_rates" ALTER COLUMN "shipping_method_id" SET NOT NULL;
ALTER TABLE "spree_promotion_rule_users" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_promotion_rule_users" ALTER COLUMN "promotion_rule_id" SET NOT NULL;
ALTER TABLE "spree_zone_members" ALTER COLUMN "zone_id" SET NOT NULL;
ALTER TABLE "spree_zone_members" ALTER COLUMN "zoneable_id" SET NOT NULL;
ALTER TABLE "spree_zone_members" ALTER COLUMN "zoneable_type" SET NOT NULL;
ALTER TABLE "spree_role_users" ALTER COLUMN "role_id" SET NOT NULL;
ALTER TABLE "spree_role_users" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_payment_capture_events" ALTER COLUMN "payment_id" SET NOT NULL;
ALTER TABLE "spree_taxonomies" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_store_credit_events" ALTER COLUMN "originator_id" SET NOT NULL;
ALTER TABLE "spree_store_credit_events" ALTER COLUMN "originator_type" SET NOT NULL;
ALTER TABLE "spree_stock_transfers" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "spree_stock_transfers" ALTER COLUMN "source_location_id" SET NOT NULL;
ALTER TABLE "spree_stock_transfers" ALTER COLUMN "destination_location_id" SET NOT NULL;
ALTER TABLE "spree_prototype_taxons" ALTER COLUMN "taxon_id" SET NOT NULL;
ALTER TABLE "spree_prototype_taxons" ALTER COLUMN "prototype_id" SET NOT NULL;
ALTER TABLE "spree_store_credits" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_store_credits" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_store_credits" ALTER COLUMN "type_id" SET NOT NULL;
ALTER TABLE "spree_store_credits" ALTER COLUMN "originator_id" SET NOT NULL;
ALTER TABLE "spree_store_credits" ALTER COLUMN "originator_type" SET NOT NULL;
ALTER TABLE "spree_zones" ALTER COLUMN "default_tax" SET NOT NULL;
ALTER TABLE "spree_zones" ALTER COLUMN "kind" SET NOT NULL;
ALTER TABLE "spree_shipments" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "spree_shipments" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_shipments" ALTER COLUMN "stock_location_id" SET NOT NULL;
ALTER TABLE "spree_shipments" ALTER COLUMN "address_id" SET NOT NULL;
ALTER TABLE "spree_users" ALTER COLUMN "spree_api_key" SET NOT NULL;
ALTER TABLE "spree_products" ALTER COLUMN "slug" SET NOT NULL;
ALTER TABLE "spree_products" ALTER COLUMN "available_on" SET NOT NULL;
ALTER TABLE "spree_products" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_products" ALTER COLUMN "shipping_category_id" SET NOT NULL;
ALTER TABLE "spree_products" ALTER COLUMN "tax_category_id" SET NOT NULL;
ALTER TABLE "spree_products" ALTER COLUMN "discontinue_on" SET NOT NULL;
ALTER TABLE "spree_promotion_action_line_items" ALTER COLUMN "promotion_action_id" SET NOT NULL;
ALTER TABLE "spree_promotion_action_line_items" ALTER COLUMN "variant_id" SET NOT NULL;
ALTER TABLE "spree_store_credit_types" ALTER COLUMN "priority" SET NOT NULL;
ALTER TABLE "spree_properties" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "spree_reimbursements" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "spree_reimbursements" ALTER COLUMN "customer_return_id" SET NOT NULL;
ALTER TABLE "spree_reimbursements" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_stock_items" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_stock_items" ALTER COLUMN "backorderable" SET NOT NULL;
ALTER TABLE "spree_stock_items" ALTER COLUMN "variant_id" SET NOT NULL;
ALTER TABLE "spree_stock_items" ALTER COLUMN "stock_location_id" SET NOT NULL;
ALTER TABLE "spree_reimbursement_credits" ALTER COLUMN "reimbursement_id" SET NOT NULL;
ALTER TABLE "spree_reimbursement_credits" ALTER COLUMN "creditable_id" SET NOT NULL;
ALTER TABLE "spree_reimbursement_credits" ALTER COLUMN "creditable_type" SET NOT NULL;
ALTER TABLE "spree_promotion_actions" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_promotion_actions" ALTER COLUMN "promotion_id" SET NOT NULL;
ALTER TABLE "spree_promotion_actions" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_property_prototypes" ALTER COLUMN "prototype_id" SET NOT NULL;
ALTER TABLE "spree_property_prototypes" ALTER COLUMN "property_id" SET NOT NULL;
ALTER TABLE "spree_countries" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "spree_countries" ALTER COLUMN "iso_name" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "code" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "expires_at" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "starts_at" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "advertise" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "promotion_category_id" SET NOT NULL;
ALTER TABLE "spree_reimbursement_types" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "spree_reimbursement_types" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_credit_cards" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_credit_cards" ALTER COLUMN "payment_method_id" SET NOT NULL;
ALTER TABLE "spree_credit_cards" ALTER COLUMN "address_id" SET NOT NULL;
ALTER TABLE "spree_credit_cards" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_option_types" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "spree_gateways" ALTER COLUMN "active" SET NOT NULL;
ALTER TABLE "spree_gateways" ALTER COLUMN "test_mode" SET NOT NULL;
ALTER TABLE "spree_line_items" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_line_items" ALTER COLUMN "variant_id" SET NOT NULL;
ALTER TABLE "spree_line_items" ALTER COLUMN "tax_category_id" SET NOT NULL;
ALTER TABLE "spree_stores" ALTER COLUMN "code" SET NOT NULL;
ALTER TABLE "spree_stores" ALTER COLUMN "url" SET NOT NULL;
ALTER TABLE "spree_calculators" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_calculators" ALTER COLUMN "calculable_id" SET NOT NULL;
ALTER TABLE "spree_calculators" ALTER COLUMN "calculable_type" SET NOT NULL;
ALTER TABLE "spree_calculators" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_state_changes" ALTER COLUMN "stateful_id" SET NOT NULL;
ALTER TABLE "spree_state_changes" ALTER COLUMN "stateful_type" SET NOT NULL;
ALTER TABLE "spree_promotion_rule_taxons" ALTER COLUMN "taxon_id" SET NOT NULL;
ALTER TABLE "spree_promotion_rule_taxons" ALTER COLUMN "promotion_rule_id" SET NOT NULL;
ALTER TABLE "spree_product_promotion_rules" ALTER COLUMN "promotion_rule_id" SET NOT NULL;
ALTER TABLE "spree_product_promotion_rules" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "spree_customer_returns" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "spree_customer_returns" ALTER COLUMN "stock_location_id" SET NOT NULL;
ALTER TABLE "spree_preferences" ALTER COLUMN "key" SET NOT NULL;
ALTER TABLE "friendly_id_slugs" ALTER COLUMN "scope" SET NOT NULL;
ALTER TABLE "friendly_id_slugs" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "friendly_id_slugs" ALTER COLUMN "sluggable_type" SET NOT NULL;
ALTER TABLE "spree_product_option_types" ALTER COLUMN "option_type_id" SET NOT NULL;
ALTER TABLE "spree_product_option_types" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "spree_product_option_types" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_trackers" ALTER COLUMN "active" SET NOT NULL;
ALTER TABLE "spree_promotion_rules" ALTER COLUMN "product_group_id" SET NOT NULL;
ALTER TABLE "spree_promotion_rules" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_promotion_rules" ALTER COLUMN "promotion_id" SET NOT NULL;
ALTER TABLE "spree_states" ALTER COLUMN "country_id" SET NOT NULL;
ALTER TABLE "spree_inventory_units" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_inventory_units" ALTER COLUMN "shipment_id" SET NOT NULL;
ALTER TABLE "spree_inventory_units" ALTER COLUMN "variant_id" SET NOT NULL;
ALTER TABLE "spree_inventory_units" ALTER COLUMN "line_item_id" SET NOT NULL;
ALTER TABLE "spree_inventory_units" ALTER COLUMN "original_return_item_id" SET NOT NULL;
ALTER TABLE "spree_oauth_access_tokens" ALTER COLUMN "refresh_token" SET NOT NULL;
ALTER TABLE "spree_oauth_access_tokens" ALTER COLUMN "resource_owner_id" SET NOT NULL;
ALTER TABLE "spree_oauth_access_tokens" ALTER COLUMN "application_id" SET NOT NULL;
ALTER TABLE "spree_order_promotions" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_order_promotions" ALTER COLUMN "promotion_id" SET NOT NULL;
ALTER TABLE "spree_payment_methods" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_payment_methods" ALTER COLUMN "store_id" SET NOT NULL;
ALTER TABLE "spree_variants" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "spree_variants" ALTER COLUMN "tax_category_id" SET NOT NULL;
ALTER TABLE "spree_variants" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_variants" ALTER COLUMN "is_master" SET NOT NULL;
ALTER TABLE "spree_variants" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_variants" ALTER COLUMN "track_inventory" SET NOT NULL;
ALTER TABLE "spree_variants" ALTER COLUMN "discontinue_on" SET NOT NULL;
ALTER TABLE "spree_prices" ALTER COLUMN "currency" SET NOT NULL;
ALTER TABLE "spree_prices" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_tax_categories" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_tax_categories" ALTER COLUMN "is_default" SET NOT NULL;
ALTER TABLE "spree_return_authorization_reasons" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "spree_addresses" ALTER COLUMN "firstname" SET NOT NULL;
ALTER TABLE "spree_addresses" ALTER COLUMN "lastname" SET NOT NULL;
ALTER TABLE "spree_addresses" ALTER COLUMN "country_id" SET NOT NULL;
ALTER TABLE "spree_addresses" ALTER COLUMN "state_id" SET NOT NULL;
ALTER TABLE "spree_addresses" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_addresses" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_stock_movements" ALTER COLUMN "stock_item_id" SET NOT NULL;
ALTER TABLE "spree_stock_movements" ALTER COLUMN "originator_id" SET NOT NULL;
ALTER TABLE "spree_stock_movements" ALTER COLUMN "originator_type" SET NOT NULL;
ALTER TABLE "spree_adjustments" ALTER COLUMN "adjustable_id" SET NOT NULL;
ALTER TABLE "spree_adjustments" ALTER COLUMN "adjustable_type" SET NOT NULL;
ALTER TABLE "spree_adjustments" ALTER COLUMN "eligible" SET NOT NULL;
ALTER TABLE "spree_adjustments" ALTER COLUMN "source_id" SET NOT NULL;
ALTER TABLE "spree_adjustments" ALTER COLUMN "source_type" SET NOT NULL;
ALTER TABLE "spree_return_authorizations" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "spree_return_authorizations" ALTER COLUMN "return_authorization_reason_id" SET NOT NULL;
ALTER TABLE "spree_return_authorizations" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_return_authorizations" ALTER COLUMN "stock_location_id" SET NOT NULL;
ALTER TABLE "spree_refund_reasons" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "spree_log_entries" ALTER COLUMN "source_id" SET NOT NULL;
ALTER TABLE "spree_log_entries" ALTER COLUMN "source_type" SET NOT NULL;
ALTER TABLE "spree_return_items" ALTER COLUMN "customer_return_id" SET NOT NULL;
ALTER TABLE "spree_return_items" ALTER COLUMN "return_authorization_id" SET NOT NULL;
ALTER TABLE "spree_return_items" ALTER COLUMN "inventory_unit_id" SET NOT NULL;
ALTER TABLE "spree_return_items" ALTER COLUMN "reimbursement_id" SET NOT NULL;
ALTER TABLE "spree_return_items" ALTER COLUMN "exchange_variant_id" SET NOT NULL;
ALTER TABLE "spree_return_items" ALTER COLUMN "preferred_reimbursement_type_id" SET NOT NULL;
ALTER TABLE "spree_return_items" ALTER COLUMN "override_reimbursement_type_id" SET NOT NULL;
ALTER TABLE "spree_product_properties" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "spree_product_properties" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_product_properties" ALTER COLUMN "property_id" SET NOT NULL;
ALTER TABLE "spree_shipping_categories" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "spree_stock_locations" ALTER COLUMN "active" SET NOT NULL;
ALTER TABLE "spree_stock_locations" ALTER COLUMN "backorderable_default" SET NOT NULL;
ALTER TABLE "spree_stock_locations" ALTER COLUMN "country_id" SET NOT NULL;
ALTER TABLE "spree_stock_locations" ALTER COLUMN "propagate_all_variants" SET NOT NULL;
ALTER TABLE "spree_stock_locations" ALTER COLUMN "state_id" SET NOT NULL;
ALTER TABLE "spree_products_taxons" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "spree_products_taxons" ALTER COLUMN "taxon_id" SET NOT NULL;
ALTER TABLE "spree_products_taxons" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_refunds" ALTER COLUMN "refund_reason_id" SET NOT NULL;
ALTER TABLE "spree_refunds" ALTER COLUMN "payment_id" SET NOT NULL;
ALTER TABLE "spree_refunds" ALTER COLUMN "reimbursement_id" SET NOT NULL;
ALTER TABLE "spree_option_values" ALTER COLUMN "option_type_id" SET NOT NULL;
ALTER TABLE "spree_option_values" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_option_values" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "completed_at" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "approver_id" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "bill_address_id" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "confirmation_delivered" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "considered_risky" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "ship_address_id" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "created_by_id" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "token" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "canceler_id" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "store_id" SET NOT NULL;
ALTER TABLE "spree_shipping_method_zones" ALTER COLUMN "zone_id" SET NOT NULL;
ALTER TABLE "spree_shipping_method_zones" ALTER COLUMN "shipping_method_id" SET NOT NULL;
ALTER TABLE "spree_shipping_methods" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_shipping_methods" ALTER COLUMN "tax_category_id" SET NOT NULL;
ALTER TABLE "spree_promotion_rule_taxons" ADD CONSTRAINT "wetune_u_2c2e3281c66fe40a" UNIQUE ("taxon_id", "promotion_rule_id");
ALTER TABLE "spree_products_taxons" ADD CONSTRAINT "wetune_fk_cd6dc207d6fd6dcc" FOREIGN KEY ("taxon_id") REFERENCES "spree_taxons" ("id");
ALTER TABLE "spree_promotion_rules" ADD CONSTRAINT "wetune_fk_cafe11cedb0dd56e" FOREIGN KEY ("promotion_id") REFERENCES "spree_promotions" ("id");
ALTER TABLE "spree_stock_items" ADD CONSTRAINT "wetune_fk_00129d8867d1382f" FOREIGN KEY ("stock_location_id") REFERENCES "spree_stock_locations" ("id");
ALTER TABLE "spree_shipping_rates" ADD CONSTRAINT "wetune_fk_a8a24e2fda82da02" FOREIGN KEY ("shipping_method_id") REFERENCES "spree_shipping_methods" ("id");
ALTER TABLE "spree_product_option_types" ADD CONSTRAINT "wetune_fk_6a0e0c1eebd3c655" FOREIGN KEY ("option_type_id") REFERENCES "spree_option_types" ("id");
ALTER TABLE "spree_adjustments" ADD CONSTRAINT "wetune_fk_f461cffaba312ec4" FOREIGN KEY ("adjustable_id") REFERENCES "spree_line_items" ("id");
ALTER TABLE "spree_option_type_prototypes" ADD CONSTRAINT "wetune_fk_34435b199bb5f9d1" FOREIGN KEY ("option_type_id") REFERENCES "spree_option_types" ("id");
ALTER TABLE "friendly_id_slugs" ADD CONSTRAINT "wetune_fk_19f5eef41e2e4785" FOREIGN KEY ("sluggable_id") REFERENCES "spree_products" ("id");
ALTER TABLE "spree_inventory_units" ADD CONSTRAINT "wetune_fk_9c503f0cfee84187" FOREIGN KEY ("original_return_item_id") REFERENCES "spree_return_items" ("id");
ALTER TABLE "spree_inventory_units" ADD CONSTRAINT "wetune_fk_6ed61e7dc43acddc" FOREIGN KEY ("order_id") REFERENCES "spree_orders" ("id");
ALTER TABLE "spree_shipping_method_categories" ADD CONSTRAINT "wetune_fk_de657e99e8bab772" FOREIGN KEY ("shipping_category_id") REFERENCES "spree_shipping_categories" ("id");
ALTER TABLE "spree_inventory_units" ADD CONSTRAINT "wetune_fk_9cf2ccd4f095c21c" FOREIGN KEY ("variant_id") REFERENCES "spree_variants" ("id");
ALTER TABLE "spree_taxons" ADD CONSTRAINT "wetune_fk_4de7491427208916" FOREIGN KEY ("taxonomy_id") REFERENCES "spree_taxonomies" ("id");
ALTER TABLE "spree_products" ADD CONSTRAINT "wetune_fk_b727e71c92fce754" FOREIGN KEY ("shipping_category_id") REFERENCES "spree_shipping_categories" ("id");
ALTER TABLE "spree_store_credits" ADD CONSTRAINT "wetune_fk_5975940aa5faa4fa" FOREIGN KEY ("type_id") REFERENCES "spree_store_credit_types" ("id");
ALTER TABLE "spree_adjustments" ADD CONSTRAINT "wetune_fk_52fbf85d339c9dc3" FOREIGN KEY ("order_id") REFERENCES "spree_orders" ("id");
ALTER TABLE "spree_line_items" ADD CONSTRAINT "wetune_fk_509ec73b8d6fb218" FOREIGN KEY ("variant_id") REFERENCES "spree_variants" ("id");
ALTER TABLE "spree_product_properties" ADD CONSTRAINT "wetune_fk_3c8a0157240eba31" FOREIGN KEY ("property_id") REFERENCES "spree_properties" ("id");
ALTER TABLE "spree_prices" ADD CONSTRAINT "wetune_fk_560119b6b3df8fa5" FOREIGN KEY ("variant_id") REFERENCES "spree_variants" ("id");
ALTER TABLE "spree_stock_items" ADD CONSTRAINT "wetune_fk_3453027b7850fef6" FOREIGN KEY ("variant_id") REFERENCES "spree_variants" ("id");
ALTER TABLE "spree_users" ADD CONSTRAINT "wetune_fk_e22a4d38263fc986" FOREIGN KEY ("bill_address_id") REFERENCES "spree_addresses" ("id");
ALTER TABLE "spree_orders" ADD CONSTRAINT "wetune_fk_bf13d83d1f7cf785" FOREIGN KEY ("user_id") REFERENCES "spree_users" ("id");
ALTER TABLE "spree_refunds" ADD CONSTRAINT "wetune_fk_618c3ecfa46d38a1" FOREIGN KEY ("payment_id") REFERENCES "spree_payments" ("id");
ALTER TABLE "spree_option_value_variants" ADD CONSTRAINT "wetune_fk_eff6ce84e642ef92" FOREIGN KEY ("option_value_id") REFERENCES "spree_option_values" ("id");
ALTER TABLE "spree_product_properties" ADD CONSTRAINT "wetune_fk_bf9a142a239a12f1" FOREIGN KEY ("product_id") REFERENCES "spree_products" ("id");
ALTER TABLE "spree_line_items" ADD CONSTRAINT "wetune_fk_ea36069359fcab21" FOREIGN KEY ("order_id") REFERENCES "spree_orders" ("id");
ALTER TABLE "spree_users" ADD CONSTRAINT "wetune_fk_ae16175315df591a" FOREIGN KEY ("ship_address_id") REFERENCES "spree_addresses" ("id");
ALTER TABLE "spree_return_items" ADD CONSTRAINT "wetune_fk_0acce125c7a34ee3" FOREIGN KEY ("return_authorization_id") REFERENCES "spree_return_authorizations" ("id");
ALTER TABLE "spree_products_taxons" ADD CONSTRAINT "wetune_fk_54b4fb3bf4972d71" FOREIGN KEY ("product_id") REFERENCES "spree_products" ("id");
ALTER TABLE "spree_shipping_method_zones" ADD CONSTRAINT "wetune_fk_15a59c5b0b9c933c" FOREIGN KEY ("shipping_method_id") REFERENCES "spree_shipping_methods" ("id");
ALTER TABLE "spree_product_promotion_rules" ADD CONSTRAINT "wetune_fk_f5b319349e99ff40" FOREIGN KEY ("product_id") REFERENCES "spree_products" ("id");
ALTER TABLE "spree_option_values" ADD CONSTRAINT "wetune_fk_1b638676382e592d" FOREIGN KEY ("option_type_id") REFERENCES "spree_option_types" ("id");
ALTER TABLE "spree_zone_members" ADD CONSTRAINT "wetune_fk_b975a41748d04512" FOREIGN KEY ("zone_id") REFERENCES "spree_zones" ("id");
ALTER TABLE "spree_order_promotions" ADD CONSTRAINT "wetune_fk_143eecc480f33b28" FOREIGN KEY ("promotion_id") REFERENCES "spree_promotions" ("id");
ALTER TABLE "spree_option_value_variants" ADD CONSTRAINT "wetune_fk_81531013ce457041" FOREIGN KEY ("variant_id") REFERENCES "spree_variants" ("id");
ALTER TABLE "spree_promotion_actions" ADD CONSTRAINT "wetune_fk_559451e270ff52cb" FOREIGN KEY ("promotion_id") REFERENCES "spree_promotions" ("id");
ALTER TABLE "spree_variants" ADD CONSTRAINT "wetune_fk_506092e02122e48b" FOREIGN KEY ("product_id") REFERENCES "spree_products" ("id");
ALTER TABLE "spree_promotion_rule_taxons" ADD CONSTRAINT "wetune_fk_4ba38db7e08b0436" FOREIGN KEY ("taxon_id") REFERENCES "spree_taxons" ("id");
ALTER TABLE "spree_order_promotions" ADD CONSTRAINT "wetune_fk_3477adbc25312c9f" FOREIGN KEY ("order_id") REFERENCES "spree_orders" ("id");
ALTER TABLE "spree_role_users" ADD CONSTRAINT "wetune_fk_3413e78caa252a18" FOREIGN KEY ("role_id") REFERENCES "spree_roles" ("id");
ALTER TABLE "spree_zone_members" ADD CONSTRAINT "wetune_fk_7196be35e15e08b8" FOREIGN KEY ("zoneable_id") REFERENCES "spree_countries" ("id");
ALTER TABLE "spree_shipments" ADD CONSTRAINT "wetune_fk_d8185014630f7c58" FOREIGN KEY ("order_id") REFERENCES "spree_orders" ("id");
ALTER TABLE "spree_shipping_method_zones" ADD CONSTRAINT "wetune_fk_3f65ac4e624e0506" FOREIGN KEY ("zone_id") REFERENCES "spree_zones" ("id");
ALTER TABLE "spree_stock_movements" ADD CONSTRAINT "wetune_fk_9d86dcd0b603fc51" FOREIGN KEY ("stock_item_id") REFERENCES "spree_stock_items" ("id");
ALTER TABLE "spree_property_prototypes" ADD CONSTRAINT "wetune_fk_26d9910df4dbcaa9" FOREIGN KEY ("property_id") REFERENCES "spree_properties" ("id");
ALTER TABLE "spree_product_promotion_rules" ADD CONSTRAINT "wetune_fk_884ecfcedb3f5c8b" FOREIGN KEY ("promotion_rule_id") REFERENCES "spree_promotion_rules" ("id");
ALTER TABLE "spree_inventory_units" ADD CONSTRAINT "wetune_fk_c54e02918e05e129" FOREIGN KEY ("shipment_id") REFERENCES "spree_shipments" ("id");
ALTER TABLE "spree_prototype_taxons" ADD CONSTRAINT "wetune_fk_555358738d80fc06" FOREIGN KEY ("taxon_id") REFERENCES "spree_taxons" ("id");
