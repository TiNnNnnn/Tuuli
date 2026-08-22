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
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
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
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_adjustment_reasons" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "code" VARCHAR(255),
  "active" SMALLINT,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_adjustments" (
  "id" INTEGER NOT NULL,
  "source_type" VARCHAR(255),
  "source_id" INTEGER,
  "adjustable_type" VARCHAR(255),
  "adjustable_id" INTEGER NOT NULL,
  "amount" DECIMAL(10, 2),
  "label" VARCHAR(255),
  "eligible" SMALLINT,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "order_id" INTEGER NOT NULL,
  "included" SMALLINT,
  "promotion_code_id" INTEGER,
  "adjustment_reason_id" INTEGER,
  "finalized" SMALLINT NOT NULL,
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
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_calculators" (
  "id" INTEGER NOT NULL,
  "type" VARCHAR(255),
  "calculable_type" VARCHAR(255),
  "calculable_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "preferences" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_cartons" (
  "id" INTEGER NOT NULL,
  "number" VARCHAR(255),
  "external_number" VARCHAR(255),
  "stock_location_id" INTEGER,
  "address_id" INTEGER,
  "shipping_method_id" INTEGER,
  "tracking" VARCHAR(255),
  "shipped_at" TIMESTAMP,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "imported_from_shipment_id" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("imported_from_shipment_id"),
  UNIQUE ("number")
);

CREATE TABLE "spree_countries" (
  "id" INTEGER NOT NULL,
  "iso_name" VARCHAR(255),
  "iso" VARCHAR(255),
  "iso3" VARCHAR(255),
  "name" VARCHAR(255),
  "numcode" INTEGER,
  "states_required" SMALLINT,
  "updated_at" TIMESTAMP(6),
  "created_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_credit_cards" (
  "id" INTEGER NOT NULL,
  "month" VARCHAR(255),
  "year" VARCHAR(255),
  "cc_type" VARCHAR(255),
  "last_digits" VARCHAR(255),
  "gateway_customer_profile_id" VARCHAR(255),
  "gateway_payment_profile_id" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "name" VARCHAR(255),
  "user_id" INTEGER,
  "payment_method_id" INTEGER,
  "default" SMALLINT NOT NULL,
  "address_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_customer_returns" (
  "id" INTEGER NOT NULL,
  "number" VARCHAR(255),
  "stock_location_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_inventory_units" (
  "id" INTEGER NOT NULL,
  "state" VARCHAR(255),
  "variant_id" INTEGER,
  "shipment_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "pending" SMALLINT,
  "line_item_id" INTEGER,
  "carton_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_line_item_actions" (
  "id" INTEGER NOT NULL,
  "line_item_id" INTEGER NOT NULL,
  "action_id" INTEGER NOT NULL,
  "quantity" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_line_items" (
  "id" INTEGER NOT NULL,
  "variant_id" INTEGER,
  "order_id" INTEGER,
  "quantity" INTEGER NOT NULL,
  "price" DECIMAL(10, 2) NOT NULL,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "cost_price" DECIMAL(10, 2),
  "tax_category_id" INTEGER,
  "adjustment_total" DECIMAL(10, 2),
  "additional_tax_total" DECIMAL(10, 2),
  "promo_total" DECIMAL(10, 2),
  "included_tax_total" DECIMAL(10, 2) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_log_entries" (
  "id" INTEGER NOT NULL,
  "source_type" VARCHAR(255),
  "source_id" INTEGER,
  "details" TEXT,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_option_type_prototypes" (
  "id" INTEGER NOT NULL,
  "prototype_id" INTEGER,
  "option_type_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_option_types" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(100),
  "presentation" VARCHAR(100),
  "position" INTEGER NOT NULL,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_option_values" (
  "id" INTEGER NOT NULL,
  "position" INTEGER,
  "name" VARCHAR(255),
  "presentation" VARCHAR(255),
  "option_type_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_option_values_variants" (
  "id" INTEGER NOT NULL,
  "variant_id" INTEGER,
  "option_value_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_order_mutexes" (
  "id" INTEGER NOT NULL,
  "order_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP(6),
  PRIMARY KEY ("id"),
  UNIQUE ("order_id")
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
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
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
  "guest_token" VARCHAR(255),
  "canceled_at" TIMESTAMP,
  "canceler_id" INTEGER,
  "store_id" INTEGER,
  "approver_name" VARCHAR(255),
  "frontend_viewable" SMALLINT NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_orders_promotions" (
  "id" INTEGER NOT NULL,
  "order_id" INTEGER,
  "promotion_id" INTEGER,
  "promotion_code_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_payment_capture_events" (
  "id" INTEGER NOT NULL,
  "amount" DECIMAL(10, 2),
  "payment_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_payment_methods" (
  "id" INTEGER NOT NULL,
  "type" VARCHAR(255),
  "name" VARCHAR(255),
  "description" TEXT,
  "active" SMALLINT,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "auto_capture" SMALLINT,
  "preferences" TEXT,
  "preference_source" VARCHAR(255),
  "position" INTEGER,
  "available_to_users" SMALLINT,
  "available_to_admin" SMALLINT,
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
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
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
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id"),
  UNIQUE ("key")
);

CREATE TABLE "spree_prices" (
  "id" INTEGER NOT NULL,
  "variant_id" INTEGER NOT NULL,
  "amount" DECIMAL(10, 2),
  "currency" VARCHAR(255),
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "country_iso" VARCHAR(2),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_product_option_types" (
  "id" INTEGER NOT NULL,
  "position" INTEGER,
  "product_id" INTEGER,
  "option_type_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_product_promotion_rules" (
  "id" INTEGER NOT NULL,
  "product_id" INTEGER,
  "promotion_rule_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_product_properties" (
  "id" INTEGER NOT NULL,
  "value" VARCHAR(255),
  "product_id" INTEGER,
  "property_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "position" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_products" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "description" TEXT,
  "available_on" TIMESTAMP,
  "deleted_at" TIMESTAMP,
  "slug" VARCHAR(255),
  "meta_description" TEXT,
  "meta_keywords" VARCHAR(255),
  "tax_category_id" INTEGER,
  "shipping_category_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "promotionable" SMALLINT,
  "meta_title" VARCHAR(255),
  PRIMARY KEY ("id"),
  UNIQUE ("slug")
);

CREATE TABLE "spree_products_taxons" (
  "id" INTEGER NOT NULL,
  "product_id" INTEGER,
  "taxon_id" INTEGER,
  "position" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_action_line_items" (
  "id" INTEGER NOT NULL,
  "promotion_action_id" INTEGER,
  "variant_id" INTEGER,
  "quantity" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_actions" (
  "id" INTEGER NOT NULL,
  "promotion_id" INTEGER,
  "position" INTEGER,
  "type" VARCHAR(255),
  "deleted_at" TIMESTAMP,
  "preferences" TEXT,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_categories" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "code" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_code_batches" (
  "id" INTEGER NOT NULL,
  "promotion_id" INTEGER NOT NULL,
  "base_code" VARCHAR(255) NOT NULL,
  "number_of_codes" INTEGER NOT NULL,
  "email" VARCHAR(255),
  "error" VARCHAR(255),
  "state" VARCHAR(255),
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "join_characters" VARCHAR(255) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_codes" (
  "id" INTEGER NOT NULL,
  "promotion_id" INTEGER NOT NULL,
  "value" VARCHAR(255) NOT NULL,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "promotion_code_batch_id" INTEGER,
  PRIMARY KEY ("id"),
  UNIQUE ("value")
);

CREATE TABLE "spree_promotion_rule_taxons" (
  "id" INTEGER NOT NULL,
  "taxon_id" INTEGER,
  "promotion_rule_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_rules" (
  "id" INTEGER NOT NULL,
  "promotion_id" INTEGER,
  "product_group_id" INTEGER,
  "type" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "code" VARCHAR(255),
  "preferences" TEXT,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_rules_stores" (
  "id" BIGINT NOT NULL,
  "store_id" BIGINT NOT NULL,
  "promotion_rule_id" BIGINT NOT NULL,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_promotion_rules_users" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "promotion_rule_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
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
  "advertise" SMALLINT,
  "path" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "promotion_category_id" INTEGER,
  "per_code_usage_limit" INTEGER,
  "apply_automatically" SMALLINT,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_properties" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "presentation" VARCHAR(255) NOT NULL,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_property_prototypes" (
  "id" INTEGER NOT NULL,
  "prototype_id" INTEGER,
  "property_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_prototype_taxons" (
  "id" INTEGER NOT NULL,
  "taxon_id" INTEGER,
  "prototype_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_prototypes" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_refund_reasons" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "active" SMALLINT,
  "mutable" SMALLINT,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "code" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_refunds" (
  "id" INTEGER NOT NULL,
  "payment_id" INTEGER,
  "amount" DECIMAL(10, 2) NOT NULL,
  "transaction_id" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
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
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_reimbursement_types" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "active" SMALLINT,
  "mutable" SMALLINT,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "type" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_reimbursements" (
  "id" INTEGER NOT NULL,
  "number" VARCHAR(255),
  "reimbursement_status" VARCHAR(255),
  "customer_return_id" INTEGER,
  "order_id" INTEGER,
  "total" DECIMAL(10, 2),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_return_authorizations" (
  "id" INTEGER NOT NULL,
  "number" VARCHAR(255),
  "state" VARCHAR(255),
  "order_id" INTEGER,
  "memo" TEXT,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "stock_location_id" INTEGER,
  "return_reason_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_return_items" (
  "id" INTEGER NOT NULL,
  "return_authorization_id" INTEGER,
  "inventory_unit_id" INTEGER,
  "exchange_variant_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "amount" DECIMAL(12, 4) NOT NULL,
  "included_tax_total" DECIMAL(12, 4) NOT NULL,
  "additional_tax_total" DECIMAL(12, 4) NOT NULL,
  "reception_status" VARCHAR(255),
  "acceptance_status" VARCHAR(255),
  "customer_return_id" INTEGER,
  "reimbursement_id" INTEGER,
  "exchange_inventory_unit_id" INTEGER,
  "acceptance_status_errors" TEXT,
  "preferred_reimbursement_type_id" INTEGER,
  "override_reimbursement_type_id" INTEGER,
  "resellable" SMALLINT NOT NULL,
  "return_reason_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_return_reasons" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "active" SMALLINT,
  "mutable" SMALLINT,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_roles" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id"),
  UNIQUE ("name")
);

CREATE TABLE "spree_roles_users" (
  "id" INTEGER NOT NULL,
  "role_id" INTEGER,
  "user_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "role_id")
);

CREATE TABLE "spree_shipments" (
  "id" INTEGER NOT NULL,
  "tracking" VARCHAR(255),
  "number" VARCHAR(255),
  "cost" DECIMAL(10, 2),
  "shipped_at" TIMESTAMP,
  "order_id" INTEGER,
  "deprecated_address_id" INTEGER,
  "state" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "stock_location_id" INTEGER,
  "adjustment_total" DECIMAL(10, 2),
  "additional_tax_total" DECIMAL(10, 2),
  "promo_total" DECIMAL(10, 2),
  "included_tax_total" DECIMAL(10, 2) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_shipping_categories" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_shipping_method_categories" (
  "id" INTEGER NOT NULL,
  "shipping_method_id" INTEGER NOT NULL,
  "shipping_category_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id"),
  UNIQUE ("shipping_category_id", "shipping_method_id")
);

CREATE TABLE "spree_shipping_method_stock_locations" (
  "id" INTEGER NOT NULL,
  "shipping_method_id" INTEGER,
  "stock_location_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_shipping_method_zones" (
  "id" INTEGER NOT NULL,
  "shipping_method_id" INTEGER,
  "zone_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_shipping_methods" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "tracking_url" VARCHAR(255),
  "admin_name" VARCHAR(255),
  "tax_category_id" INTEGER,
  "code" VARCHAR(255),
  "available_to_all" SMALLINT,
  "carrier" VARCHAR(255),
  "service_level" VARCHAR(255),
  "available_to_users" SMALLINT,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_shipping_rate_taxes" (
  "id" INTEGER NOT NULL,
  "amount" DECIMAL(8, 2) NOT NULL,
  "tax_rate_id" INTEGER,
  "shipping_rate_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_shipping_rates" (
  "id" INTEGER NOT NULL,
  "shipment_id" INTEGER,
  "shipping_method_id" INTEGER,
  "selected" SMALLINT,
  "cost" DECIMAL(8, 2),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
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
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_states" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "abbr" VARCHAR(255),
  "country_id" INTEGER,
  "updated_at" TIMESTAMP(6),
  "created_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_stock_items" (
  "id" INTEGER NOT NULL,
  "stock_location_id" INTEGER,
  "variant_id" INTEGER,
  "count_on_hand" INTEGER NOT NULL,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "backorderable" SMALLINT,
  "deleted_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_stock_locations" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
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
  "position" INTEGER,
  "restock_inventory" SMALLINT NOT NULL,
  "fulfillable" SMALLINT NOT NULL,
  "code" VARCHAR(255),
  "check_stock_on_transfer" SMALLINT,
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

CREATE TABLE "spree_store_credit_categories" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_store_credit_events" (
  "id" INTEGER NOT NULL,
  "store_credit_id" INTEGER NOT NULL,
  "action" VARCHAR(255) NOT NULL,
  "amount" DECIMAL(8, 2),
  "user_total_amount" DECIMAL(8, 2) NOT NULL,
  "authorization_code" VARCHAR(255) NOT NULL,
  "deleted_at" TIMESTAMP,
  "originator_type" VARCHAR(255),
  "originator_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "amount_remaining" DECIMAL(8, 2),
  "store_credit_reason_id" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_store_credit_reasons" (
  "id" BIGINT NOT NULL,
  "name" VARCHAR(255),
  "active" SMALLINT,
  "created_at" TIMESTAMP NOT NULL,
  "updated_at" TIMESTAMP NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_store_credit_types" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "priority" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_store_credits" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "category_id" INTEGER,
  "created_by_id" INTEGER,
  "amount" DECIMAL(8, 2) NOT NULL,
  "amount_used" DECIMAL(8, 2) NOT NULL,
  "amount_authorized" DECIMAL(8, 2) NOT NULL,
  "currency" VARCHAR(255),
  "memo" TEXT,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "type_id" INTEGER,
  "invalidated_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_store_payment_methods" (
  "id" INTEGER NOT NULL,
  "store_id" INTEGER NOT NULL,
  "payment_method_id" INTEGER NOT NULL,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_store_shipping_methods" (
  "id" BIGINT NOT NULL,
  "store_id" BIGINT NOT NULL,
  "shipping_method_id" BIGINT NOT NULL,
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
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "cart_tax_country_iso" VARCHAR(255),
  "available_locales" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_tax_categories" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "description" VARCHAR(255),
  "is_default" SMALLINT,
  "deleted_at" TIMESTAMP,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "tax_code" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_tax_rate_tax_categories" (
  "id" INTEGER NOT NULL,
  "tax_category_id" INTEGER NOT NULL,
  "tax_rate_id" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_tax_rates" (
  "id" INTEGER NOT NULL,
  "amount" DECIMAL(8, 5),
  "zone_id" INTEGER,
  "included_in_price" SMALLINT,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "name" VARCHAR(255),
  "show_rate_in_label" SMALLINT,
  "deleted_at" TIMESTAMP,
  "starts_at" TIMESTAMP,
  "expires_at" TIMESTAMP,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_taxonomies" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255) NOT NULL,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
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
  "icon_file_name" VARCHAR(255),
  "icon_content_type" VARCHAR(255),
  "icon_file_size" INTEGER,
  "icon_updated_at" TIMESTAMP,
  "description" TEXT,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  "meta_title" VARCHAR(255),
  "meta_description" VARCHAR(255),
  "meta_keywords" VARCHAR(255),
  "depth" INTEGER,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_unit_cancels" (
  "id" INTEGER NOT NULL,
  "inventory_unit_id" INTEGER NOT NULL,
  "reason" VARCHAR(255),
  "created_by" VARCHAR(255),
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_user_addresses" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "address_id" INTEGER NOT NULL,
  "default" SMALLINT,
  "archived" SMALLINT,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "address_id")
);

CREATE TABLE "spree_user_stock_locations" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER,
  "stock_location_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_users" (
  "id" INTEGER NOT NULL,
  "crypted_password" VARCHAR(128),
  "salt" VARCHAR(128),
  "email" VARCHAR(255),
  "remember_token" VARCHAR(255),
  "remember_token_expires_at" VARCHAR(255),
  "persistence_token" VARCHAR(255),
  "single_access_token" VARCHAR(255),
  "perishable_token" VARCHAR(255),
  "login_count" INTEGER NOT NULL,
  "failed_login_count" INTEGER NOT NULL,
  "last_request_at" TIMESTAMP,
  "current_login_at" TIMESTAMP,
  "last_login_at" TIMESTAMP,
  "current_login_ip" VARCHAR(255),
  "last_login_ip" VARCHAR(255),
  "login" VARCHAR(255),
  "ship_address_id" INTEGER,
  "bill_address_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  "openid_identifier" VARCHAR(255),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_variant_property_rule_conditions" (
  "id" INTEGER NOT NULL,
  "option_value_id" INTEGER,
  "variant_property_rule_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_variant_property_rule_values" (
  "id" INTEGER NOT NULL,
  "value" TEXT,
  "position" INTEGER,
  "property_id" INTEGER,
  "variant_property_rule_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_variant_property_rules" (
  "id" INTEGER NOT NULL,
  "product_id" INTEGER,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
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
  "is_master" SMALLINT,
  "product_id" INTEGER,
  "cost_price" DECIMAL(10, 2),
  "position" INTEGER,
  "cost_currency" VARCHAR(255),
  "track_inventory" SMALLINT,
  "tax_category_id" INTEGER,
  "updated_at" TIMESTAMP(6),
  "created_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_wallet_payment_sources" (
  "id" INTEGER NOT NULL,
  "user_id" INTEGER NOT NULL,
  "payment_source_type" VARCHAR(255) NOT NULL,
  "payment_source_id" INTEGER NOT NULL,
  "default" SMALLINT NOT NULL,
  "created_at" TIMESTAMP(6) NOT NULL,
  "updated_at" TIMESTAMP(6) NOT NULL,
  PRIMARY KEY ("id"),
  UNIQUE ("user_id", "payment_source_id", "payment_source_type")
);

CREATE TABLE "spree_zone_members" (
  "id" INTEGER NOT NULL,
  "zoneable_type" VARCHAR(255),
  "zoneable_id" INTEGER,
  "zone_id" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

CREATE TABLE "spree_zones" (
  "id" INTEGER NOT NULL,
  "name" VARCHAR(255),
  "description" VARCHAR(255),
  "zone_members_count" INTEGER,
  "created_at" TIMESTAMP(6),
  "updated_at" TIMESTAMP(6),
  PRIMARY KEY ("id")
);

ALTER TABLE "active_storage_attachments" ADD FOREIGN KEY ("blob_id") REFERENCES "active_storage_blobs" ("id");

ALTER TABLE "spree_promotion_code_batches" ADD FOREIGN KEY ("promotion_id") REFERENCES "spree_promotions" ("id");

ALTER TABLE "spree_promotion_codes" ADD FOREIGN KEY ("promotion_code_batch_id") REFERENCES "spree_promotion_code_batches" ("id");

ALTER TABLE "spree_tax_rate_tax_categories" ADD FOREIGN KEY ("tax_rate_id") REFERENCES "spree_tax_rates" ("id");

ALTER TABLE "spree_tax_rate_tax_categories" ADD FOREIGN KEY ("tax_category_id") REFERENCES "spree_tax_categories" ("id");

ALTER TABLE "spree_wallet_payment_sources" ADD FOREIGN KEY ("user_id") REFERENCES "spree_users" ("id");

-- WeTune schema patches
ALTER TABLE "spree_option_values_variants" ALTER COLUMN "option_value_id" SET NOT NULL;
ALTER TABLE "spree_tax_rates" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_tax_rates" ALTER COLUMN "zone_id" SET NOT NULL;
ALTER TABLE "spree_payments" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "spree_payments" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_payments" ALTER COLUMN "payment_method_id" SET NOT NULL;
ALTER TABLE "spree_payments" ALTER COLUMN "source_id" SET NOT NULL;
ALTER TABLE "spree_payments" ALTER COLUMN "source_type" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "permalink" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "taxonomy_id" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "lft" SET NOT NULL;
ALTER TABLE "spree_taxons" ALTER COLUMN "rgt" SET NOT NULL;
ALTER TABLE "spree_assets" ALTER COLUMN "viewable_id" SET NOT NULL;
ALTER TABLE "spree_assets" ALTER COLUMN "viewable_type" SET NOT NULL;
ALTER TABLE "spree_assets" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_shipping_rates" ALTER COLUMN "shipment_id" SET NOT NULL;
ALTER TABLE "spree_shipping_rates" ALTER COLUMN "shipping_method_id" SET NOT NULL;
ALTER TABLE "spree_payment_capture_events" ALTER COLUMN "payment_id" SET NOT NULL;
ALTER TABLE "spree_store_credit_events" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_orders_promotions" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_orders_promotions" ALTER COLUMN "promotion_id" SET NOT NULL;
ALTER TABLE "spree_orders_promotions" ALTER COLUMN "promotion_code_id" SET NOT NULL;
ALTER TABLE "spree_store_credits" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_store_credits" ALTER COLUMN "type_id" SET NOT NULL;
ALTER TABLE "spree_store_credits" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_shipments" ALTER COLUMN "deprecated_address_id" SET NOT NULL;
ALTER TABLE "spree_shipments" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "spree_shipments" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_shipments" ALTER COLUMN "stock_location_id" SET NOT NULL;
ALTER TABLE "spree_store_credit_types" ALTER COLUMN "priority" SET NOT NULL;
ALTER TABLE "spree_reimbursements" ALTER COLUMN "customer_return_id" SET NOT NULL;
ALTER TABLE "spree_reimbursements" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_stock_items" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_stock_items" ALTER COLUMN "variant_id" SET NOT NULL;
ALTER TABLE "spree_stock_items" ALTER COLUMN "stock_location_id" SET NOT NULL;
ALTER TABLE "spree_promotion_actions" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_promotion_actions" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_promotion_actions" ALTER COLUMN "promotion_id" SET NOT NULL;
ALTER TABLE "spree_countries" ALTER COLUMN "iso" SET NOT NULL;
ALTER TABLE "spree_cartons" ALTER COLUMN "imported_from_shipment_id" SET NOT NULL;
ALTER TABLE "spree_cartons" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "spree_cartons" ALTER COLUMN "external_number" SET NOT NULL;
ALTER TABLE "spree_cartons" ALTER COLUMN "stock_location_id" SET NOT NULL;
ALTER TABLE "spree_reimbursement_types" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_shipping_rate_taxes" ALTER COLUMN "shipping_rate_id" SET NOT NULL;
ALTER TABLE "spree_shipping_rate_taxes" ALTER COLUMN "tax_rate_id" SET NOT NULL;
ALTER TABLE "spree_state_changes" ALTER COLUMN "stateful_id" SET NOT NULL;
ALTER TABLE "spree_state_changes" ALTER COLUMN "stateful_type" SET NOT NULL;
ALTER TABLE "spree_state_changes" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_product_promotion_rules" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "spree_product_promotion_rules" ALTER COLUMN "promotion_rule_id" SET NOT NULL;
ALTER TABLE "spree_preferences" ALTER COLUMN "key" SET NOT NULL;
ALTER TABLE "spree_promotion_rules_users" ALTER COLUMN "promotion_rule_id" SET NOT NULL;
ALTER TABLE "spree_promotion_rules_users" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "friendly_id_slugs" ALTER COLUMN "scope" SET NOT NULL;
ALTER TABLE "friendly_id_slugs" ALTER COLUMN "sluggable_type" SET NOT NULL;
ALTER TABLE "spree_promotion_codes" ALTER COLUMN "promotion_code_batch_id" SET NOT NULL;
ALTER TABLE "spree_product_option_types" ALTER COLUMN "option_type_id" SET NOT NULL;
ALTER TABLE "spree_product_option_types" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_product_option_types" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "spree_variant_property_rule_values" ALTER COLUMN "property_id" SET NOT NULL;
ALTER TABLE "spree_variant_property_rule_values" ALTER COLUMN "variant_property_rule_id" SET NOT NULL;
ALTER TABLE "spree_inventory_units" ALTER COLUMN "carton_id" SET NOT NULL;
ALTER TABLE "spree_inventory_units" ALTER COLUMN "line_item_id" SET NOT NULL;
ALTER TABLE "spree_inventory_units" ALTER COLUMN "shipment_id" SET NOT NULL;
ALTER TABLE "spree_inventory_units" ALTER COLUMN "variant_id" SET NOT NULL;
ALTER TABLE "spree_payment_methods" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_roles_users" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_roles_users" ALTER COLUMN "role_id" SET NOT NULL;
ALTER TABLE "spree_variants" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_variants" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "spree_variants" ALTER COLUMN "tax_category_id" SET NOT NULL;
ALTER TABLE "spree_variants" ALTER COLUMN "track_inventory" SET NOT NULL;
ALTER TABLE "spree_addresses" ALTER COLUMN "country_id" SET NOT NULL;
ALTER TABLE "spree_addresses" ALTER COLUMN "firstname" SET NOT NULL;
ALTER TABLE "spree_addresses" ALTER COLUMN "lastname" SET NOT NULL;
ALTER TABLE "spree_addresses" ALTER COLUMN "state_id" SET NOT NULL;
ALTER TABLE "spree_adjustment_reasons" ALTER COLUMN "active" SET NOT NULL;
ALTER TABLE "spree_adjustment_reasons" ALTER COLUMN "code" SET NOT NULL;
ALTER TABLE "spree_stock_movements" ALTER COLUMN "stock_item_id" SET NOT NULL;
ALTER TABLE "spree_adjustments" ALTER COLUMN "adjustable_type" SET NOT NULL;
ALTER TABLE "spree_adjustments" ALTER COLUMN "eligible" SET NOT NULL;
ALTER TABLE "spree_adjustments" ALTER COLUMN "promotion_code_id" SET NOT NULL;
ALTER TABLE "spree_adjustments" ALTER COLUMN "source_id" SET NOT NULL;
ALTER TABLE "spree_adjustments" ALTER COLUMN "source_type" SET NOT NULL;
ALTER TABLE "spree_return_authorizations" ALTER COLUMN "return_reason_id" SET NOT NULL;
ALTER TABLE "spree_return_items" ALTER COLUMN "customer_return_id" SET NOT NULL;
ALTER TABLE "spree_return_items" ALTER COLUMN "exchange_inventory_unit_id" SET NOT NULL;
ALTER TABLE "spree_stock_locations" ALTER COLUMN "country_id" SET NOT NULL;
ALTER TABLE "spree_stock_locations" ALTER COLUMN "state_id" SET NOT NULL;
ALTER TABLE "spree_refunds" ALTER COLUMN "payment_id" SET NOT NULL;
ALTER TABLE "spree_refunds" ALTER COLUMN "refund_reason_id" SET NOT NULL;
ALTER TABLE "spree_refunds" ALTER COLUMN "reimbursement_id" SET NOT NULL;
ALTER TABLE "spree_user_stock_locations" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_roles" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "spree_shipping_method_stock_locations" ALTER COLUMN "shipping_method_id" SET NOT NULL;
ALTER TABLE "spree_shipping_method_stock_locations" ALTER COLUMN "stock_location_id" SET NOT NULL;
ALTER TABLE "spree_zone_members" ALTER COLUMN "zone_id" SET NOT NULL;
ALTER TABLE "spree_zone_members" ALTER COLUMN "zoneable_id" SET NOT NULL;
ALTER TABLE "spree_zone_members" ALTER COLUMN "zoneable_type" SET NOT NULL;
ALTER TABLE "spree_taxonomies" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_prototype_taxons" ALTER COLUMN "prototype_id" SET NOT NULL;
ALTER TABLE "spree_prototype_taxons" ALTER COLUMN "taxon_id" SET NOT NULL;
ALTER TABLE "spree_products" ALTER COLUMN "slug" SET NOT NULL;
ALTER TABLE "spree_products" ALTER COLUMN "available_on" SET NOT NULL;
ALTER TABLE "spree_products" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "spree_variant_property_rules" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "spree_promotion_action_line_items" ALTER COLUMN "promotion_action_id" SET NOT NULL;
ALTER TABLE "spree_promotion_action_line_items" ALTER COLUMN "variant_id" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "advertise" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "apply_automatically" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "expires_at" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "promotion_category_id" SET NOT NULL;
ALTER TABLE "spree_promotions" ALTER COLUMN "starts_at" SET NOT NULL;
ALTER TABLE "spree_credit_cards" ALTER COLUMN "payment_method_id" SET NOT NULL;
ALTER TABLE "spree_credit_cards" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_line_items" ALTER COLUMN "order_id" SET NOT NULL;
ALTER TABLE "spree_line_items" ALTER COLUMN "variant_id" SET NOT NULL;
ALTER TABLE "spree_stores" ALTER COLUMN "code" SET NOT NULL;
ALTER TABLE "spree_calculators" ALTER COLUMN "calculable_id" SET NOT NULL;
ALTER TABLE "spree_calculators" ALTER COLUMN "calculable_type" SET NOT NULL;
ALTER TABLE "spree_calculators" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "spree_promotion_rule_taxons" ALTER COLUMN "taxon_id" SET NOT NULL;
ALTER TABLE "spree_promotion_rule_taxons" ALTER COLUMN "promotion_rule_id" SET NOT NULL;
ALTER TABLE "spree_variant_property_rule_conditions" ALTER COLUMN "variant_property_rule_id" SET NOT NULL;
ALTER TABLE "spree_variant_property_rule_conditions" ALTER COLUMN "option_value_id" SET NOT NULL;
ALTER TABLE "spree_promotion_rules" ALTER COLUMN "product_group_id" SET NOT NULL;
ALTER TABLE "spree_promotion_rules" ALTER COLUMN "promotion_id" SET NOT NULL;
ALTER TABLE "spree_states" ALTER COLUMN "country_id" SET NOT NULL;
ALTER TABLE "spree_option_values_variants" ALTER COLUMN "variant_id" SET NOT NULL;
ALTER TABLE "spree_prices" ALTER COLUMN "country_iso" SET NOT NULL;
ALTER TABLE "spree_prices" ALTER COLUMN "currency" SET NOT NULL;
ALTER TABLE "spree_log_entries" ALTER COLUMN "source_id" SET NOT NULL;
ALTER TABLE "spree_log_entries" ALTER COLUMN "source_type" SET NOT NULL;
ALTER TABLE "spree_product_properties" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_product_properties" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "spree_product_properties" ALTER COLUMN "property_id" SET NOT NULL;
ALTER TABLE "spree_products_taxons" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_products_taxons" ALTER COLUMN "product_id" SET NOT NULL;
ALTER TABLE "spree_products_taxons" ALTER COLUMN "taxon_id" SET NOT NULL;
ALTER TABLE "spree_option_values" ALTER COLUMN "option_type_id" SET NOT NULL;
ALTER TABLE "spree_option_values" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "approver_id" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "bill_address_id" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "completed_at" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "guest_token" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "ship_address_id" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "created_by_id" SET NOT NULL;
ALTER TABLE "spree_orders" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "spree_shipping_methods" ALTER COLUMN "tax_category_id" SET NOT NULL;
ALTER TABLE "spree_promotion_rule_taxons" ADD CONSTRAINT "wetune_u_6f261c8da136e6a8" UNIQUE ("taxon_id", "promotion_rule_id");
ALTER TABLE "spree_products_taxons" ADD CONSTRAINT "wetune_u_21d100e63d8eec6c" UNIQUE ("product_id", "taxon_id");
ALTER TABLE "spree_option_values_variants" ADD CONSTRAINT "wetune_u_6a75008589a670da" UNIQUE ("variant_id", "option_value_id");
ALTER TABLE "spree_roles_users" ADD CONSTRAINT "wetune_u_c367b170de1239e7" UNIQUE ("user_id", "role_id");
ALTER TABLE "spree_adjustments" ADD CONSTRAINT "wetune_u_729ec04592ec326a" UNIQUE ("source_id", "source_type");
ALTER TABLE "spree_states" ADD CONSTRAINT "wetune_fk_e81a0f965e0f6ca6" FOREIGN KEY ("country_id") REFERENCES "spree_countries" ("id");
ALTER TABLE "spree_product_option_types" ADD CONSTRAINT "wetune_fk_aa5a4a26af34db34" FOREIGN KEY ("product_id") REFERENCES "spree_products" ("id");
ALTER TABLE "spree_stock_items" ADD CONSTRAINT "wetune_fk_809c5854aac4d301" FOREIGN KEY ("stock_location_id") REFERENCES "spree_stock_locations" ("id");
ALTER TABLE "spree_tax_rates" ADD CONSTRAINT "wetune_fk_67378fe547192cbb" FOREIGN KEY ("zone_id") REFERENCES "spree_zones" ("id");
ALTER TABLE "spree_assets" ADD CONSTRAINT "wetune_fk_c39f99ae2dabbb6e" FOREIGN KEY ("viewable_id") REFERENCES "spree_variants" ("id");
ALTER TABLE "spree_store_credits" ADD CONSTRAINT "wetune_fk_b965b4c089ca2044" FOREIGN KEY ("type_id") REFERENCES "spree_store_credit_types" ("id");
ALTER TABLE "spree_stock_items" ADD CONSTRAINT "wetune_fk_fae0541c3cde29c2" FOREIGN KEY ("variant_id") REFERENCES "spree_variants" ("id");
ALTER TABLE "spree_shipping_method_stock_locations" ADD CONSTRAINT "wetune_fk_db4ab27ef687789e" FOREIGN KEY ("shipping_method_id") REFERENCES "spree_shipping_methods" ("id");
ALTER TABLE "spree_refunds" ADD CONSTRAINT "wetune_fk_6c3d75be91445329" FOREIGN KEY ("payment_id") REFERENCES "spree_payments" ("id");
ALTER TABLE "spree_shipping_method_zones" ADD CONSTRAINT "wetune_fk_83ce47828a1cdfaa" FOREIGN KEY ("shipping_method_id") REFERENCES "spree_shipping_methods" ("id");
ALTER TABLE "spree_option_values" ADD CONSTRAINT "wetune_fk_49b32efa10719d11" FOREIGN KEY ("option_type_id") REFERENCES "spree_option_types" ("id");
ALTER TABLE "spree_variant_property_rule_conditions" ADD CONSTRAINT "wetune_fk_37db4118902719d3" FOREIGN KEY ("option_value_id") REFERENCES "spree_option_values" ("id");
ALTER TABLE "spree_promotion_codes" ADD CONSTRAINT "wetune_fk_bdb22321dffb5ec8" FOREIGN KEY ("promotion_id") REFERENCES "spree_promotions" ("id");
ALTER TABLE "spree_variants" ADD CONSTRAINT "wetune_fk_b6871e77d3e156f3" FOREIGN KEY ("product_id") REFERENCES "spree_products" ("id");
ALTER TABLE "spree_inventory_units" ADD CONSTRAINT "wetune_fk_8cbf6e78e5d41c0c" FOREIGN KEY ("line_item_id") REFERENCES "spree_line_items" ("id");
ALTER TABLE "spree_user_stock_locations" ADD CONSTRAINT "wetune_fk_b6d3b27ca916de2c" FOREIGN KEY ("stock_location_id") REFERENCES "spree_stock_locations" ("id");
ALTER TABLE "spree_promotion_rule_taxons" ADD CONSTRAINT "wetune_fk_5f6945ebb6722d44" FOREIGN KEY ("taxon_id") REFERENCES "spree_taxons" ("id");
ALTER TABLE "spree_orders_promotions" ADD CONSTRAINT "wetune_fk_895fa5f2ec6ffc1b" FOREIGN KEY ("promotion_id") REFERENCES "spree_promotions" ("id");
ALTER TABLE "spree_zone_members" ADD CONSTRAINT "wetune_fk_c3508806b44f5ace" FOREIGN KEY ("zoneable_id") REFERENCES "spree_countries" ("id");
ALTER TABLE "spree_option_values_variants" ADD CONSTRAINT "wetune_fk_b9ead0663629f34c" FOREIGN KEY ("option_value_id") REFERENCES "spree_option_values" ("id");
ALTER TABLE "spree_stock_movements" ADD CONSTRAINT "wetune_fk_45c8ecfc0d0ec1c4" FOREIGN KEY ("stock_item_id") REFERENCES "spree_stock_items" ("id");
ALTER TABLE "spree_product_promotion_rules" ADD CONSTRAINT "wetune_fk_9ebe708d004ad585" FOREIGN KEY ("promotion_rule_id") REFERENCES "spree_promotion_rules" ("id");
ALTER TABLE "spree_inventory_units" ADD CONSTRAINT "wetune_fk_04a750998bfc21fa" FOREIGN KEY ("carton_id") REFERENCES "spree_cartons" ("id");
ALTER TABLE "spree_products_taxons" ADD CONSTRAINT "wetune_fk_3d84e4cd4fe54bbb" FOREIGN KEY ("taxon_id") REFERENCES "spree_taxons" ("id");
ALTER TABLE "spree_shipping_rates" ADD CONSTRAINT "wetune_fk_654d35d222882c8b" FOREIGN KEY ("shipping_method_id") REFERENCES "spree_shipping_methods" ("id");
ALTER TABLE "spree_roles_users" ADD CONSTRAINT "wetune_fk_03b62e0b75108f5e" FOREIGN KEY ("user_id") REFERENCES "spree_users" ("id");
ALTER TABLE "spree_product_option_types" ADD CONSTRAINT "wetune_fk_e7638cfddebce536" FOREIGN KEY ("option_type_id") REFERENCES "spree_option_types" ("id");
ALTER TABLE "spree_adjustments" ADD CONSTRAINT "wetune_fk_045b8f5e1ce64fd7" FOREIGN KEY ("adjustable_id") REFERENCES "spree_shipments" ("id");
ALTER TABLE "friendly_id_slugs" ADD CONSTRAINT "wetune_fk_59b7293c76a5d805" FOREIGN KEY ("sluggable_id") REFERENCES "spree_products" ("id");
ALTER TABLE "spree_shipping_method_categories" ADD CONSTRAINT "wetune_fk_15b6913bc905b24c" FOREIGN KEY ("shipping_category_id") REFERENCES "spree_shipping_categories" ("id");
ALTER TABLE "spree_inventory_units" ADD CONSTRAINT "wetune_fk_8da518bbf3ae6349" FOREIGN KEY ("variant_id") REFERENCES "spree_variants" ("id");
ALTER TABLE "spree_adjustments" ADD CONSTRAINT "wetune_fk_f46a5c77ffd9a535" FOREIGN KEY ("order_id") REFERENCES "spree_orders" ("id");
ALTER TABLE "spree_product_properties" ADD CONSTRAINT "wetune_fk_92d83e5720a44fe9" FOREIGN KEY ("property_id") REFERENCES "spree_properties" ("id");
ALTER TABLE "spree_line_items" ADD CONSTRAINT "wetune_fk_dbd950e4198c4bc7" FOREIGN KEY ("variant_id") REFERENCES "spree_variants" ("id");
ALTER TABLE "spree_prices" ADD CONSTRAINT "wetune_fk_c5de188727f2236e" FOREIGN KEY ("variant_id") REFERENCES "spree_variants" ("id");
ALTER TABLE "spree_orders" ADD CONSTRAINT "wetune_fk_bfa1f0c57b976b7e" FOREIGN KEY ("user_id") REFERENCES "spree_users" ("id");
ALTER TABLE "spree_user_addresses" ADD CONSTRAINT "wetune_fk_792e11bdf7dfcea6" FOREIGN KEY ("address_id") REFERENCES "spree_addresses" ("id");
ALTER TABLE "spree_products_taxons" ADD CONSTRAINT "wetune_fk_72ddc8db5be8e327" FOREIGN KEY ("product_id") REFERENCES "spree_products" ("id");
ALTER TABLE "spree_return_items" ADD CONSTRAINT "wetune_fk_d1fe49b23edede0a" FOREIGN KEY ("inventory_unit_id") REFERENCES "spree_inventory_units" ("id");
ALTER TABLE "spree_product_promotion_rules" ADD CONSTRAINT "wetune_fk_e0c59868e7c2164f" FOREIGN KEY ("product_id") REFERENCES "spree_products" ("id");
ALTER TABLE "spree_zone_members" ADD CONSTRAINT "wetune_fk_5e2c97a6bf2bc2ef" FOREIGN KEY ("zone_id") REFERENCES "spree_zones" ("id");
ALTER TABLE "spree_promotion_actions" ADD CONSTRAINT "wetune_fk_8241f6c398a14c24" FOREIGN KEY ("promotion_id") REFERENCES "spree_promotions" ("id");
ALTER TABLE "spree_option_values_variants" ADD CONSTRAINT "wetune_fk_738574d3b5d379fe" FOREIGN KEY ("variant_id") REFERENCES "spree_variants" ("id");
ALTER TABLE "spree_store_payment_methods" ADD CONSTRAINT "wetune_fk_618885425ff947e7" FOREIGN KEY ("store_id") REFERENCES "spree_stores" ("id");
ALTER TABLE "spree_shipping_method_categories" ADD CONSTRAINT "wetune_fk_332431c1dc771270" FOREIGN KEY ("shipping_method_id") REFERENCES "spree_shipping_methods" ("id");
ALTER TABLE "spree_store_shipping_methods" ADD CONSTRAINT "wetune_fk_45b1d6820470ca97" FOREIGN KEY ("shipping_method_id") REFERENCES "spree_shipping_methods" ("id");
ALTER TABLE "spree_shipments" ADD CONSTRAINT "wetune_fk_240f0ff589994153" FOREIGN KEY ("order_id") REFERENCES "spree_orders" ("id");
ALTER TABLE "spree_roles_users" ADD CONSTRAINT "wetune_fk_715cfe7153ab040d" FOREIGN KEY ("role_id") REFERENCES "spree_roles" ("id");
ALTER TABLE "spree_shipping_method_zones" ADD CONSTRAINT "wetune_fk_ad3a41b1de36c14f" FOREIGN KEY ("zone_id") REFERENCES "spree_zones" ("id");
ALTER TABLE "spree_inventory_units" ADD CONSTRAINT "wetune_fk_3a07e6f264950a85" FOREIGN KEY ("shipment_id") REFERENCES "spree_shipments" ("id");
