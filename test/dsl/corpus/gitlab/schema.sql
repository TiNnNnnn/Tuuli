CREATE TABLE public.abuse_reports (id INT NOT NULL, reporter_id INT, user_id INT, message TEXT, created_at TIMESTAMP, updated_at TIMESTAMP, message_html TEXT, cached_markdown_version INT);

CREATE TABLE public.alerts_service_data (id BIGINT NOT NULL, service_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, encrypted_token VARCHAR(255), encrypted_token_iv VARCHAR(255));

CREATE TABLE public.allowed_email_domains (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, group_id INT NOT NULL, domain VARCHAR(255) NOT NULL);

CREATE TABLE public.analytics_cycle_analytics_group_stages (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, relative_position INT, start_event_identifier INT NOT NULL, end_event_identifier INT NOT NULL, group_id BIGINT NOT NULL, start_event_label_id BIGINT, end_event_label_id BIGINT, hidden BOOLEAN DEFAULT FALSE NOT NULL, custom BOOLEAN DEFAULT TRUE NOT NULL, name VARCHAR(255) NOT NULL);

CREATE TABLE public.analytics_cycle_analytics_project_stages (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, relative_position INT, start_event_identifier INT NOT NULL, end_event_identifier INT NOT NULL, project_id BIGINT NOT NULL, start_event_label_id BIGINT, end_event_label_id BIGINT, hidden BOOLEAN DEFAULT FALSE NOT NULL, custom BOOLEAN DEFAULT TRUE NOT NULL, name VARCHAR(255) NOT NULL);

CREATE TABLE public.analytics_language_trend_repository_languages (file_count INT DEFAULT 0 NOT NULL, programming_language_id BIGINT NOT NULL, project_id BIGINT NOT NULL, loc INT DEFAULT 0 NOT NULL, bytes INT DEFAULT 0 NOT NULL, percentage SMALLINT DEFAULT 0 NOT NULL, snapshot_date DATE NOT NULL);

CREATE TABLE public.appearances (id INT NOT NULL, title VARCHAR NOT NULL, description TEXT NOT NULL, logo VARCHAR, updated_by INT, header_logo VARCHAR, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, description_html TEXT, cached_markdown_version INT, new_project_guidelines TEXT, new_project_guidelines_html TEXT, header_message TEXT, header_message_html TEXT, footer_message TEXT, footer_message_html TEXT, message_background_color TEXT, message_font_color TEXT, favicon VARCHAR, email_header_and_footer_enabled BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.application_setting_terms (id INT NOT NULL, cached_markdown_version INT, terms TEXT NOT NULL, terms_html TEXT);

CREATE TABLE public.application_settings (id INT NOT NULL, default_projects_limit INT, signup_enabled BOOLEAN, gravatar_enabled BOOLEAN, sign_in_text TEXT, created_at TIMESTAMP, updated_at TIMESTAMP, home_page_url VARCHAR, default_branch_protection INT DEFAULT 2, help_text TEXT, restricted_visibility_levels TEXT, version_check_enabled BOOLEAN DEFAULT TRUE, max_attachment_size INT DEFAULT 10 NOT NULL, default_project_visibility INT DEFAULT 0 NOT NULL, default_snippet_visibility INT DEFAULT 0 NOT NULL, domain_whitelist TEXT, user_oauth_applications BOOLEAN DEFAULT TRUE, after_sign_out_path VARCHAR, session_expire_delay INT DEFAULT 10080 NOT NULL, import_sources TEXT, help_page_text TEXT, admin_notification_email VARCHAR, shared_runners_enabled BOOLEAN DEFAULT TRUE NOT NULL, max_artifacts_size INT DEFAULT 100 NOT NULL, runners_registration_token VARCHAR, max_pages_size INT DEFAULT 100 NOT NULL, require_two_factor_authentication BOOLEAN DEFAULT FALSE, two_factor_grace_period INT DEFAULT 48, metrics_enabled BOOLEAN DEFAULT FALSE, metrics_host VARCHAR DEFAULT CAST('localhost' AS VARCHAR), metrics_pool_size INT DEFAULT 16, metrics_timeout INT DEFAULT 10, metrics_method_call_threshold INT DEFAULT 10, recaptcha_enabled BOOLEAN DEFAULT FALSE, metrics_port INT DEFAULT 8089, akismet_enabled BOOLEAN DEFAULT FALSE, metrics_sample_interval INT DEFAULT 15, email_author_in_body BOOLEAN DEFAULT FALSE, default_group_visibility INT, repository_checks_enabled BOOLEAN DEFAULT FALSE, shared_runners_text TEXT, metrics_packet_size INT DEFAULT 1, disabled_oauth_sign_in_sources TEXT, health_check_access_token VARCHAR, send_user_confirmation_email BOOLEAN DEFAULT FALSE, container_registry_token_expire_delay INT DEFAULT 5, after_sign_up_text TEXT, user_default_external BOOLEAN DEFAULT FALSE NOT NULL, elasticsearch_indexing BOOLEAN DEFAULT FALSE NOT NULL, elasticsearch_search BOOLEAN DEFAULT FALSE NOT NULL, repository_storages VARCHAR DEFAULT CAST('default' AS VARCHAR), enabled_git_access_protocol VARCHAR, domain_blacklist_enabled BOOLEAN DEFAULT FALSE, domain_blacklist TEXT, usage_ping_enabled BOOLEAN DEFAULT TRUE NOT NULL, sign_in_text_html TEXT, help_page_text_html TEXT, shared_runners_text_html TEXT, after_sign_up_text_html TEXT, rsa_key_restriction INT DEFAULT 0 NOT NULL, dsa_key_restriction INT DEFAULT CAST('-1' AS INT) NOT NULL, ecdsa_key_restriction INT DEFAULT 0 NOT NULL, ed25519_key_restriction INT DEFAULT 0 NOT NULL, housekeeping_enabled BOOLEAN DEFAULT TRUE NOT NULL, housekeeping_bitmaps_enabled BOOLEAN DEFAULT TRUE NOT NULL, housekeeping_incremental_repack_period INT DEFAULT 10 NOT NULL, housekeeping_full_repack_period INT DEFAULT 50 NOT NULL, housekeeping_gc_period INT DEFAULT 200 NOT NULL, html_emails_enabled BOOLEAN DEFAULT TRUE, plantuml_url VARCHAR, plantuml_enabled BOOLEAN, shared_runners_minutes INT DEFAULT 0 NOT NULL, repository_size_limit BIGINT DEFAULT 0, terminal_max_session_time INT DEFAULT 0 NOT NULL, unique_ips_limit_per_user INT, unique_ips_limit_time_window INT, unique_ips_limit_enabled BOOLEAN DEFAULT FALSE NOT NULL, default_artifacts_expire_in VARCHAR DEFAULT CAST('0' AS VARCHAR) NOT NULL, elasticsearch_url VARCHAR DEFAULT CAST('http://localhost:9200' AS VARCHAR), elasticsearch_aws BOOLEAN DEFAULT FALSE NOT NULL, elasticsearch_aws_region VARCHAR DEFAULT CAST('us-east-1' AS VARCHAR), elasticsearch_aws_access_key VARCHAR, geo_status_timeout INT DEFAULT 10, uuid VARCHAR, polling_interval_multiplier DECIMAL DEFAULT 1.0 NOT NULL, elasticsearch_experimental_indexer BOOLEAN, cached_markdown_version INT, check_namespace_plan BOOLEAN DEFAULT FALSE NOT NULL, mirror_max_delay INT DEFAULT 300 NOT NULL, mirror_max_capacity INT DEFAULT 100 NOT NULL, mirror_capacity_threshold INT DEFAULT 50 NOT NULL, prometheus_metrics_enabled BOOLEAN DEFAULT TRUE NOT NULL, authorized_keys_enabled BOOLEAN DEFAULT TRUE NOT NULL, help_page_hide_commercial_content BOOLEAN DEFAULT FALSE, help_page_support_url VARCHAR, slack_app_enabled BOOLEAN DEFAULT FALSE, slack_app_id VARCHAR, performance_bar_allowed_group_id INT, allow_group_owners_to_manage_ldap BOOLEAN DEFAULT TRUE NOT NULL, hashed_storage_enabled BOOLEAN DEFAULT TRUE NOT NULL, project_export_enabled BOOLEAN DEFAULT TRUE NOT NULL, auto_devops_enabled BOOLEAN DEFAULT TRUE NOT NULL, throttle_unauthenticated_enabled BOOLEAN DEFAULT FALSE NOT NULL, throttle_unauthenticated_requests_per_period INT DEFAULT 3600 NOT NULL, throttle_unauthenticated_period_in_seconds INT DEFAULT 3600 NOT NULL, throttle_authenticated_api_enabled BOOLEAN DEFAULT FALSE NOT NULL, throttle_authenticated_api_requests_per_period INT DEFAULT 7200 NOT NULL, throttle_authenticated_api_period_in_seconds INT DEFAULT 3600 NOT NULL, throttle_authenticated_web_enabled BOOLEAN DEFAULT FALSE NOT NULL, throttle_authenticated_web_requests_per_period INT DEFAULT 7200 NOT NULL, throttle_authenticated_web_period_in_seconds INT DEFAULT 3600 NOT NULL, gitaly_timeout_default INT DEFAULT 55 NOT NULL, gitaly_timeout_medium INT DEFAULT 30 NOT NULL, gitaly_timeout_fast INT DEFAULT 10 NOT NULL, mirror_available BOOLEAN DEFAULT TRUE NOT NULL, password_authentication_enabled_for_web BOOLEAN, password_authentication_enabled_for_git BOOLEAN DEFAULT TRUE NOT NULL, auto_devops_domain VARCHAR, external_authorization_service_enabled BOOLEAN DEFAULT FALSE NOT NULL, external_authorization_service_url VARCHAR, external_authorization_service_default_label VARCHAR, pages_domain_verification_enabled BOOLEAN DEFAULT TRUE NOT NULL, user_default_internal_regex VARCHAR, external_authorization_service_timeout DOUBLE PRECISION DEFAULT 0.5, external_auth_client_cert TEXT, encrypted_external_auth_client_key TEXT, encrypted_external_auth_client_key_iv VARCHAR, encrypted_external_auth_client_key_pass VARCHAR, encrypted_external_auth_client_key_pass_iv VARCHAR, email_additional_text VARCHAR, enforce_terms BOOLEAN DEFAULT FALSE, file_template_project_id INT, pseudonymizer_enabled BOOLEAN DEFAULT FALSE NOT NULL, hide_third_party_offers BOOLEAN DEFAULT FALSE NOT NULL, snowplow_enabled BOOLEAN DEFAULT FALSE NOT NULL, snowplow_collector_hostname VARCHAR, snowplow_cookie_domain VARCHAR, instance_statistics_visibility_private BOOLEAN DEFAULT FALSE NOT NULL, web_ide_clientside_preview_enabled BOOLEAN DEFAULT FALSE NOT NULL, user_show_add_ssh_key_message BOOLEAN DEFAULT TRUE NOT NULL, custom_project_templates_group_id INT, usage_stats_set_by_user_id INT, receive_max_input_size INT, diff_max_patch_bytes INT DEFAULT 102400 NOT NULL, archive_builds_in_seconds INT, commit_email_hostname VARCHAR, protected_ci_variables BOOLEAN DEFAULT FALSE NOT NULL, runners_registration_token_encrypted VARCHAR, local_markdown_version INT DEFAULT 0 NOT NULL, first_day_of_week INT DEFAULT 0 NOT NULL, elasticsearch_limit_indexing BOOLEAN DEFAULT FALSE NOT NULL, default_project_creation INT DEFAULT 2 NOT NULL, lets_encrypt_notification_email VARCHAR, lets_encrypt_terms_of_service_accepted BOOLEAN DEFAULT FALSE NOT NULL, geo_node_allowed_ips VARCHAR DEFAULT CAST('0.0.0.0/0, ::/0' AS VARCHAR), elasticsearch_shards INT DEFAULT 5 NOT NULL, elasticsearch_replicas INT DEFAULT 1 NOT NULL, encrypted_lets_encrypt_private_key TEXT, encrypted_lets_encrypt_private_key_iv TEXT, required_instance_ci_template VARCHAR, dns_rebinding_protection_enabled BOOLEAN DEFAULT TRUE NOT NULL, default_project_deletion_protection BOOLEAN DEFAULT FALSE NOT NULL, grafana_enabled BOOLEAN DEFAULT FALSE NOT NULL, lock_memberships_to_ldap BOOLEAN DEFAULT FALSE NOT NULL, time_tracking_limit_to_hours BOOLEAN DEFAULT FALSE NOT NULL, grafana_url VARCHAR DEFAULT CAST('/-/grafana' AS VARCHAR) NOT NULL, login_recaptcha_protection_enabled BOOLEAN DEFAULT FALSE NOT NULL, outbound_local_requests_whitelist VARCHAR(255)[] DEFAULT CAST('{}' AS VARCHAR[]) NOT NULL, raw_blob_request_limit INT DEFAULT 300 NOT NULL, allow_local_requests_from_web_hooks_and_services BOOLEAN DEFAULT FALSE NOT NULL, allow_local_requests_from_system_hooks BOOLEAN DEFAULT TRUE NOT NULL, instance_administration_project_id BIGINT, asset_proxy_enabled BOOLEAN DEFAULT FALSE NOT NULL, asset_proxy_url VARCHAR, asset_proxy_whitelist TEXT, encrypted_asset_proxy_secret_key TEXT, encrypted_asset_proxy_secret_key_iv VARCHAR, static_objects_external_storage_url VARCHAR(255), static_objects_external_storage_auth_token VARCHAR(255), max_personal_access_token_lifetime INT, throttle_protected_paths_enabled BOOLEAN DEFAULT FALSE NOT NULL, throttle_protected_paths_requests_per_period INT DEFAULT 10 NOT NULL, throttle_protected_paths_period_in_seconds INT DEFAULT 60 NOT NULL, protected_paths VARCHAR(255)[] DEFAULT CAST('{/users/password,/users/sign_in,/api/v3/session.json,/api/v3/session,/api/v4/session.json,/api/v4/session,/users,/users/confirmation,/unsubscribes/,/import/github/personal_access_token,/admin/session}' AS VARCHAR[]), throttle_incident_management_notification_enabled BOOLEAN DEFAULT FALSE NOT NULL, throttle_incident_management_notification_period_in_seconds INT DEFAULT 3600, throttle_incident_management_notification_per_period INT DEFAULT 3600, snowplow_iglu_registry_url VARCHAR(255), push_event_hooks_limit INT DEFAULT 3 NOT NULL, push_event_activities_limit INT DEFAULT 3 NOT NULL, custom_http_clone_url_root VARCHAR(511), deletion_adjourned_period INT DEFAULT 7 NOT NULL, license_trial_ends_on DATE, eks_integration_enabled BOOLEAN DEFAULT FALSE NOT NULL, eks_account_id VARCHAR(128), eks_access_key_id VARCHAR(128), encrypted_eks_secret_access_key_iv VARCHAR(255), encrypted_eks_secret_access_key TEXT, snowplow_app_id VARCHAR, productivity_analytics_start_date TIMESTAMPTZ, default_ci_config_path VARCHAR(255), sourcegraph_enabled BOOLEAN DEFAULT FALSE NOT NULL, sourcegraph_url VARCHAR(255), sourcegraph_public_only BOOLEAN DEFAULT TRUE NOT NULL, snippet_size_limit BIGINT DEFAULT 52428800 NOT NULL, minimum_password_length INT DEFAULT 8 NOT NULL, encrypted_akismet_api_key TEXT, encrypted_akismet_api_key_iv VARCHAR(255), encrypted_elasticsearch_aws_secret_access_key TEXT, encrypted_elasticsearch_aws_secret_access_key_iv VARCHAR(255), encrypted_recaptcha_private_key TEXT, encrypted_recaptcha_private_key_iv VARCHAR(255), encrypted_recaptcha_site_key TEXT, encrypted_recaptcha_site_key_iv VARCHAR(255), encrypted_slack_app_secret TEXT, encrypted_slack_app_secret_iv VARCHAR(255), encrypted_slack_app_verification_token TEXT, encrypted_slack_app_verification_token_iv VARCHAR(255), force_pages_access_control BOOLEAN DEFAULT FALSE NOT NULL, updating_name_disabled_for_users BOOLEAN DEFAULT FALSE NOT NULL, instance_administrators_group_id INT, elasticsearch_indexed_field_length_limit INT DEFAULT 0 NOT NULL, elasticsearch_max_bulk_size_mb SMALLINT DEFAULT 10 NOT NULL, elasticsearch_max_bulk_concurrency SMALLINT DEFAULT 10 NOT NULL, disable_overriding_approvers_per_merge_request BOOLEAN DEFAULT FALSE NOT NULL, prevent_merge_requests_author_approval BOOLEAN DEFAULT FALSE NOT NULL, prevent_merge_requests_committers_approval BOOLEAN DEFAULT FALSE NOT NULL, email_restrictions_enabled BOOLEAN DEFAULT FALSE NOT NULL, email_restrictions TEXT, npm_package_requests_forwarding BOOLEAN DEFAULT TRUE NOT NULL, namespace_storage_size_limit BIGINT DEFAULT 0 NOT NULL);

CREATE TABLE public.approval_merge_request_rule_sources (id BIGINT NOT NULL, approval_merge_request_rule_id BIGINT NOT NULL, approval_project_rule_id BIGINT NOT NULL);

CREATE TABLE public.approval_merge_request_rules (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, merge_request_id INT NOT NULL, approvals_required SMALLINT DEFAULT 0 NOT NULL, code_owner BOOLEAN DEFAULT FALSE NOT NULL, name VARCHAR NOT NULL, rule_type SMALLINT DEFAULT 1 NOT NULL, report_type SMALLINT);

CREATE TABLE public.approval_merge_request_rules_approved_approvers (id BIGINT NOT NULL, approval_merge_request_rule_id BIGINT NOT NULL, user_id INT NOT NULL);

CREATE TABLE public.approval_merge_request_rules_groups (id BIGINT NOT NULL, approval_merge_request_rule_id BIGINT NOT NULL, group_id INT NOT NULL);

CREATE TABLE public.approval_merge_request_rules_users (id BIGINT NOT NULL, approval_merge_request_rule_id BIGINT NOT NULL, user_id INT NOT NULL);

CREATE TABLE public.approval_project_rules (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id INT NOT NULL, approvals_required SMALLINT DEFAULT 0 NOT NULL, name VARCHAR NOT NULL, rule_type SMALLINT DEFAULT 0 NOT NULL);

CREATE TABLE public.approval_project_rules_groups (id BIGINT NOT NULL, approval_project_rule_id BIGINT NOT NULL, group_id INT NOT NULL);

CREATE TABLE public.approval_project_rules_protected_branches (approval_project_rule_id BIGINT NOT NULL, protected_branch_id BIGINT NOT NULL);

CREATE TABLE public.approval_project_rules_users (id BIGINT NOT NULL, approval_project_rule_id BIGINT NOT NULL, user_id INT NOT NULL);

CREATE TABLE public.approvals (id INT NOT NULL, merge_request_id INT NOT NULL, user_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.approver_groups (id INT NOT NULL, target_id INT NOT NULL, target_type VARCHAR NOT NULL, group_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.approvers (id INT NOT NULL, target_id INT NOT NULL, target_type VARCHAR, user_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.ar_internal_metadata (key VARCHAR NOT NULL, value VARCHAR, created_at TIMESTAMP(6) NOT NULL, updated_at TIMESTAMP(6) NOT NULL);

CREATE TABLE public.audit_events (id INT NOT NULL, author_id INT NOT NULL, type VARCHAR NOT NULL, entity_id INT NOT NULL, entity_type VARCHAR NOT NULL, details TEXT, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.award_emoji (id INT NOT NULL, name VARCHAR, user_id INT, awardable_id INT, awardable_type VARCHAR, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.aws_roles (user_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, role_arn VARCHAR(2048) NOT NULL, role_external_id VARCHAR(64) NOT NULL);

CREATE TABLE public.badges (id INT NOT NULL, link_url VARCHAR NOT NULL, image_url VARCHAR NOT NULL, project_id INT, group_id INT, type VARCHAR NOT NULL, name VARCHAR(255), created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.board_assignees (id INT NOT NULL, board_id INT NOT NULL, assignee_id INT NOT NULL);

CREATE TABLE public.board_group_recent_visits (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, user_id INT, board_id INT, group_id INT);

CREATE TABLE public.board_labels (id INT NOT NULL, board_id INT NOT NULL, label_id INT NOT NULL);

CREATE TABLE public.board_project_recent_visits (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, user_id INT, project_id INT, board_id INT);

CREATE TABLE public.boards (id INT NOT NULL, project_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, name VARCHAR DEFAULT CAST('Development' AS VARCHAR) NOT NULL, milestone_id INT, group_id INT, weight INT);

CREATE TABLE public.broadcast_messages (id INT NOT NULL, message TEXT NOT NULL, starts_at TIMESTAMP NOT NULL, ends_at TIMESTAMP NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, color VARCHAR, font VARCHAR, message_html TEXT NOT NULL, cached_markdown_version INT, target_path VARCHAR(255), broadcast_type SMALLINT DEFAULT 1 NOT NULL, dismissable BOOLEAN);

CREATE TABLE public.chat_names (id INT NOT NULL, user_id INT NOT NULL, service_id INT NOT NULL, team_id VARCHAR NOT NULL, team_domain VARCHAR, chat_id VARCHAR NOT NULL, chat_name VARCHAR, last_used_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.chat_teams (id INT NOT NULL, namespace_id INT NOT NULL, team_id VARCHAR, name VARCHAR, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.ci_build_needs (id INT NOT NULL, build_id INT NOT NULL, name TEXT NOT NULL, artifacts BOOLEAN DEFAULT TRUE NOT NULL);

CREATE TABLE public.ci_build_trace_chunks (id BIGINT NOT NULL, build_id INT NOT NULL, chunk_index INT NOT NULL, data_store INT NOT NULL, raw_data BYTEA);

CREATE TABLE public.ci_build_trace_section_names (id INT NOT NULL, project_id INT NOT NULL, name VARCHAR NOT NULL);

CREATE TABLE public.ci_build_trace_sections (project_id INT NOT NULL, date_start TIMESTAMP NOT NULL, date_end TIMESTAMP NOT NULL, byte_start BIGINT NOT NULL, byte_end BIGINT NOT NULL, build_id INT NOT NULL, section_name_id INT NOT NULL);

CREATE TABLE public.ci_builds (id INT NOT NULL, status VARCHAR, finished_at TIMESTAMP, trace TEXT, created_at TIMESTAMP, updated_at TIMESTAMP, started_at TIMESTAMP, runner_id INT, coverage DOUBLE PRECISION, commit_id INT, commands TEXT, name VARCHAR, options TEXT, allow_failure BOOLEAN DEFAULT FALSE NOT NULL, stage VARCHAR, trigger_request_id INT, stage_idx INT, tag BOOLEAN, ref VARCHAR, user_id INT, type VARCHAR, target_url VARCHAR, description VARCHAR, artifacts_file TEXT, project_id INT, artifacts_metadata TEXT, erased_by_id INT, erased_at TIMESTAMP, artifacts_expire_at TIMESTAMP, environment VARCHAR, artifacts_size BIGINT, "when" VARCHAR, yaml_variables TEXT, queued_at TIMESTAMP, token VARCHAR, lock_version INT DEFAULT 0, coverage_regex VARCHAR, auto_canceled_by_id INT, retried BOOLEAN, stage_id INT, artifacts_file_store INT, artifacts_metadata_store INT, protected BOOLEAN, failure_reason INT, scheduled_at TIMESTAMPTZ, token_encrypted VARCHAR, upstream_pipeline_id INT, resource_group_id BIGINT, waiting_for_resource_at TIMESTAMPTZ, processed BOOLEAN, scheduling_type SMALLINT);

CREATE TABLE public.ci_builds_metadata (id INT NOT NULL, build_id INT NOT NULL, project_id INT NOT NULL, timeout INT, timeout_source INT DEFAULT 1 NOT NULL, interruptible BOOLEAN, config_options JSONB, config_variables JSONB, has_exposed_artifacts BOOLEAN, environment_auto_stop_in VARCHAR(255), expanded_environment_name VARCHAR(255));

CREATE TABLE public.ci_builds_runner_session (id BIGINT NOT NULL, build_id INT NOT NULL, url VARCHAR NOT NULL, certificate VARCHAR, "authorization" VARCHAR);

CREATE TABLE public.ci_daily_report_results (id BIGINT NOT NULL, date DATE NOT NULL, project_id BIGINT NOT NULL, last_pipeline_id BIGINT NOT NULL, value DOUBLE PRECISION NOT NULL, param_type BIGINT NOT NULL, ref_path VARCHAR NOT NULL, title VARCHAR NOT NULL);

CREATE TABLE public.ci_group_variables (id INT NOT NULL, key VARCHAR NOT NULL, value TEXT, encrypted_value TEXT, encrypted_value_salt VARCHAR, encrypted_value_iv VARCHAR, group_id INT NOT NULL, protected BOOLEAN DEFAULT FALSE NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, masked BOOLEAN DEFAULT FALSE NOT NULL, variable_type SMALLINT DEFAULT 1 NOT NULL);

CREATE TABLE public.ci_job_artifacts (id INT NOT NULL, project_id INT NOT NULL, job_id INT NOT NULL, file_type INT NOT NULL, size BIGINT, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, expire_at TIMESTAMPTZ, file VARCHAR, file_store INT, file_sha256 BYTEA, file_format SMALLINT, file_location SMALLINT);

CREATE TABLE public.ci_job_variables (id BIGINT NOT NULL, key VARCHAR NOT NULL, encrypted_value TEXT, encrypted_value_iv VARCHAR, job_id BIGINT NOT NULL, variable_type SMALLINT DEFAULT 1 NOT NULL, source SMALLINT DEFAULT 0 NOT NULL);

CREATE TABLE public.ci_pipeline_chat_data (id BIGINT NOT NULL, pipeline_id INT NOT NULL, chat_name_id INT NOT NULL, response_url TEXT NOT NULL);

CREATE TABLE public.ci_pipeline_schedule_variables (id INT NOT NULL, key VARCHAR NOT NULL, value TEXT, encrypted_value TEXT, encrypted_value_salt VARCHAR, encrypted_value_iv VARCHAR, pipeline_schedule_id INT NOT NULL, created_at TIMESTAMPTZ, updated_at TIMESTAMPTZ, variable_type SMALLINT DEFAULT 1 NOT NULL);

CREATE TABLE public.ci_pipeline_schedules (id INT NOT NULL, description VARCHAR, ref VARCHAR, cron VARCHAR, cron_timezone VARCHAR, next_run_at TIMESTAMP, project_id INT, owner_id INT, active BOOLEAN DEFAULT TRUE, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.ci_pipeline_variables (id INT NOT NULL, key VARCHAR NOT NULL, value TEXT, encrypted_value TEXT, encrypted_value_salt VARCHAR, encrypted_value_iv VARCHAR, pipeline_id INT NOT NULL, variable_type SMALLINT DEFAULT 1 NOT NULL);

CREATE TABLE public.ci_pipelines (id INT NOT NULL, ref VARCHAR, sha VARCHAR, before_sha VARCHAR, created_at TIMESTAMP, updated_at TIMESTAMP, tag BOOLEAN DEFAULT FALSE, yaml_errors TEXT, committed_at TIMESTAMP, project_id INT, status VARCHAR, started_at TIMESTAMP, finished_at TIMESTAMP, duration INT, user_id INT, lock_version INT DEFAULT 0, auto_canceled_by_id INT, pipeline_schedule_id INT, source INT, config_source INT, protected BOOLEAN, failure_reason INT, iid INT, merge_request_id INT, source_sha BYTEA, target_sha BYTEA, external_pull_request_id BIGINT);

CREATE TABLE public.ci_pipelines_config (pipeline_id BIGINT NOT NULL, content TEXT NOT NULL);

CREATE TABLE public.ci_refs (id BIGINT NOT NULL, project_id INT NOT NULL, lock_version INT DEFAULT 0, last_updated_by_pipeline_id INT, tag BOOLEAN DEFAULT FALSE NOT NULL, ref VARCHAR(255) NOT NULL, status VARCHAR(255) NOT NULL);

CREATE TABLE public.ci_resource_groups (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id BIGINT NOT NULL, key VARCHAR(255) NOT NULL);

CREATE TABLE public.ci_resources (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, resource_group_id BIGINT NOT NULL, build_id BIGINT);

CREATE TABLE public.ci_runner_namespaces (id INT NOT NULL, runner_id INT, namespace_id INT);

CREATE TABLE public.ci_runner_projects (id INT NOT NULL, runner_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, project_id INT);

CREATE TABLE public.ci_runners (id INT NOT NULL, token VARCHAR, created_at TIMESTAMP, updated_at TIMESTAMP, description VARCHAR, contacted_at TIMESTAMP, active BOOLEAN DEFAULT TRUE NOT NULL, is_shared BOOLEAN DEFAULT FALSE, name VARCHAR, version VARCHAR, revision VARCHAR, platform VARCHAR, architecture VARCHAR, run_untagged BOOLEAN DEFAULT TRUE NOT NULL, locked BOOLEAN DEFAULT FALSE NOT NULL, access_level INT DEFAULT 0 NOT NULL, ip_address VARCHAR, maximum_timeout INT, runner_type SMALLINT NOT NULL, token_encrypted VARCHAR, public_projects_minutes_cost_factor DOUBLE PRECISION DEFAULT 0.0 NOT NULL, private_projects_minutes_cost_factor DOUBLE PRECISION DEFAULT 1.0 NOT NULL);

CREATE TABLE public.ci_sources_pipelines (id INT NOT NULL, project_id INT, pipeline_id INT, source_project_id INT, source_job_id INT, source_pipeline_id INT);

CREATE TABLE public.ci_sources_projects (id BIGINT NOT NULL, pipeline_id BIGINT NOT NULL, source_project_id BIGINT NOT NULL);

CREATE TABLE public.ci_stages (id INT NOT NULL, project_id INT, pipeline_id INT, created_at TIMESTAMP, updated_at TIMESTAMP, name VARCHAR, status INT, lock_version INT DEFAULT 0, "position" INT);

CREATE TABLE public.ci_subscriptions_projects (id BIGINT NOT NULL, downstream_project_id BIGINT NOT NULL, upstream_project_id BIGINT NOT NULL);

CREATE TABLE public.ci_trigger_requests (id INT NOT NULL, trigger_id INT NOT NULL, variables TEXT, created_at TIMESTAMP, updated_at TIMESTAMP, commit_id INT);

CREATE TABLE public.ci_triggers (id INT NOT NULL, token VARCHAR, created_at TIMESTAMP, updated_at TIMESTAMP, project_id INT, owner_id INT NOT NULL, description VARCHAR, ref VARCHAR);

CREATE TABLE public.ci_variables (id INT NOT NULL, key VARCHAR NOT NULL, value TEXT, encrypted_value TEXT, encrypted_value_salt VARCHAR, encrypted_value_iv VARCHAR, project_id INT NOT NULL, protected BOOLEAN DEFAULT FALSE NOT NULL, environment_scope VARCHAR DEFAULT CAST('*' AS VARCHAR) NOT NULL, masked BOOLEAN DEFAULT FALSE NOT NULL, variable_type SMALLINT DEFAULT 1 NOT NULL);

CREATE TABLE public.cluster_groups (id INT NOT NULL, cluster_id INT NOT NULL, group_id INT NOT NULL);

CREATE TABLE public.cluster_platforms_kubernetes (id INT NOT NULL, cluster_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, api_url TEXT, ca_cert TEXT, namespace VARCHAR, username VARCHAR, encrypted_password TEXT, encrypted_password_iv VARCHAR, encrypted_token TEXT, encrypted_token_iv VARCHAR, authorization_type SMALLINT);

CREATE TABLE public.cluster_projects (id INT NOT NULL, project_id INT NOT NULL, cluster_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.cluster_providers_aws (id BIGINT NOT NULL, cluster_id BIGINT NOT NULL, created_by_user_id INT, num_nodes INT NOT NULL, status INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, key_name VARCHAR(255) NOT NULL, role_arn VARCHAR(2048) NOT NULL, region VARCHAR(255) NOT NULL, vpc_id VARCHAR(255) NOT NULL, subnet_ids VARCHAR(255)[] DEFAULT CAST('{}' AS VARCHAR[]) NOT NULL, security_group_id VARCHAR(255) NOT NULL, instance_type VARCHAR(255) NOT NULL, access_key_id VARCHAR(255), encrypted_secret_access_key_iv VARCHAR(255), encrypted_secret_access_key TEXT, session_token TEXT, status_reason TEXT);

CREATE TABLE public.cluster_providers_gcp (id INT NOT NULL, cluster_id INT NOT NULL, status INT, num_nodes INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, status_reason TEXT, gcp_project_id VARCHAR NOT NULL, zone VARCHAR NOT NULL, machine_type VARCHAR, operation_id VARCHAR, endpoint VARCHAR, encrypted_access_token TEXT, encrypted_access_token_iv VARCHAR, legacy_abac BOOLEAN DEFAULT FALSE NOT NULL, cloud_run BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.clusters (id INT NOT NULL, user_id INT, provider_type INT, platform_type INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, enabled BOOLEAN DEFAULT TRUE, name VARCHAR NOT NULL, environment_scope VARCHAR DEFAULT CAST('*' AS VARCHAR) NOT NULL, cluster_type SMALLINT DEFAULT 3 NOT NULL, domain VARCHAR, managed BOOLEAN DEFAULT TRUE NOT NULL, namespace_per_environment BOOLEAN DEFAULT TRUE NOT NULL, management_project_id INT, cleanup_status SMALLINT DEFAULT 1 NOT NULL, cleanup_status_reason TEXT);

CREATE TABLE public.clusters_applications_cert_managers (id INT NOT NULL, cluster_id INT NOT NULL, status INT NOT NULL, version VARCHAR NOT NULL, email VARCHAR NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, status_reason TEXT);

CREATE TABLE public.clusters_applications_crossplane (id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, cluster_id BIGINT NOT NULL, status INT NOT NULL, version VARCHAR(255) NOT NULL, stack VARCHAR(255) NOT NULL, status_reason TEXT);

CREATE TABLE public.clusters_applications_elastic_stacks (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, cluster_id BIGINT NOT NULL, status INT NOT NULL, version VARCHAR(255) NOT NULL, status_reason TEXT);

CREATE TABLE public.clusters_applications_helm (id INT NOT NULL, cluster_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, status INT NOT NULL, version VARCHAR NOT NULL, status_reason TEXT, encrypted_ca_key TEXT, encrypted_ca_key_iv TEXT, ca_cert TEXT);

CREATE TABLE public.clusters_applications_ingress (id INT NOT NULL, cluster_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, status INT NOT NULL, ingress_type INT NOT NULL, version VARCHAR NOT NULL, cluster_ip VARCHAR, status_reason TEXT, external_ip VARCHAR, external_hostname VARCHAR, modsecurity_enabled BOOLEAN, modsecurity_mode SMALLINT DEFAULT 0 NOT NULL);

CREATE TABLE public.clusters_applications_jupyter (id INT NOT NULL, cluster_id INT NOT NULL, oauth_application_id INT, status INT NOT NULL, version VARCHAR NOT NULL, hostname VARCHAR, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, status_reason TEXT);

CREATE TABLE public.clusters_applications_knative (id INT NOT NULL, cluster_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, status INT NOT NULL, version VARCHAR NOT NULL, hostname VARCHAR, status_reason TEXT, external_ip VARCHAR, external_hostname VARCHAR);

CREATE TABLE public.clusters_applications_prometheus (id INT NOT NULL, cluster_id INT NOT NULL, status INT NOT NULL, version VARCHAR NOT NULL, status_reason TEXT, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, last_update_started_at TIMESTAMPTZ, encrypted_alert_manager_token VARCHAR, encrypted_alert_manager_token_iv VARCHAR, healthy BOOLEAN);

CREATE TABLE public.clusters_applications_runners (id INT NOT NULL, cluster_id INT NOT NULL, runner_id INT, status INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, version VARCHAR NOT NULL, status_reason TEXT, privileged BOOLEAN DEFAULT TRUE NOT NULL);

CREATE TABLE public.clusters_kubernetes_namespaces (id BIGINT NOT NULL, cluster_id INT NOT NULL, project_id INT, cluster_project_id INT, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, encrypted_service_account_token TEXT, encrypted_service_account_token_iv VARCHAR, namespace VARCHAR NOT NULL, service_account_name VARCHAR, environment_id BIGINT);

CREATE TABLE public.commit_user_mentions (id BIGINT NOT NULL, note_id INT NOT NULL, mentioned_users_ids INT[], mentioned_projects_ids INT[], mentioned_groups_ids INT[], commit_id VARCHAR NOT NULL);

CREATE TABLE public.container_expiration_policies (project_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, next_run_at TIMESTAMPTZ, name_regex VARCHAR(255), cadence VARCHAR(12) DEFAULT CAST('7d' AS VARCHAR) NOT NULL, older_than VARCHAR(12), keep_n INT, enabled BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.container_repositories (id INT NOT NULL, project_id INT NOT NULL, name VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.conversational_development_index_metrics (id INT NOT NULL, leader_issues DOUBLE PRECISION NOT NULL, instance_issues DOUBLE PRECISION NOT NULL, leader_notes DOUBLE PRECISION NOT NULL, instance_notes DOUBLE PRECISION NOT NULL, leader_milestones DOUBLE PRECISION NOT NULL, instance_milestones DOUBLE PRECISION NOT NULL, leader_boards DOUBLE PRECISION NOT NULL, instance_boards DOUBLE PRECISION NOT NULL, leader_merge_requests DOUBLE PRECISION NOT NULL, instance_merge_requests DOUBLE PRECISION NOT NULL, leader_ci_pipelines DOUBLE PRECISION NOT NULL, instance_ci_pipelines DOUBLE PRECISION NOT NULL, leader_environments DOUBLE PRECISION NOT NULL, instance_environments DOUBLE PRECISION NOT NULL, leader_deployments DOUBLE PRECISION NOT NULL, instance_deployments DOUBLE PRECISION NOT NULL, leader_projects_prometheus_active DOUBLE PRECISION NOT NULL, instance_projects_prometheus_active DOUBLE PRECISION NOT NULL, leader_service_desk_issues DOUBLE PRECISION NOT NULL, instance_service_desk_issues DOUBLE PRECISION NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, percentage_boards DOUBLE PRECISION DEFAULT 0.0 NOT NULL, percentage_ci_pipelines DOUBLE PRECISION DEFAULT 0.0 NOT NULL, percentage_deployments DOUBLE PRECISION DEFAULT 0.0 NOT NULL, percentage_environments DOUBLE PRECISION DEFAULT 0.0 NOT NULL, percentage_issues DOUBLE PRECISION DEFAULT 0.0 NOT NULL, percentage_merge_requests DOUBLE PRECISION DEFAULT 0.0 NOT NULL, percentage_milestones DOUBLE PRECISION DEFAULT 0.0 NOT NULL, percentage_notes DOUBLE PRECISION DEFAULT 0.0 NOT NULL, percentage_projects_prometheus_active DOUBLE PRECISION DEFAULT 0.0 NOT NULL, percentage_service_desk_issues DOUBLE PRECISION DEFAULT 0.0 NOT NULL);

CREATE TABLE public.dependency_proxy_blobs (id INT NOT NULL, group_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, size BIGINT, file_store INT, file_name VARCHAR NOT NULL, file TEXT NOT NULL);

CREATE TABLE public.dependency_proxy_group_settings (id INT NOT NULL, group_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, enabled BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.deploy_keys_projects (id INT NOT NULL, deploy_key_id INT NOT NULL, project_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, can_push BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.deploy_tokens (id INT NOT NULL, revoked BOOLEAN DEFAULT FALSE, read_repository BOOLEAN DEFAULT FALSE NOT NULL, read_registry BOOLEAN DEFAULT FALSE NOT NULL, expires_at TIMESTAMPTZ NOT NULL, created_at TIMESTAMPTZ NOT NULL, name VARCHAR NOT NULL, token VARCHAR, username VARCHAR, token_encrypted VARCHAR(255), deploy_token_type SMALLINT DEFAULT 2 NOT NULL);

CREATE TABLE public.deployment_clusters (deployment_id INT NOT NULL, cluster_id INT NOT NULL, kubernetes_namespace VARCHAR(255));

CREATE TABLE public.deployment_merge_requests (deployment_id INT NOT NULL, merge_request_id INT NOT NULL, environment_id INT);

CREATE TABLE public.deployments (id INT NOT NULL, iid INT NOT NULL, project_id INT NOT NULL, environment_id INT NOT NULL, ref VARCHAR NOT NULL, tag BOOLEAN NOT NULL, sha VARCHAR NOT NULL, user_id INT, deployable_id INT, deployable_type VARCHAR, created_at TIMESTAMP, updated_at TIMESTAMP, on_stop VARCHAR, status SMALLINT NOT NULL, finished_at TIMESTAMPTZ, cluster_id INT);

CREATE TABLE public.description_versions (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, issue_id INT, merge_request_id INT, epic_id INT, description TEXT, deleted_at TIMESTAMPTZ);

CREATE TABLE public.design_management_designs (id BIGINT NOT NULL, project_id INT NOT NULL, issue_id INT, filename VARCHAR NOT NULL);

CREATE TABLE public.design_management_designs_versions (id BIGINT NOT NULL, design_id BIGINT NOT NULL, version_id BIGINT NOT NULL, event SMALLINT DEFAULT 0 NOT NULL, image_v432x230 VARCHAR(255));

CREATE TABLE public.design_management_versions (id BIGINT NOT NULL, sha BYTEA NOT NULL, issue_id BIGINT, created_at TIMESTAMPTZ NOT NULL, author_id INT);

CREATE TABLE public.design_user_mentions (id BIGINT NOT NULL, design_id INT NOT NULL, note_id INT NOT NULL, mentioned_users_ids INT[], mentioned_projects_ids INT[], mentioned_groups_ids INT[]);

CREATE TABLE public.draft_notes (id BIGINT NOT NULL, merge_request_id INT NOT NULL, author_id INT NOT NULL, resolve_discussion BOOLEAN DEFAULT FALSE NOT NULL, discussion_id VARCHAR, note TEXT NOT NULL, "position" TEXT, original_position TEXT, change_position TEXT, commit_id BYTEA);

CREATE TABLE public.elasticsearch_indexed_namespaces (created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, namespace_id INT);

CREATE TABLE public.elasticsearch_indexed_projects (created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id INT);

CREATE TABLE public.emails (id INT NOT NULL, user_id INT NOT NULL, email VARCHAR NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, confirmation_token VARCHAR, confirmed_at TIMESTAMP, confirmation_sent_at TIMESTAMP);

CREATE TABLE public.environments (id INT NOT NULL, project_id INT NOT NULL, name VARCHAR NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, external_url VARCHAR, environment_type VARCHAR, state VARCHAR DEFAULT CAST('available' AS VARCHAR) NOT NULL, slug VARCHAR NOT NULL, auto_stop_at TIMESTAMPTZ);

CREATE TABLE public.epic_issues (id INT NOT NULL, epic_id INT NOT NULL, issue_id INT NOT NULL, relative_position INT);

CREATE TABLE public.epic_metrics (id INT NOT NULL, epic_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.epic_user_mentions (id BIGINT NOT NULL, epic_id INT NOT NULL, note_id INT, mentioned_users_ids INT[], mentioned_projects_ids INT[], mentioned_groups_ids INT[]);

CREATE TABLE public.epics (id INT NOT NULL, group_id INT NOT NULL, author_id INT NOT NULL, assignee_id INT, iid INT NOT NULL, cached_markdown_version INT, updated_by_id INT, last_edited_by_id INT, lock_version INT DEFAULT 0, start_date DATE, end_date DATE, last_edited_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, title VARCHAR NOT NULL, title_html VARCHAR NOT NULL, description TEXT, description_html TEXT, start_date_sourcing_milestone_id INT, due_date_sourcing_milestone_id INT, start_date_fixed DATE, due_date_fixed DATE, start_date_is_fixed BOOLEAN, due_date_is_fixed BOOLEAN, closed_by_id INT, closed_at TIMESTAMP, parent_id INT, relative_position INT, state_id SMALLINT DEFAULT 1 NOT NULL, start_date_sourcing_epic_id INT, due_date_sourcing_epic_id INT, health_status SMALLINT, external_key VARCHAR(255));

CREATE TABLE public.events (id INT NOT NULL, project_id INT, author_id INT NOT NULL, target_id INT, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, action SMALLINT NOT NULL, target_type VARCHAR, group_id BIGINT);

CREATE TABLE public.evidences (id BIGINT NOT NULL, release_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, summary_sha BYTEA, summary JSONB DEFAULT CAST('{}' AS JSONB) NOT NULL);

CREATE TABLE public.external_pull_requests (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id BIGINT NOT NULL, pull_request_iid INT NOT NULL, status SMALLINT NOT NULL, source_branch VARCHAR(255) NOT NULL, target_branch VARCHAR(255) NOT NULL, source_repository VARCHAR(255) NOT NULL, target_repository VARCHAR(255) NOT NULL, source_sha BYTEA NOT NULL, target_sha BYTEA NOT NULL);

CREATE TABLE public.feature_gates (id INT NOT NULL, feature_key VARCHAR NOT NULL, key VARCHAR NOT NULL, value VARCHAR, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.features (id INT NOT NULL, key VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.fork_network_members (id INT NOT NULL, fork_network_id INT NOT NULL, project_id INT NOT NULL, forked_from_project_id INT);

CREATE TABLE public.fork_networks (id INT NOT NULL, root_project_id INT, deleted_root_project_name VARCHAR);

CREATE TABLE public.geo_cache_invalidation_events (id BIGINT NOT NULL, key VARCHAR NOT NULL);

CREATE TABLE public.geo_container_repository_updated_events (id BIGINT NOT NULL, container_repository_id INT NOT NULL);

CREATE TABLE public.geo_event_log (id BIGINT NOT NULL, created_at TIMESTAMP NOT NULL, repository_updated_event_id BIGINT, repository_deleted_event_id BIGINT, repository_renamed_event_id BIGINT, repositories_changed_event_id BIGINT, repository_created_event_id BIGINT, hashed_storage_migrated_event_id BIGINT, lfs_object_deleted_event_id BIGINT, hashed_storage_attachments_event_id BIGINT, upload_deleted_event_id BIGINT, job_artifact_deleted_event_id BIGINT, reset_checksum_event_id BIGINT, cache_invalidation_event_id BIGINT, container_repository_updated_event_id BIGINT, geo_event_id INT);

CREATE TABLE public.geo_events (id BIGINT NOT NULL, replicable_name VARCHAR(255) NOT NULL, event_name VARCHAR(255) NOT NULL, payload JSONB DEFAULT CAST('{}' AS JSONB) NOT NULL, created_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.geo_hashed_storage_attachments_events (id BIGINT NOT NULL, project_id INT NOT NULL, old_attachments_path TEXT NOT NULL, new_attachments_path TEXT NOT NULL);

CREATE TABLE public.geo_hashed_storage_migrated_events (id BIGINT NOT NULL, project_id INT NOT NULL, repository_storage_name TEXT NOT NULL, old_disk_path TEXT NOT NULL, new_disk_path TEXT NOT NULL, old_wiki_disk_path TEXT NOT NULL, new_wiki_disk_path TEXT NOT NULL, old_storage_version SMALLINT, new_storage_version SMALLINT NOT NULL, old_design_disk_path TEXT, new_design_disk_path TEXT);

CREATE TABLE public.geo_job_artifact_deleted_events (id BIGINT NOT NULL, job_artifact_id INT NOT NULL, file_path VARCHAR NOT NULL);

CREATE TABLE public.geo_lfs_object_deleted_events (id BIGINT NOT NULL, lfs_object_id INT NOT NULL, oid VARCHAR NOT NULL, file_path VARCHAR NOT NULL);

CREATE TABLE public.geo_node_namespace_links (id INT NOT NULL, geo_node_id INT NOT NULL, namespace_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.geo_node_statuses (id INT NOT NULL, geo_node_id INT NOT NULL, db_replication_lag_seconds INT, repositories_synced_count INT, repositories_failed_count INT, lfs_objects_count INT, lfs_objects_synced_count INT, lfs_objects_failed_count INT, attachments_count INT, attachments_synced_count INT, attachments_failed_count INT, last_event_id INT, last_event_date TIMESTAMP, cursor_last_event_id INT, cursor_last_event_date TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, last_successful_status_check_at TIMESTAMP, status_message VARCHAR, replication_slots_count INT, replication_slots_used_count INT, replication_slots_max_retained_wal_bytes BIGINT, wikis_synced_count INT, wikis_failed_count INT, job_artifacts_count INT, job_artifacts_synced_count INT, job_artifacts_failed_count INT, version VARCHAR, revision VARCHAR, repositories_verified_count INT, repositories_verification_failed_count INT, wikis_verified_count INT, wikis_verification_failed_count INT, lfs_objects_synced_missing_on_primary_count INT, job_artifacts_synced_missing_on_primary_count INT, attachments_synced_missing_on_primary_count INT, repositories_checksummed_count INT, repositories_checksum_failed_count INT, repositories_checksum_mismatch_count INT, wikis_checksummed_count INT, wikis_checksum_failed_count INT, wikis_checksum_mismatch_count INT, storage_configuration_digest BYTEA, repositories_retrying_verification_count INT, wikis_retrying_verification_count INT, projects_count INT, container_repositories_count INT, container_repositories_synced_count INT, container_repositories_failed_count INT, container_repositories_registry_count INT, design_repositories_count INT, design_repositories_synced_count INT, design_repositories_failed_count INT, design_repositories_registry_count INT);

CREATE TABLE public.geo_nodes (id INT NOT NULL, "primary" BOOLEAN DEFAULT FALSE NOT NULL, oauth_application_id INT, enabled BOOLEAN DEFAULT TRUE NOT NULL, access_key VARCHAR, encrypted_secret_access_key VARCHAR, encrypted_secret_access_key_iv VARCHAR, clone_url_prefix VARCHAR, files_max_capacity INT DEFAULT 10 NOT NULL, repos_max_capacity INT DEFAULT 25 NOT NULL, url VARCHAR NOT NULL, selective_sync_type VARCHAR, selective_sync_shards TEXT, verification_max_capacity INT DEFAULT 100 NOT NULL, minimum_reverification_interval INT DEFAULT 7 NOT NULL, internal_url VARCHAR, name VARCHAR NOT NULL, container_repositories_max_capacity INT DEFAULT 10 NOT NULL, created_at TIMESTAMPTZ, updated_at TIMESTAMPTZ, sync_object_storage BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.geo_repositories_changed_events (id BIGINT NOT NULL, geo_node_id INT NOT NULL);

CREATE TABLE public.geo_repository_created_events (id BIGINT NOT NULL, project_id INT NOT NULL, repository_storage_name TEXT NOT NULL, repo_path TEXT NOT NULL, wiki_path TEXT, project_name TEXT NOT NULL);

CREATE TABLE public.geo_repository_deleted_events (id BIGINT NOT NULL, project_id INT NOT NULL, repository_storage_name TEXT NOT NULL, deleted_path TEXT NOT NULL, deleted_wiki_path TEXT, deleted_project_name TEXT NOT NULL);

CREATE TABLE public.geo_repository_renamed_events (id BIGINT NOT NULL, project_id INT NOT NULL, repository_storage_name TEXT NOT NULL, old_path_with_namespace TEXT NOT NULL, new_path_with_namespace TEXT NOT NULL, old_wiki_path_with_namespace TEXT NOT NULL, new_wiki_path_with_namespace TEXT NOT NULL, old_path TEXT NOT NULL, new_path TEXT NOT NULL);

CREATE TABLE public.geo_repository_updated_events (id BIGINT NOT NULL, branches_affected INT NOT NULL, tags_affected INT NOT NULL, project_id INT NOT NULL, source SMALLINT NOT NULL, new_branch BOOLEAN DEFAULT FALSE NOT NULL, remove_branch BOOLEAN DEFAULT FALSE NOT NULL, ref TEXT);

CREATE TABLE public.geo_reset_checksum_events (id BIGINT NOT NULL, project_id INT NOT NULL);

CREATE TABLE public.geo_upload_deleted_events (id BIGINT NOT NULL, upload_id INT NOT NULL, file_path VARCHAR NOT NULL, model_id INT NOT NULL, model_type VARCHAR NOT NULL, uploader VARCHAR NOT NULL);

CREATE TABLE public.gitlab_subscription_histories (id BIGINT NOT NULL, gitlab_subscription_created_at TIMESTAMPTZ, gitlab_subscription_updated_at TIMESTAMPTZ, start_date DATE, end_date DATE, trial_ends_on DATE, namespace_id INT, hosted_plan_id INT, max_seats_used INT, seats INT, trial BOOLEAN, change_type SMALLINT, gitlab_subscription_id BIGINT NOT NULL, created_at TIMESTAMPTZ, trial_starts_on DATE, auto_renew BOOLEAN);

CREATE TABLE public.gitlab_subscriptions (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, start_date DATE, end_date DATE, trial_ends_on DATE, namespace_id INT, hosted_plan_id INT, max_seats_used INT DEFAULT 0, seats INT DEFAULT 0, trial BOOLEAN DEFAULT FALSE, trial_starts_on DATE, auto_renew BOOLEAN);

CREATE TABLE public.gpg_key_subkeys (id INT NOT NULL, gpg_key_id INT NOT NULL, keyid BYTEA, fingerprint BYTEA);

CREATE TABLE public.gpg_keys (id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, user_id INT, primary_keyid BYTEA, fingerprint BYTEA, key TEXT);

CREATE TABLE public.gpg_signatures (id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id INT, gpg_key_id INT, commit_sha BYTEA, gpg_key_primary_keyid BYTEA, gpg_key_user_name TEXT, gpg_key_user_email TEXT, verification_status SMALLINT DEFAULT 0 NOT NULL, gpg_key_subkey_id INT);

CREATE TABLE public.grafana_integrations (id BIGINT NOT NULL, project_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, encrypted_token VARCHAR(255) NOT NULL, encrypted_token_iv VARCHAR(255) NOT NULL, grafana_url VARCHAR(1024) NOT NULL, enabled BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.group_custom_attributes (id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, group_id INT NOT NULL, key VARCHAR NOT NULL, value VARCHAR NOT NULL);

CREATE TABLE public.group_deletion_schedules (group_id BIGINT NOT NULL, user_id BIGINT NOT NULL, marked_for_deletion_on DATE NOT NULL);

CREATE TABLE public.group_deploy_tokens (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, group_id BIGINT NOT NULL, deploy_token_id BIGINT NOT NULL);

CREATE TABLE public.group_group_links (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, shared_group_id BIGINT NOT NULL, shared_with_group_id BIGINT NOT NULL, expires_at DATE, group_access SMALLINT DEFAULT 30 NOT NULL);

CREATE TABLE public.historical_data (id INT NOT NULL, date DATE NOT NULL, active_user_count INT, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.identities (id INT NOT NULL, extern_uid VARCHAR, provider VARCHAR, user_id INT, created_at TIMESTAMP, updated_at TIMESTAMP, secondary_extern_uid VARCHAR, saml_provider_id INT);

CREATE TABLE public.import_export_uploads (id INT NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id INT, import_file TEXT, export_file TEXT, group_id BIGINT);

CREATE TABLE public.import_failures (id BIGINT NOT NULL, relation_index INT, project_id BIGINT, created_at TIMESTAMPTZ NOT NULL, relation_key VARCHAR(64), exception_class VARCHAR(128), correlation_id_value VARCHAR(128), exception_message VARCHAR(255), retry_count INT, group_id INT, source VARCHAR(128));

CREATE TABLE public.index_statuses (id INT NOT NULL, project_id INT NOT NULL, indexed_at TIMESTAMP, note TEXT, last_commit VARCHAR, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, last_wiki_commit BYTEA, wiki_indexed_at TIMESTAMPTZ);

CREATE TABLE public.insights (id INT NOT NULL, namespace_id INT NOT NULL, project_id INT NOT NULL);

CREATE TABLE public.internal_ids (id BIGINT NOT NULL, project_id INT, usage INT NOT NULL, last_value INT NOT NULL, namespace_id INT);

CREATE TABLE public.ip_restrictions (id BIGINT NOT NULL, group_id INT NOT NULL, range VARCHAR NOT NULL);

CREATE TABLE public.issue_assignees (user_id INT NOT NULL, issue_id INT NOT NULL);

CREATE TABLE public.issue_links (id INT NOT NULL, source_id INT NOT NULL, target_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, link_type SMALLINT DEFAULT 0 NOT NULL);

CREATE TABLE public.issue_metrics (id INT NOT NULL, issue_id INT NOT NULL, first_mentioned_in_commit_at TIMESTAMP, first_associated_with_milestone_at TIMESTAMP, first_added_to_board_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.issue_tracker_data (id BIGINT NOT NULL, service_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, encrypted_project_url VARCHAR, encrypted_project_url_iv VARCHAR, encrypted_issues_url VARCHAR, encrypted_issues_url_iv VARCHAR, encrypted_new_issue_url VARCHAR, encrypted_new_issue_url_iv VARCHAR);

CREATE TABLE public.issue_user_mentions (id BIGINT NOT NULL, issue_id INT NOT NULL, note_id INT, mentioned_users_ids INT[], mentioned_projects_ids INT[], mentioned_groups_ids INT[]);

CREATE TABLE public.issues (id INT NOT NULL, title VARCHAR, author_id INT, project_id INT, created_at TIMESTAMP, updated_at TIMESTAMP, description TEXT, milestone_id INT, iid INT, updated_by_id INT, weight INT, confidential BOOLEAN DEFAULT FALSE NOT NULL, due_date DATE, moved_to_id INT, lock_version INT DEFAULT 0, title_html TEXT, description_html TEXT, time_estimate INT, relative_position INT, service_desk_reply_to VARCHAR, cached_markdown_version INT, last_edited_at TIMESTAMP, last_edited_by_id INT, discussion_locked BOOLEAN, closed_at TIMESTAMPTZ, closed_by_id INT, state_id SMALLINT DEFAULT 1 NOT NULL, duplicated_to_id INT, promoted_to_epic_id INT, health_status SMALLINT, external_key VARCHAR(255));

CREATE TABLE public.issues_prometheus_alert_events (issue_id BIGINT NOT NULL, prometheus_alert_event_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.issues_self_managed_prometheus_alert_events (issue_id BIGINT NOT NULL, self_managed_prometheus_alert_event_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.jira_connect_installations (id BIGINT NOT NULL, client_key VARCHAR, encrypted_shared_secret VARCHAR, encrypted_shared_secret_iv VARCHAR, base_url VARCHAR);

CREATE TABLE public.jira_connect_subscriptions (id BIGINT NOT NULL, jira_connect_installation_id BIGINT NOT NULL, namespace_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.jira_tracker_data (id BIGINT NOT NULL, service_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, encrypted_url VARCHAR, encrypted_url_iv VARCHAR, encrypted_api_url VARCHAR, encrypted_api_url_iv VARCHAR, encrypted_username VARCHAR, encrypted_username_iv VARCHAR, encrypted_password VARCHAR, encrypted_password_iv VARCHAR, jira_issue_transition_id VARCHAR);

CREATE TABLE public.keys (id INT NOT NULL, user_id INT, created_at TIMESTAMP, updated_at TIMESTAMP, key TEXT, title VARCHAR, type VARCHAR, fingerprint VARCHAR, public BOOLEAN DEFAULT FALSE NOT NULL, last_used_at TIMESTAMP, fingerprint_sha256 BYTEA, expires_at TIMESTAMPTZ);

CREATE TABLE public.label_links (id INT NOT NULL, label_id INT, target_id INT, target_type VARCHAR, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.label_priorities (id INT NOT NULL, project_id INT NOT NULL, label_id INT NOT NULL, priority INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.labels (id INT NOT NULL, title VARCHAR, color VARCHAR, project_id INT, created_at TIMESTAMP, updated_at TIMESTAMP, template BOOLEAN DEFAULT FALSE, description VARCHAR, description_html TEXT, type VARCHAR, group_id INT, cached_markdown_version INT);

CREATE TABLE public.ldap_group_links (id INT NOT NULL, cn VARCHAR, group_access INT NOT NULL, group_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, provider VARCHAR, filter VARCHAR);

CREATE TABLE public.lfs_file_locks (id INT NOT NULL, project_id INT NOT NULL, user_id INT NOT NULL, created_at TIMESTAMP NOT NULL, path VARCHAR(511));

CREATE TABLE public.lfs_objects (id INT NOT NULL, oid VARCHAR NOT NULL, size BIGINT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, file VARCHAR, file_store INT);

CREATE TABLE public.lfs_objects_projects (id INT NOT NULL, lfs_object_id INT NOT NULL, project_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, repository_type SMALLINT);

CREATE TABLE public.licenses (id INT NOT NULL, data TEXT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.list_user_preferences (id BIGINT NOT NULL, user_id BIGINT NOT NULL, list_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, collapsed BOOLEAN);

CREATE TABLE public.lists (id INT NOT NULL, board_id INT NOT NULL, label_id INT, list_type INT DEFAULT 1 NOT NULL, "position" INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, user_id INT, milestone_id INT, max_issue_count INT DEFAULT 0 NOT NULL, max_issue_weight INT DEFAULT 0 NOT NULL, limit_metric VARCHAR(20));

CREATE TABLE public.members (id INT NOT NULL, access_level INT NOT NULL, source_id INT NOT NULL, source_type VARCHAR NOT NULL, user_id INT, notification_level INT NOT NULL, type VARCHAR, created_at TIMESTAMP, updated_at TIMESTAMP, created_by_id INT, invite_email VARCHAR, invite_token VARCHAR, invite_accepted_at TIMESTAMP, requested_at TIMESTAMP, expires_at DATE, ldap BOOLEAN DEFAULT FALSE NOT NULL, override BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.merge_request_assignees (id BIGINT NOT NULL, user_id INT NOT NULL, merge_request_id INT NOT NULL, created_at TIMESTAMPTZ);

CREATE TABLE public.merge_request_blocks (id BIGINT NOT NULL, blocking_merge_request_id INT NOT NULL, blocked_merge_request_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.merge_request_context_commit_diff_files (sha BYTEA NOT NULL, relative_order INT NOT NULL, new_file BOOLEAN NOT NULL, renamed_file BOOLEAN NOT NULL, deleted_file BOOLEAN NOT NULL, too_large BOOLEAN NOT NULL, a_mode VARCHAR(255) NOT NULL, b_mode VARCHAR(255) NOT NULL, new_path TEXT NOT NULL, old_path TEXT NOT NULL, diff TEXT, "binary" BOOLEAN, merge_request_context_commit_id BIGINT);

CREATE TABLE public.merge_request_context_commits (id BIGINT NOT NULL, authored_date TIMESTAMPTZ, committed_date TIMESTAMPTZ, relative_order INT NOT NULL, sha BYTEA NOT NULL, author_name TEXT, author_email TEXT, committer_name TEXT, committer_email TEXT, message TEXT, merge_request_id BIGINT);

CREATE TABLE public.merge_request_diff_commits (authored_date TIMESTAMP, committed_date TIMESTAMP, merge_request_diff_id INT NOT NULL, relative_order INT NOT NULL, sha BYTEA NOT NULL, author_name TEXT, author_email TEXT, committer_name TEXT, committer_email TEXT, message TEXT);

CREATE TABLE public.merge_request_diff_files (merge_request_diff_id INT NOT NULL, relative_order INT NOT NULL, new_file BOOLEAN NOT NULL, renamed_file BOOLEAN NOT NULL, deleted_file BOOLEAN NOT NULL, too_large BOOLEAN NOT NULL, a_mode VARCHAR NOT NULL, b_mode VARCHAR NOT NULL, new_path TEXT NOT NULL, old_path TEXT NOT NULL, diff TEXT, "binary" BOOLEAN, external_diff_offset INT, external_diff_size INT);

CREATE TABLE public.merge_request_diffs (id INT NOT NULL, state VARCHAR, merge_request_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, base_commit_sha VARCHAR, real_size VARCHAR, head_commit_sha VARCHAR, start_commit_sha VARCHAR, commits_count INT, external_diff VARCHAR, external_diff_store INT, stored_externally BOOLEAN);

CREATE TABLE public.merge_request_metrics (id INT NOT NULL, merge_request_id INT NOT NULL, latest_build_started_at TIMESTAMP, latest_build_finished_at TIMESTAMP, first_deployed_to_production_at TIMESTAMP, merged_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, pipeline_id INT, merged_by_id INT, latest_closed_by_id INT, latest_closed_at TIMESTAMPTZ, first_comment_at TIMESTAMPTZ, first_commit_at TIMESTAMPTZ, last_commit_at TIMESTAMPTZ, diff_size INT, modified_paths_size INT, commits_count INT, first_approved_at TIMESTAMPTZ, first_reassigned_at TIMESTAMPTZ);

CREATE TABLE public.merge_request_user_mentions (id BIGINT NOT NULL, merge_request_id INT NOT NULL, note_id INT, mentioned_users_ids INT[], mentioned_projects_ids INT[], mentioned_groups_ids INT[]);

CREATE TABLE public.merge_requests (id INT NOT NULL, target_branch VARCHAR NOT NULL, source_branch VARCHAR NOT NULL, source_project_id INT, author_id INT, assignee_id INT, title VARCHAR, created_at TIMESTAMP, updated_at TIMESTAMP, milestone_id INT, merge_status VARCHAR DEFAULT CAST('unchecked' AS VARCHAR) NOT NULL, target_project_id INT NOT NULL, iid INT, description TEXT, updated_by_id INT, merge_error TEXT, merge_params TEXT, merge_when_pipeline_succeeds BOOLEAN DEFAULT FALSE NOT NULL, merge_user_id INT, merge_commit_sha VARCHAR, approvals_before_merge INT, rebase_commit_sha VARCHAR, in_progress_merge_commit_sha VARCHAR, lock_version INT DEFAULT 0, title_html TEXT, description_html TEXT, time_estimate INT, squash BOOLEAN DEFAULT FALSE NOT NULL, cached_markdown_version INT, last_edited_at TIMESTAMP, last_edited_by_id INT, head_pipeline_id INT, merge_jid VARCHAR, discussion_locked BOOLEAN, latest_merge_request_diff_id INT, allow_maintainer_to_push BOOLEAN, state_id SMALLINT DEFAULT 1 NOT NULL, rebase_jid VARCHAR, squash_commit_sha BYTEA);

CREATE TABLE public.merge_requests_closing_issues (id INT NOT NULL, merge_request_id INT NOT NULL, issue_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.merge_trains (id BIGINT NOT NULL, merge_request_id INT NOT NULL, user_id INT NOT NULL, pipeline_id INT, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, target_project_id INT NOT NULL, target_branch TEXT NOT NULL, status SMALLINT DEFAULT 0 NOT NULL, merged_at TIMESTAMPTZ, duration INT);

CREATE TABLE public.milestone_releases (milestone_id BIGINT NOT NULL, release_id BIGINT NOT NULL);

CREATE TABLE public.milestones (id INT NOT NULL, title VARCHAR NOT NULL, project_id INT, description TEXT, due_date DATE, created_at TIMESTAMP, updated_at TIMESTAMP, state VARCHAR, iid INT, title_html TEXT, description_html TEXT, start_date DATE, cached_markdown_version INT, group_id INT);

CREATE TABLE public.namespace_aggregation_schedules (namespace_id INT NOT NULL);

CREATE TABLE public.namespace_root_storage_statistics (namespace_id INT NOT NULL, updated_at TIMESTAMPTZ NOT NULL, repository_size BIGINT DEFAULT 0 NOT NULL, lfs_objects_size BIGINT DEFAULT 0 NOT NULL, wiki_size BIGINT DEFAULT 0 NOT NULL, build_artifacts_size BIGINT DEFAULT 0 NOT NULL, storage_size BIGINT DEFAULT 0 NOT NULL, packages_size BIGINT DEFAULT 0 NOT NULL);

CREATE TABLE public.namespace_statistics (id INT NOT NULL, namespace_id INT NOT NULL, shared_runners_seconds INT DEFAULT 0 NOT NULL, shared_runners_seconds_last_reset TIMESTAMP);

CREATE TABLE public.namespaces (id INT NOT NULL, name VARCHAR NOT NULL, path VARCHAR NOT NULL, owner_id INT, created_at TIMESTAMP, updated_at TIMESTAMP, type VARCHAR, description VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, avatar VARCHAR, membership_lock BOOLEAN DEFAULT FALSE, share_with_group_lock BOOLEAN DEFAULT FALSE, visibility_level INT DEFAULT 20 NOT NULL, request_access_enabled BOOLEAN DEFAULT TRUE NOT NULL, ldap_sync_status VARCHAR DEFAULT CAST('ready' AS VARCHAR) NOT NULL, ldap_sync_error VARCHAR, ldap_sync_last_update_at TIMESTAMP, ldap_sync_last_successful_update_at TIMESTAMP, ldap_sync_last_sync_at TIMESTAMP, description_html TEXT, lfs_enabled BOOLEAN, parent_id INT, shared_runners_minutes_limit INT, repository_size_limit BIGINT, require_two_factor_authentication BOOLEAN DEFAULT FALSE NOT NULL, two_factor_grace_period INT DEFAULT 48 NOT NULL, cached_markdown_version INT, plan_id INT, project_creation_level INT, runners_token VARCHAR, trial_ends_on TIMESTAMPTZ, file_template_project_id INT, saml_discovery_token VARCHAR, runners_token_encrypted VARCHAR, custom_project_templates_group_id INT, auto_devops_enabled BOOLEAN, extra_shared_runners_minutes_limit INT, last_ci_minutes_notification_at TIMESTAMPTZ, last_ci_minutes_usage_notification_level INT, subgroup_creation_level INT DEFAULT 1, emails_disabled BOOLEAN, max_pages_size INT, max_artifacts_size INT, mentions_disabled BOOLEAN, default_branch_protection SMALLINT, unlock_membership_to_ldap BOOLEAN, max_personal_access_token_lifetime INT);

CREATE TABLE public.note_diff_files (id INT NOT NULL, diff_note_id INT NOT NULL, diff TEXT NOT NULL, new_file BOOLEAN NOT NULL, renamed_file BOOLEAN NOT NULL, deleted_file BOOLEAN NOT NULL, a_mode VARCHAR NOT NULL, b_mode VARCHAR NOT NULL, new_path TEXT NOT NULL, old_path TEXT NOT NULL);

CREATE TABLE public.notes (id INT NOT NULL, note TEXT, noteable_type VARCHAR, author_id INT, created_at TIMESTAMP, updated_at TIMESTAMP, project_id INT, attachment VARCHAR, line_code VARCHAR, commit_id VARCHAR, noteable_id INT, system BOOLEAN DEFAULT FALSE NOT NULL, st_diff TEXT, updated_by_id INT, type VARCHAR, "position" TEXT, original_position TEXT, resolved_at TIMESTAMP, resolved_by_id INT, discussion_id VARCHAR, note_html TEXT, cached_markdown_version INT, change_position TEXT, resolved_by_push BOOLEAN, review_id BIGINT, confidential BOOLEAN);

CREATE TABLE public.notification_settings (id INT NOT NULL, user_id INT NOT NULL, source_id INT, source_type VARCHAR, level INT DEFAULT 0 NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, new_note BOOLEAN, new_issue BOOLEAN, reopen_issue BOOLEAN, close_issue BOOLEAN, reassign_issue BOOLEAN, new_merge_request BOOLEAN, reopen_merge_request BOOLEAN, close_merge_request BOOLEAN, reassign_merge_request BOOLEAN, merge_merge_request BOOLEAN, failed_pipeline BOOLEAN, success_pipeline BOOLEAN, push_to_merge_request BOOLEAN, issue_due BOOLEAN, new_epic BOOLEAN, notification_email VARCHAR, fixed_pipeline BOOLEAN, new_release BOOLEAN);

CREATE TABLE public.oauth_access_grants (id INT NOT NULL, resource_owner_id INT NOT NULL, application_id INT NOT NULL, token VARCHAR NOT NULL, expires_in INT NOT NULL, redirect_uri TEXT NOT NULL, created_at TIMESTAMP NOT NULL, revoked_at TIMESTAMP, scopes VARCHAR);

CREATE TABLE public.oauth_access_tokens (id INT NOT NULL, resource_owner_id INT, application_id INT, token VARCHAR NOT NULL, refresh_token VARCHAR, expires_in INT, revoked_at TIMESTAMP, created_at TIMESTAMP NOT NULL, scopes VARCHAR);

CREATE TABLE public.oauth_applications (id INT NOT NULL, name VARCHAR NOT NULL, uid VARCHAR NOT NULL, secret VARCHAR NOT NULL, redirect_uri TEXT NOT NULL, scopes VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, owner_id INT, owner_type VARCHAR, trusted BOOLEAN DEFAULT FALSE NOT NULL, confidential BOOLEAN DEFAULT TRUE NOT NULL);

CREATE TABLE public.oauth_openid_requests (id INT NOT NULL, access_grant_id INT NOT NULL, nonce VARCHAR NOT NULL);

CREATE TABLE public.open_project_tracker_data (id BIGINT NOT NULL, service_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, encrypted_url VARCHAR(255), encrypted_url_iv VARCHAR(255), encrypted_api_url VARCHAR(255), encrypted_api_url_iv VARCHAR(255), encrypted_token VARCHAR(255), encrypted_token_iv VARCHAR(255), closed_status_id VARCHAR(5), project_identifier_code VARCHAR(100));

CREATE TABLE public.operations_feature_flag_scopes (id BIGINT NOT NULL, feature_flag_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, active BOOLEAN NOT NULL, environment_scope VARCHAR DEFAULT CAST('*' AS VARCHAR) NOT NULL, strategies JSONB DEFAULT CAST('[{"name": "default", "parameters": {}}]' AS JSONB) NOT NULL);

CREATE TABLE public.operations_feature_flags (id BIGINT NOT NULL, project_id INT NOT NULL, active BOOLEAN NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, name VARCHAR NOT NULL, description TEXT, iid INT NOT NULL, version SMALLINT DEFAULT 1 NOT NULL);

CREATE TABLE public.operations_feature_flags_clients (id BIGINT NOT NULL, project_id INT NOT NULL, token_encrypted VARCHAR);

CREATE TABLE public.operations_scopes (id BIGINT NOT NULL, strategy_id BIGINT NOT NULL, environment_scope VARCHAR(255) NOT NULL);

CREATE TABLE public.operations_strategies (id BIGINT NOT NULL, feature_flag_id BIGINT NOT NULL, name VARCHAR(255) NOT NULL, parameters JSONB DEFAULT CAST('{}' AS JSONB) NOT NULL);

CREATE TABLE public.packages_build_infos (id BIGINT NOT NULL, package_id INT NOT NULL, pipeline_id INT);

CREATE TABLE public.packages_conan_file_metadata (id BIGINT NOT NULL, package_file_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, recipe_revision VARCHAR(255) DEFAULT CAST('0' AS VARCHAR) NOT NULL, package_revision VARCHAR(255), conan_package_reference VARCHAR(255), conan_file_type SMALLINT NOT NULL);

CREATE TABLE public.packages_conan_metadata (id BIGINT NOT NULL, package_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, package_username VARCHAR(255) NOT NULL, package_channel VARCHAR(255) NOT NULL);

CREATE TABLE public.packages_dependencies (id BIGINT NOT NULL, name VARCHAR(255) NOT NULL, version_pattern VARCHAR(255) NOT NULL);

CREATE TABLE public.packages_dependency_links (id BIGINT NOT NULL, package_id BIGINT NOT NULL, dependency_id BIGINT NOT NULL, dependency_type SMALLINT NOT NULL);

CREATE TABLE public.packages_maven_metadata (id BIGINT NOT NULL, package_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, app_group VARCHAR NOT NULL, app_name VARCHAR NOT NULL, app_version VARCHAR, path VARCHAR(512) NOT NULL);

CREATE TABLE public.packages_package_files (id BIGINT NOT NULL, package_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, size BIGINT, file_store INT, file_md5 BYTEA, file_sha1 BYTEA, file_name VARCHAR NOT NULL, file TEXT NOT NULL, file_sha256 BYTEA);

CREATE TABLE public.packages_packages (id BIGINT NOT NULL, project_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, name VARCHAR NOT NULL, version VARCHAR, package_type SMALLINT NOT NULL);

CREATE TABLE public.packages_tags (id BIGINT NOT NULL, package_id INT NOT NULL, name VARCHAR(255) NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.pages_domain_acme_orders (id BIGINT NOT NULL, pages_domain_id INT NOT NULL, expires_at TIMESTAMPTZ NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, url VARCHAR NOT NULL, challenge_token VARCHAR NOT NULL, challenge_file_content TEXT NOT NULL, encrypted_private_key TEXT NOT NULL, encrypted_private_key_iv TEXT NOT NULL);

CREATE TABLE public.pages_domains (id INT NOT NULL, project_id INT, certificate TEXT, encrypted_key TEXT, encrypted_key_iv VARCHAR, encrypted_key_salt VARCHAR, domain VARCHAR, verified_at TIMESTAMPTZ, verification_code VARCHAR NOT NULL, enabled_until TIMESTAMPTZ, remove_at TIMESTAMPTZ, auto_ssl_enabled BOOLEAN DEFAULT FALSE NOT NULL, certificate_valid_not_before TIMESTAMPTZ, certificate_valid_not_after TIMESTAMPTZ, certificate_source SMALLINT DEFAULT 0 NOT NULL, wildcard BOOLEAN DEFAULT FALSE NOT NULL, usage SMALLINT DEFAULT 0 NOT NULL, scope SMALLINT DEFAULT 2 NOT NULL, auto_ssl_failed BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.path_locks (id INT NOT NULL, path VARCHAR NOT NULL, project_id INT, user_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.personal_access_tokens (id INT NOT NULL, user_id INT NOT NULL, name VARCHAR NOT NULL, revoked BOOLEAN DEFAULT FALSE, expires_at DATE, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, scopes VARCHAR DEFAULT CAST('--- []' AS VARCHAR) NOT NULL, impersonation BOOLEAN DEFAULT FALSE NOT NULL, token_digest VARCHAR, expire_notification_delivered BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.plan_limits (id BIGINT NOT NULL, plan_id BIGINT NOT NULL, ci_active_pipelines INT DEFAULT 0 NOT NULL, ci_pipeline_size INT DEFAULT 0 NOT NULL, ci_active_jobs INT DEFAULT 0 NOT NULL, project_hooks INT DEFAULT 100 NOT NULL, group_hooks INT DEFAULT 50 NOT NULL, ci_project_subscriptions INT DEFAULT 2 NOT NULL, ci_pipeline_schedules INT DEFAULT 10 NOT NULL);

CREATE TABLE public.plans (id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, name VARCHAR, title VARCHAR);

CREATE TABLE public.pool_repositories (id BIGINT NOT NULL, shard_id INT NOT NULL, disk_path VARCHAR, state VARCHAR, source_project_id INT);

CREATE TABLE public.programming_languages (id INT NOT NULL, name VARCHAR NOT NULL, color VARCHAR NOT NULL, created_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.project_alerting_settings (project_id INT NOT NULL, encrypted_token VARCHAR NOT NULL, encrypted_token_iv VARCHAR NOT NULL);

CREATE TABLE public.project_aliases (id BIGINT NOT NULL, project_id INT NOT NULL, name VARCHAR NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.project_authorizations (user_id INT NOT NULL, project_id INT NOT NULL, access_level INT NOT NULL);

CREATE TABLE public.project_auto_devops (id INT NOT NULL, project_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, enabled BOOLEAN, deploy_strategy INT DEFAULT 0 NOT NULL);

CREATE TABLE public.project_ci_cd_settings (id INT NOT NULL, project_id INT NOT NULL, group_runners_enabled BOOLEAN DEFAULT TRUE NOT NULL, merge_pipelines_enabled BOOLEAN, default_git_depth INT, forward_deployment_enabled BOOLEAN);

CREATE TABLE public.project_custom_attributes (id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id INT NOT NULL, key VARCHAR NOT NULL, value VARCHAR NOT NULL);

CREATE TABLE public.project_daily_statistics (id BIGINT NOT NULL, project_id INT NOT NULL, fetch_count INT NOT NULL, date DATE);

CREATE TABLE public.project_deploy_tokens (id INT NOT NULL, project_id INT NOT NULL, deploy_token_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.project_error_tracking_settings (project_id INT NOT NULL, enabled BOOLEAN DEFAULT FALSE NOT NULL, api_url VARCHAR, encrypted_token VARCHAR, encrypted_token_iv VARCHAR, project_name VARCHAR, organization_name VARCHAR);

CREATE TABLE public.project_export_jobs (id BIGINT NOT NULL, project_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, status SMALLINT DEFAULT 0 NOT NULL, jid VARCHAR(100) NOT NULL);

CREATE TABLE public.project_feature_usages (project_id INT NOT NULL, jira_dvcs_cloud_last_sync_at TIMESTAMP, jira_dvcs_server_last_sync_at TIMESTAMP);

CREATE TABLE public.project_features (id INT NOT NULL, project_id INT NOT NULL, merge_requests_access_level INT, issues_access_level INT, wiki_access_level INT, snippets_access_level INT DEFAULT 20 NOT NULL, builds_access_level INT, created_at TIMESTAMP, updated_at TIMESTAMP, repository_access_level INT DEFAULT 20 NOT NULL, pages_access_level INT NOT NULL, forking_access_level INT);

CREATE TABLE public.project_group_links (id INT NOT NULL, project_id INT NOT NULL, group_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, group_access INT DEFAULT 30 NOT NULL, expires_at DATE);

CREATE TABLE public.project_import_data (id INT NOT NULL, project_id INT, data TEXT, encrypted_credentials TEXT, encrypted_credentials_iv VARCHAR, encrypted_credentials_salt VARCHAR);

CREATE TABLE public.project_incident_management_settings (project_id INT NOT NULL, create_issue BOOLEAN DEFAULT TRUE NOT NULL, send_email BOOLEAN DEFAULT FALSE NOT NULL, issue_template_key TEXT);

CREATE TABLE public.project_metrics_settings (project_id INT NOT NULL, external_dashboard_url VARCHAR NOT NULL);

CREATE TABLE public.project_mirror_data (id INT NOT NULL, project_id INT NOT NULL, retry_count INT DEFAULT 0 NOT NULL, last_update_started_at TIMESTAMP, last_update_scheduled_at TIMESTAMP, next_execution_timestamp TIMESTAMP, status VARCHAR, jid VARCHAR, last_error TEXT, last_update_at TIMESTAMPTZ, last_successful_update_at TIMESTAMPTZ);

CREATE TABLE public.project_pages_metadata (project_id BIGINT NOT NULL, deployed BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.project_repositories (id BIGINT NOT NULL, shard_id INT NOT NULL, disk_path VARCHAR NOT NULL, project_id INT NOT NULL);

CREATE TABLE public.project_repository_states (id INT NOT NULL, project_id INT NOT NULL, repository_verification_checksum BYTEA, wiki_verification_checksum BYTEA, last_repository_verification_failure VARCHAR, last_wiki_verification_failure VARCHAR, repository_retry_at TIMESTAMPTZ, wiki_retry_at TIMESTAMPTZ, repository_retry_count INT, wiki_retry_count INT, last_repository_verification_ran_at TIMESTAMPTZ, last_wiki_verification_ran_at TIMESTAMPTZ);

CREATE TABLE public.project_settings (project_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.project_statistics (id INT NOT NULL, project_id INT NOT NULL, namespace_id INT NOT NULL, commit_count BIGINT DEFAULT 0 NOT NULL, storage_size BIGINT DEFAULT 0 NOT NULL, repository_size BIGINT DEFAULT 0 NOT NULL, lfs_objects_size BIGINT DEFAULT 0 NOT NULL, build_artifacts_size BIGINT DEFAULT 0 NOT NULL, shared_runners_seconds BIGINT DEFAULT 0 NOT NULL, shared_runners_seconds_last_reset TIMESTAMP, packages_size BIGINT DEFAULT 0 NOT NULL, wiki_size BIGINT);

CREATE TABLE public.project_tracing_settings (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id INT NOT NULL, external_url VARCHAR NOT NULL);

CREATE TABLE public.projects (id INT NOT NULL, name VARCHAR, path VARCHAR, description TEXT, created_at TIMESTAMP, updated_at TIMESTAMP, creator_id INT, namespace_id INT NOT NULL, last_activity_at TIMESTAMP, import_url VARCHAR, visibility_level INT DEFAULT 0 NOT NULL, archived BOOLEAN DEFAULT FALSE NOT NULL, avatar VARCHAR, merge_requests_template TEXT, star_count INT DEFAULT 0 NOT NULL, merge_requests_rebase_enabled BOOLEAN DEFAULT FALSE, import_type VARCHAR, import_source VARCHAR, approvals_before_merge INT DEFAULT 0 NOT NULL, reset_approvals_on_push BOOLEAN DEFAULT TRUE, merge_requests_ff_only_enabled BOOLEAN DEFAULT FALSE, issues_template TEXT, mirror BOOLEAN DEFAULT FALSE NOT NULL, mirror_last_update_at TIMESTAMP, mirror_last_successful_update_at TIMESTAMP, mirror_user_id INT, shared_runners_enabled BOOLEAN DEFAULT TRUE NOT NULL, runners_token VARCHAR, build_coverage_regex VARCHAR, build_allow_git_fetch BOOLEAN DEFAULT TRUE NOT NULL, build_timeout INT DEFAULT 3600 NOT NULL, mirror_trigger_builds BOOLEAN DEFAULT FALSE NOT NULL, pending_delete BOOLEAN DEFAULT FALSE, public_builds BOOLEAN DEFAULT TRUE NOT NULL, last_repository_check_failed BOOLEAN, last_repository_check_at TIMESTAMP, container_registry_enabled BOOLEAN, only_allow_merge_if_pipeline_succeeds BOOLEAN DEFAULT FALSE NOT NULL, has_external_issue_tracker BOOLEAN, repository_storage VARCHAR DEFAULT CAST('default' AS VARCHAR) NOT NULL, repository_read_only BOOLEAN, request_access_enabled BOOLEAN DEFAULT TRUE NOT NULL, has_external_wiki BOOLEAN, ci_config_path VARCHAR, lfs_enabled BOOLEAN, description_html TEXT, only_allow_merge_if_all_discussions_are_resolved BOOLEAN, repository_size_limit BIGINT, printing_merge_request_link_enabled BOOLEAN DEFAULT TRUE NOT NULL, auto_cancel_pending_pipelines INT DEFAULT 1 NOT NULL, service_desk_enabled BOOLEAN DEFAULT TRUE, cached_markdown_version INT, delete_error TEXT, last_repository_updated_at TIMESTAMP, disable_overriding_approvers_per_merge_request BOOLEAN, storage_version SMALLINT, resolve_outdated_diff_discussions BOOLEAN, remote_mirror_available_overridden BOOLEAN, only_mirror_protected_branches BOOLEAN, pull_mirror_available_overridden BOOLEAN, jobs_cache_index INT, external_authorization_classification_label VARCHAR, mirror_overwrites_diverged_branches BOOLEAN, pages_https_only BOOLEAN DEFAULT TRUE, external_webhook_token VARCHAR, packages_enabled BOOLEAN, merge_requests_author_approval BOOLEAN, pool_repository_id BIGINT, runners_token_encrypted VARCHAR, bfg_object_map VARCHAR, detected_repository_languages BOOLEAN, merge_requests_disable_committers_approval BOOLEAN, require_password_to_approve BOOLEAN, emails_disabled BOOLEAN, max_pages_size INT, max_artifacts_size INT, pull_mirror_branch_prefix VARCHAR(50), remove_source_branch_after_merge BOOLEAN, marked_for_deletion_at DATE, marked_for_deletion_by_user_id INT, autoclose_referenced_issues BOOLEAN, suggestion_commit_message VARCHAR(255));

CREATE TABLE public.prometheus_alert_events (id BIGINT NOT NULL, project_id INT NOT NULL, prometheus_alert_id INT NOT NULL, started_at TIMESTAMPTZ NOT NULL, ended_at TIMESTAMPTZ, status SMALLINT, payload_key VARCHAR);

CREATE TABLE public.prometheus_alerts (id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, threshold DOUBLE PRECISION NOT NULL, operator INT NOT NULL, environment_id INT NOT NULL, project_id INT NOT NULL, prometheus_metric_id INT NOT NULL);

CREATE TABLE public.prometheus_metrics (id INT NOT NULL, project_id INT, title VARCHAR NOT NULL, query VARCHAR NOT NULL, y_label VARCHAR NOT NULL, unit VARCHAR NOT NULL, legend VARCHAR, "congruentClass" INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, common BOOLEAN DEFAULT FALSE NOT NULL, identifier VARCHAR);

CREATE TABLE public.protected_branch_merge_access_levels (id INT NOT NULL, protected_branch_id INT NOT NULL, access_level INT DEFAULT 40, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, user_id INT, group_id INT);

CREATE TABLE public.protected_branch_push_access_levels (id INT NOT NULL, protected_branch_id INT NOT NULL, access_level INT DEFAULT 40, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, user_id INT, group_id INT);

CREATE TABLE public.protected_branch_unprotect_access_levels (id INT NOT NULL, protected_branch_id INT NOT NULL, access_level INT DEFAULT 40, user_id INT, group_id INT);

CREATE TABLE public.protected_branches (id INT NOT NULL, project_id INT NOT NULL, name VARCHAR NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, code_owner_approval_required BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.protected_environment_deploy_access_levels (id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, access_level INT DEFAULT 40, protected_environment_id INT NOT NULL, user_id INT, group_id INT);

CREATE TABLE public.protected_environments (id INT NOT NULL, project_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, name VARCHAR NOT NULL);

CREATE TABLE public.protected_tag_create_access_levels (id INT NOT NULL, protected_tag_id INT NOT NULL, access_level INT DEFAULT 40, user_id INT, group_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.protected_tags (id INT NOT NULL, project_id INT NOT NULL, name VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.push_event_payloads (commit_count BIGINT NOT NULL, event_id INT NOT NULL, action SMALLINT NOT NULL, ref_type SMALLINT NOT NULL, commit_from BYTEA, commit_to BYTEA, ref TEXT, commit_title VARCHAR(70), ref_count INT);

CREATE TABLE public.push_rules (id INT NOT NULL, force_push_regex VARCHAR, delete_branch_regex VARCHAR, commit_message_regex VARCHAR, deny_delete_tag BOOLEAN, project_id INT, created_at TIMESTAMP, updated_at TIMESTAMP, author_email_regex VARCHAR, member_check BOOLEAN DEFAULT FALSE NOT NULL, file_name_regex VARCHAR, is_sample BOOLEAN DEFAULT FALSE, max_file_size INT DEFAULT 0 NOT NULL, prevent_secrets BOOLEAN DEFAULT FALSE NOT NULL, branch_name_regex VARCHAR, reject_unsigned_commits BOOLEAN, commit_committer_check BOOLEAN, regexp_uses_re2 BOOLEAN DEFAULT TRUE, commit_message_negative_regex VARCHAR);

CREATE TABLE public.redirect_routes (id INT NOT NULL, source_id INT NOT NULL, source_type VARCHAR NOT NULL, path VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.release_links (id BIGINT NOT NULL, release_id INT NOT NULL, url VARCHAR NOT NULL, name VARCHAR NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, filepath VARCHAR(128));

CREATE TABLE public.releases (id INT NOT NULL, tag VARCHAR, description TEXT, project_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, description_html TEXT, cached_markdown_version INT, author_id INT, name VARCHAR, sha VARCHAR, released_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.remote_mirrors (id INT NOT NULL, project_id INT, url VARCHAR, enabled BOOLEAN DEFAULT FALSE, update_status VARCHAR, last_update_at TIMESTAMP, last_successful_update_at TIMESTAMP, last_error VARCHAR, encrypted_credentials TEXT, encrypted_credentials_iv VARCHAR, encrypted_credentials_salt VARCHAR, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, last_update_started_at TIMESTAMP, only_protected_branches BOOLEAN DEFAULT FALSE NOT NULL, remote_name VARCHAR, error_notification_sent BOOLEAN, keep_divergent_refs BOOLEAN);

CREATE TABLE public.repository_languages (project_id INT NOT NULL, programming_language_id INT NOT NULL, "share" DOUBLE PRECISION NOT NULL);

CREATE TABLE public.requirements (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id INT NOT NULL, author_id INT, iid INT NOT NULL, cached_markdown_version INT, state SMALLINT DEFAULT 1 NOT NULL, title VARCHAR(255) NOT NULL, title_html TEXT);

CREATE TABLE public.resource_label_events (id BIGINT NOT NULL, action INT NOT NULL, issue_id INT, merge_request_id INT, epic_id INT, label_id INT, user_id INT, created_at TIMESTAMPTZ NOT NULL, cached_markdown_version INT, reference TEXT, reference_html TEXT);

CREATE TABLE public.resource_milestone_events (id BIGINT NOT NULL, user_id BIGINT NOT NULL, issue_id BIGINT, merge_request_id BIGINT, milestone_id BIGINT, action SMALLINT NOT NULL, state SMALLINT NOT NULL, cached_markdown_version INT, reference TEXT, reference_html TEXT, created_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.resource_weight_events (id BIGINT NOT NULL, user_id BIGINT NOT NULL, issue_id BIGINT NOT NULL, weight INT, created_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.reviews (id BIGINT NOT NULL, author_id INT, merge_request_id INT NOT NULL, project_id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.routes (id INT NOT NULL, source_id INT NOT NULL, source_type VARCHAR NOT NULL, path VARCHAR NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, name VARCHAR);

CREATE TABLE public.saml_providers (id INT NOT NULL, group_id INT NOT NULL, enabled BOOLEAN NOT NULL, certificate_fingerprint VARCHAR NOT NULL, sso_url VARCHAR NOT NULL, enforced_sso BOOLEAN DEFAULT FALSE NOT NULL, enforced_group_managed_accounts BOOLEAN DEFAULT FALSE NOT NULL, prohibited_outer_forks BOOLEAN DEFAULT TRUE NOT NULL);

CREATE TABLE public.schema_migrations (version VARCHAR NOT NULL);

CREATE TABLE public.scim_identities (id BIGINT NOT NULL, group_id BIGINT NOT NULL, user_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, active BOOLEAN DEFAULT FALSE, extern_uid VARCHAR(255) NOT NULL);

CREATE TABLE public.scim_oauth_access_tokens (id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, group_id INT NOT NULL, token_encrypted VARCHAR NOT NULL);

CREATE TABLE public.security_scans (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, build_id BIGINT NOT NULL, scan_type SMALLINT NOT NULL);

CREATE TABLE public.self_managed_prometheus_alert_events (id BIGINT NOT NULL, project_id BIGINT NOT NULL, environment_id BIGINT, started_at TIMESTAMPTZ NOT NULL, ended_at TIMESTAMPTZ, status SMALLINT NOT NULL, title VARCHAR(255) NOT NULL, query_expression VARCHAR(255), payload_key VARCHAR(255) NOT NULL);

CREATE TABLE public.sent_notifications (id INT NOT NULL, project_id INT, noteable_id INT, noteable_type VARCHAR, recipient_id INT, commit_id VARCHAR, reply_key VARCHAR NOT NULL, line_code VARCHAR, note_type VARCHAR, "position" TEXT, in_reply_to_discussion_id VARCHAR);

CREATE TABLE public.sentry_issues (id BIGINT NOT NULL, issue_id BIGINT NOT NULL, sentry_issue_identifier BIGINT NOT NULL);

CREATE TABLE public.serverless_domain_cluster (uuid VARCHAR(14) NOT NULL, pages_domain_id BIGINT NOT NULL, clusters_applications_knative_id BIGINT NOT NULL, creator_id BIGINT, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, encrypted_key TEXT, encrypted_key_iv VARCHAR(255), certificate TEXT);

CREATE TABLE public.service_desk_settings (project_id BIGINT NOT NULL, issue_template_key VARCHAR(255), outgoing_name VARCHAR(255), project_key VARCHAR(255));

CREATE TABLE public.services (id INT NOT NULL, type VARCHAR, title VARCHAR, project_id INT, created_at TIMESTAMP, updated_at TIMESTAMP, active BOOLEAN DEFAULT FALSE NOT NULL, properties TEXT, push_events BOOLEAN DEFAULT TRUE, issues_events BOOLEAN DEFAULT TRUE, merge_requests_events BOOLEAN DEFAULT TRUE, tag_push_events BOOLEAN DEFAULT TRUE, note_events BOOLEAN DEFAULT TRUE NOT NULL, category VARCHAR DEFAULT CAST('common' AS VARCHAR) NOT NULL, "default" BOOLEAN DEFAULT FALSE, wiki_page_events BOOLEAN DEFAULT TRUE, pipeline_events BOOLEAN DEFAULT FALSE NOT NULL, confidential_issues_events BOOLEAN DEFAULT TRUE NOT NULL, commit_events BOOLEAN DEFAULT TRUE NOT NULL, job_events BOOLEAN DEFAULT FALSE NOT NULL, confidential_note_events BOOLEAN DEFAULT TRUE, deployment_events BOOLEAN DEFAULT FALSE NOT NULL, description VARCHAR(500), comment_on_event_enabled BOOLEAN DEFAULT TRUE NOT NULL, template BOOLEAN DEFAULT FALSE, instance BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.shards (id INT NOT NULL, name VARCHAR NOT NULL);

CREATE TABLE public.slack_integrations (id INT NOT NULL, service_id INT NOT NULL, team_id VARCHAR NOT NULL, team_name VARCHAR NOT NULL, alias VARCHAR NOT NULL, user_id VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.smartcard_identities (id BIGINT NOT NULL, user_id INT NOT NULL, subject VARCHAR NOT NULL, issuer VARCHAR NOT NULL);

CREATE TABLE public.snippet_repositories (snippet_id BIGINT NOT NULL, shard_id BIGINT NOT NULL, disk_path VARCHAR(80) NOT NULL);

CREATE TABLE public.snippet_user_mentions (id BIGINT NOT NULL, snippet_id INT NOT NULL, note_id INT, mentioned_users_ids INT[], mentioned_projects_ids INT[], mentioned_groups_ids INT[]);

CREATE TABLE public.snippets (id INT NOT NULL, title VARCHAR, content TEXT, author_id INT NOT NULL, project_id INT, created_at TIMESTAMP, updated_at TIMESTAMP, file_name VARCHAR, type VARCHAR, visibility_level INT DEFAULT 0 NOT NULL, title_html TEXT, content_html TEXT, cached_markdown_version INT, description TEXT, description_html TEXT, encrypted_secret_token VARCHAR(255), encrypted_secret_token_iv VARCHAR(255), secret BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.software_license_policies (id INT NOT NULL, project_id INT NOT NULL, software_license_id INT NOT NULL, classification INT DEFAULT 0 NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.software_licenses (id INT NOT NULL, name VARCHAR NOT NULL, spdx_identifier VARCHAR(255));

CREATE TABLE public.spam_logs (id INT NOT NULL, user_id INT, source_ip VARCHAR, user_agent VARCHAR, via_api BOOLEAN, noteable_type VARCHAR, title VARCHAR, description TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, submitted_as_ham BOOLEAN DEFAULT FALSE NOT NULL, recaptcha_verified BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.status_page_settings (project_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, enabled BOOLEAN DEFAULT FALSE NOT NULL, aws_s3_bucket_name VARCHAR(63) NOT NULL, aws_region VARCHAR(255) NOT NULL, aws_access_key VARCHAR(255) NOT NULL, encrypted_aws_secret_key VARCHAR(255) NOT NULL, encrypted_aws_secret_key_iv VARCHAR(255) NOT NULL);

CREATE TABLE public.subscriptions (id INT NOT NULL, user_id INT, subscribable_id INT, subscribable_type VARCHAR, subscribed BOOLEAN, created_at TIMESTAMP, updated_at TIMESTAMP, project_id INT);

CREATE TABLE public.suggestions (id BIGINT NOT NULL, note_id INT NOT NULL, relative_order SMALLINT NOT NULL, applied BOOLEAN DEFAULT FALSE NOT NULL, commit_id VARCHAR, from_content TEXT NOT NULL, to_content TEXT NOT NULL, lines_above INT DEFAULT 0 NOT NULL, lines_below INT DEFAULT 0 NOT NULL, outdated BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.system_note_metadata (id INT NOT NULL, note_id INT NOT NULL, commit_count INT, action VARCHAR, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, description_version_id BIGINT);

CREATE TABLE public.taggings (id INT NOT NULL, tag_id INT, taggable_id INT, taggable_type VARCHAR, tagger_id INT, tagger_type VARCHAR, context VARCHAR, created_at TIMESTAMP);

CREATE TABLE public.tags (id INT NOT NULL, name VARCHAR, taggings_count INT DEFAULT 0);

CREATE TABLE public.term_agreements (id INT NOT NULL, term_id INT NOT NULL, user_id INT NOT NULL, accepted BOOLEAN DEFAULT FALSE NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.timelogs (id INT NOT NULL, time_spent INT NOT NULL, user_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, issue_id INT, merge_request_id INT, spent_at TIMESTAMP);

CREATE TABLE public.todos (id INT NOT NULL, user_id INT NOT NULL, project_id INT, target_id INT, target_type VARCHAR NOT NULL, author_id INT NOT NULL, action INT NOT NULL, state VARCHAR NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, note_id INT, commit_id VARCHAR, group_id INT);

CREATE TABLE public.trending_projects (id INT NOT NULL, project_id INT NOT NULL);

CREATE TABLE public.u2f_registrations (id INT NOT NULL, certificate TEXT, key_handle VARCHAR, public_key VARCHAR, counter INT, user_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, name VARCHAR);

CREATE TABLE public.uploads (id INT NOT NULL, size BIGINT NOT NULL, path VARCHAR(511) NOT NULL, checksum VARCHAR(64), model_id INT, model_type VARCHAR, uploader VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, store INT, mount_point VARCHAR, secret VARCHAR);

CREATE TABLE public.user_agent_details (id INT NOT NULL, user_agent VARCHAR NOT NULL, ip_address VARCHAR NOT NULL, subject_id INT NOT NULL, subject_type VARCHAR NOT NULL, submitted BOOLEAN DEFAULT FALSE NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.user_callouts (id INT NOT NULL, feature_name INT NOT NULL, user_id INT NOT NULL, dismissed_at TIMESTAMPTZ);

CREATE TABLE public.user_canonical_emails (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, user_id BIGINT NOT NULL, canonical_email VARCHAR NOT NULL);

CREATE TABLE public.user_custom_attributes (id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, user_id INT NOT NULL, key VARCHAR NOT NULL, value VARCHAR NOT NULL);

CREATE TABLE public.user_details (user_id BIGINT NOT NULL, job_title VARCHAR(200) DEFAULT CAST('' AS VARCHAR) NOT NULL);

CREATE TABLE public.user_highest_roles (user_id BIGINT NOT NULL, updated_at TIMESTAMPTZ NOT NULL, highest_access_level INT);

CREATE TABLE public.user_interacted_projects (user_id INT NOT NULL, project_id INT NOT NULL);

CREATE TABLE public.user_preferences (id INT NOT NULL, user_id INT NOT NULL, issue_notes_filter SMALLINT DEFAULT 0 NOT NULL, merge_request_notes_filter SMALLINT DEFAULT 0 NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, epics_sort VARCHAR, roadmap_epics_state INT, epic_notes_filter SMALLINT DEFAULT 0 NOT NULL, issues_sort VARCHAR, merge_requests_sort VARCHAR, roadmaps_sort VARCHAR, first_day_of_week INT, timezone VARCHAR, time_display_relative BOOLEAN, time_format_in_24h BOOLEAN, projects_sort VARCHAR(64), show_whitespace_in_diffs BOOLEAN DEFAULT TRUE NOT NULL, sourcegraph_enabled BOOLEAN, setup_for_company BOOLEAN, render_whitespace_in_code BOOLEAN, tab_width SMALLINT, feature_filter_type BIGINT);

CREATE TABLE public.user_statuses (user_id INT NOT NULL, cached_markdown_version INT, emoji VARCHAR DEFAULT CAST('speech_balloon' AS VARCHAR) NOT NULL, message VARCHAR(100), message_html VARCHAR);

CREATE TABLE public.user_synced_attributes_metadata (id INT NOT NULL, name_synced BOOLEAN DEFAULT FALSE, email_synced BOOLEAN DEFAULT FALSE, location_synced BOOLEAN DEFAULT FALSE, user_id INT NOT NULL, provider VARCHAR);

CREATE TABLE public.users (id INT NOT NULL, email VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, encrypted_password VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, reset_password_token VARCHAR, reset_password_sent_at TIMESTAMP, remember_created_at TIMESTAMP, sign_in_count INT DEFAULT 0, current_sign_in_at TIMESTAMP, last_sign_in_at TIMESTAMP, current_sign_in_ip VARCHAR, last_sign_in_ip VARCHAR, created_at TIMESTAMP, updated_at TIMESTAMP, name VARCHAR, admin BOOLEAN DEFAULT FALSE NOT NULL, projects_limit INT NOT NULL, skype VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, linkedin VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, twitter VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, bio VARCHAR, failed_attempts INT DEFAULT 0, locked_at TIMESTAMP, username VARCHAR, can_create_group BOOLEAN DEFAULT TRUE NOT NULL, can_create_team BOOLEAN DEFAULT TRUE NOT NULL, state VARCHAR, color_scheme_id INT DEFAULT 1 NOT NULL, password_expires_at TIMESTAMP, created_by_id INT, last_credential_check_at TIMESTAMP, avatar VARCHAR, confirmation_token VARCHAR, confirmed_at TIMESTAMP, confirmation_sent_at TIMESTAMP, unconfirmed_email VARCHAR, hide_no_ssh_key BOOLEAN DEFAULT FALSE, website_url VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, admin_email_unsubscribed_at TIMESTAMP, notification_email VARCHAR, hide_no_password BOOLEAN DEFAULT FALSE, password_automatically_set BOOLEAN DEFAULT FALSE, location VARCHAR, encrypted_otp_secret VARCHAR, encrypted_otp_secret_iv VARCHAR, encrypted_otp_secret_salt VARCHAR, otp_required_for_login BOOLEAN DEFAULT FALSE NOT NULL, otp_backup_codes TEXT, public_email VARCHAR DEFAULT CAST('' AS VARCHAR) NOT NULL, dashboard INT DEFAULT 0, project_view INT DEFAULT 0, consumed_timestep INT, layout INT DEFAULT 0, hide_project_limit BOOLEAN DEFAULT FALSE, note TEXT, unlock_token VARCHAR, otp_grace_period_started_at TIMESTAMP, external BOOLEAN DEFAULT FALSE, incoming_email_token VARCHAR, organization VARCHAR, auditor BOOLEAN DEFAULT FALSE NOT NULL, require_two_factor_authentication_from_group BOOLEAN DEFAULT FALSE NOT NULL, two_factor_grace_period INT DEFAULT 48 NOT NULL, ghost BOOLEAN, last_activity_on DATE, notified_of_own_activity BOOLEAN, preferred_language VARCHAR, email_opted_in BOOLEAN, email_opted_in_ip VARCHAR, email_opted_in_source_id INT, email_opted_in_at TIMESTAMP, theme_id SMALLINT, accepted_term_id INT, feed_token VARCHAR, private_profile BOOLEAN DEFAULT FALSE NOT NULL, roadmap_layout SMALLINT, include_private_contributions BOOLEAN, commit_email VARCHAR, group_view INT, managing_group_id INT, bot_type SMALLINT, first_name VARCHAR(255), last_name VARCHAR(255), static_object_token VARCHAR(255), role SMALLINT, user_type SMALLINT);

CREATE TABLE public.users_ops_dashboard_projects (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, user_id INT NOT NULL, project_id INT NOT NULL);

CREATE TABLE public.users_security_dashboard_projects (user_id BIGINT NOT NULL, project_id BIGINT NOT NULL);

CREATE TABLE public.users_star_projects (id INT NOT NULL, project_id INT NOT NULL, user_id INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP);

CREATE TABLE public.users_statistics (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, without_groups_and_projects INT DEFAULT 0 NOT NULL, with_highest_role_guest INT DEFAULT 0 NOT NULL, with_highest_role_reporter INT DEFAULT 0 NOT NULL, with_highest_role_developer INT DEFAULT 0 NOT NULL, with_highest_role_maintainer INT DEFAULT 0 NOT NULL, with_highest_role_owner INT DEFAULT 0 NOT NULL, bots INT DEFAULT 0 NOT NULL, blocked INT DEFAULT 0 NOT NULL);

CREATE TABLE public.vulnerabilities (id BIGINT NOT NULL, milestone_id BIGINT, epic_id BIGINT, project_id BIGINT NOT NULL, author_id BIGINT NOT NULL, updated_by_id BIGINT, last_edited_by_id BIGINT, start_date DATE, due_date DATE, last_edited_at TIMESTAMPTZ, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, title VARCHAR(255) NOT NULL, title_html TEXT, description TEXT, description_html TEXT, start_date_sourcing_milestone_id BIGINT, due_date_sourcing_milestone_id BIGINT, state SMALLINT DEFAULT 1 NOT NULL, severity SMALLINT NOT NULL, severity_overridden BOOLEAN DEFAULT FALSE, confidence SMALLINT NOT NULL, confidence_overridden BOOLEAN DEFAULT FALSE, resolved_by_id BIGINT, resolved_at TIMESTAMPTZ, report_type SMALLINT NOT NULL, cached_markdown_version INT, confirmed_by_id BIGINT, confirmed_at TIMESTAMPTZ, dismissed_at TIMESTAMPTZ, dismissed_by_id BIGINT);

CREATE TABLE public.vulnerability_exports (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, started_at TIMESTAMPTZ, finished_at TIMESTAMPTZ, status VARCHAR(255) NOT NULL, file VARCHAR(255), project_id BIGINT NOT NULL, author_id BIGINT NOT NULL, file_store INT, format SMALLINT DEFAULT 0 NOT NULL);

CREATE TABLE public.vulnerability_feedback (id INT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, feedback_type SMALLINT NOT NULL, category SMALLINT NOT NULL, project_id INT NOT NULL, author_id INT NOT NULL, pipeline_id INT, issue_id INT, project_fingerprint VARCHAR(40) NOT NULL, merge_request_id INT, comment_author_id INT, comment TEXT, comment_timestamp TIMESTAMPTZ);

CREATE TABLE public.vulnerability_identifiers (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id INT NOT NULL, fingerprint BYTEA NOT NULL, external_type VARCHAR NOT NULL, external_id VARCHAR NOT NULL, name VARCHAR NOT NULL, url TEXT);

CREATE TABLE public.vulnerability_issue_links (id BIGINT NOT NULL, vulnerability_id BIGINT NOT NULL, issue_id BIGINT NOT NULL, link_type SMALLINT DEFAULT 1 NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL);

CREATE TABLE public.vulnerability_occurrence_identifiers (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, occurrence_id BIGINT NOT NULL, identifier_id BIGINT NOT NULL);

CREATE TABLE public.vulnerability_occurrence_pipelines (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, occurrence_id BIGINT NOT NULL, pipeline_id INT NOT NULL);

CREATE TABLE public.vulnerability_occurrences (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, severity SMALLINT NOT NULL, confidence SMALLINT NOT NULL, report_type SMALLINT NOT NULL, project_id INT NOT NULL, scanner_id BIGINT NOT NULL, primary_identifier_id BIGINT NOT NULL, project_fingerprint BYTEA NOT NULL, location_fingerprint BYTEA NOT NULL, uuid VARCHAR(36) NOT NULL, name VARCHAR NOT NULL, metadata_version VARCHAR NOT NULL, raw_metadata TEXT NOT NULL, vulnerability_id BIGINT);

CREATE TABLE public.vulnerability_scanners (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id INT NOT NULL, external_id VARCHAR NOT NULL, name VARCHAR NOT NULL);

CREATE TABLE public.vulnerability_user_mentions (id BIGINT NOT NULL, vulnerability_id BIGINT NOT NULL, note_id INT, mentioned_users_ids INT[], mentioned_projects_ids INT[], mentioned_groups_ids INT[]);

CREATE TABLE public.web_hook_logs (id INT NOT NULL, web_hook_id INT NOT NULL, "trigger" VARCHAR, url VARCHAR, request_headers TEXT, request_data TEXT, response_headers TEXT, response_body TEXT, response_status VARCHAR, execution_duration DOUBLE PRECISION, internal_error_message VARCHAR, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.web_hooks (id INT NOT NULL, project_id INT, created_at TIMESTAMP, updated_at TIMESTAMP, type VARCHAR DEFAULT CAST('ProjectHook' AS VARCHAR), service_id INT, push_events BOOLEAN DEFAULT TRUE NOT NULL, issues_events BOOLEAN DEFAULT FALSE NOT NULL, merge_requests_events BOOLEAN DEFAULT FALSE NOT NULL, tag_push_events BOOLEAN DEFAULT FALSE, group_id INT, note_events BOOLEAN DEFAULT FALSE NOT NULL, enable_ssl_verification BOOLEAN DEFAULT TRUE, wiki_page_events BOOLEAN DEFAULT FALSE NOT NULL, pipeline_events BOOLEAN DEFAULT FALSE NOT NULL, confidential_issues_events BOOLEAN DEFAULT FALSE NOT NULL, repository_update_events BOOLEAN DEFAULT FALSE NOT NULL, job_events BOOLEAN DEFAULT FALSE NOT NULL, confidential_note_events BOOLEAN, push_events_branch_filter TEXT, encrypted_token VARCHAR, encrypted_token_iv VARCHAR, encrypted_url VARCHAR, encrypted_url_iv VARCHAR);

CREATE TABLE public.wiki_page_meta (id INT NOT NULL, project_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, title VARCHAR(255) NOT NULL);

CREATE TABLE public.wiki_page_slugs (id INT NOT NULL, canonical BOOLEAN DEFAULT FALSE NOT NULL, wiki_page_meta_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, slug VARCHAR(2048) NOT NULL);

CREATE TABLE public.x509_certificates (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, subject_key_identifier VARCHAR(255) NOT NULL, subject VARCHAR(255) NOT NULL, email VARCHAR(255) NOT NULL, serial_number BYTEA NOT NULL, certificate_status SMALLINT DEFAULT 0 NOT NULL, x509_issuer_id BIGINT NOT NULL);

CREATE TABLE public.x509_commit_signatures (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, project_id BIGINT NOT NULL, x509_certificate_id BIGINT NOT NULL, commit_sha BYTEA NOT NULL, verification_status SMALLINT DEFAULT 0 NOT NULL);

CREATE TABLE public.x509_issuers (id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, subject_key_identifier VARCHAR(255) NOT NULL, subject VARCHAR(255) NOT NULL, crl_url VARCHAR(255) NOT NULL);

CREATE TABLE public.zoom_meetings (id BIGINT NOT NULL, project_id BIGINT NOT NULL, issue_id BIGINT NOT NULL, created_at TIMESTAMPTZ NOT NULL, updated_at TIMESTAMPTZ NOT NULL, issue_status SMALLINT DEFAULT 1 NOT NULL, url VARCHAR(255));

ALTER TABLE ONLY public.abuse_reports ADD CONSTRAINT abuse_reports_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.alerts_service_data ADD CONSTRAINT alerts_service_data_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.allowed_email_domains ADD CONSTRAINT allowed_email_domains_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.analytics_cycle_analytics_group_stages ADD CONSTRAINT analytics_cycle_analytics_group_stages_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.analytics_cycle_analytics_project_stages ADD CONSTRAINT analytics_cycle_analytics_project_stages_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.appearances ADD CONSTRAINT appearances_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.application_setting_terms ADD CONSTRAINT application_setting_terms_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.application_settings ADD CONSTRAINT application_settings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.approval_merge_request_rule_sources ADD CONSTRAINT approval_merge_request_rule_sources_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.approval_merge_request_rules_approved_approvers ADD CONSTRAINT approval_merge_request_rules_approved_approvers_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.approval_merge_request_rules_groups ADD CONSTRAINT approval_merge_request_rules_groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.approval_merge_request_rules ADD CONSTRAINT approval_merge_request_rules_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.approval_merge_request_rules_users ADD CONSTRAINT approval_merge_request_rules_users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.approval_project_rules_groups ADD CONSTRAINT approval_project_rules_groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.approval_project_rules ADD CONSTRAINT approval_project_rules_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.approval_project_rules_users ADD CONSTRAINT approval_project_rules_users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.approvals ADD CONSTRAINT approvals_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.approver_groups ADD CONSTRAINT approver_groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.approvers ADD CONSTRAINT approvers_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ar_internal_metadata ADD CONSTRAINT ar_internal_metadata_pkey PRIMARY KEY (key);

ALTER TABLE ONLY public.audit_events ADD CONSTRAINT audit_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.award_emoji ADD CONSTRAINT award_emoji_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.aws_roles ADD CONSTRAINT aws_roles_pkey PRIMARY KEY (user_id);

ALTER TABLE ONLY public.badges ADD CONSTRAINT badges_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.board_assignees ADD CONSTRAINT board_assignees_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.board_group_recent_visits ADD CONSTRAINT board_group_recent_visits_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.board_labels ADD CONSTRAINT board_labels_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.board_project_recent_visits ADD CONSTRAINT board_project_recent_visits_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.boards ADD CONSTRAINT boards_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.broadcast_messages ADD CONSTRAINT broadcast_messages_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.chat_names ADD CONSTRAINT chat_names_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.chat_teams ADD CONSTRAINT chat_teams_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_build_needs ADD CONSTRAINT ci_build_needs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_build_trace_chunks ADD CONSTRAINT ci_build_trace_chunks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_build_trace_section_names ADD CONSTRAINT ci_build_trace_section_names_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_builds_metadata ADD CONSTRAINT ci_builds_metadata_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_builds ADD CONSTRAINT ci_builds_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_builds_runner_session ADD CONSTRAINT ci_builds_runner_session_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_daily_report_results ADD CONSTRAINT ci_daily_report_results_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_group_variables ADD CONSTRAINT ci_group_variables_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_job_artifacts ADD CONSTRAINT ci_job_artifacts_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_job_variables ADD CONSTRAINT ci_job_variables_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_pipeline_chat_data ADD CONSTRAINT ci_pipeline_chat_data_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_pipeline_schedule_variables ADD CONSTRAINT ci_pipeline_schedule_variables_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_pipeline_schedules ADD CONSTRAINT ci_pipeline_schedules_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_pipeline_variables ADD CONSTRAINT ci_pipeline_variables_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_pipelines_config ADD CONSTRAINT ci_pipelines_config_pkey PRIMARY KEY (pipeline_id);

ALTER TABLE ONLY public.ci_pipelines ADD CONSTRAINT ci_pipelines_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_refs ADD CONSTRAINT ci_refs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_resource_groups ADD CONSTRAINT ci_resource_groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_resources ADD CONSTRAINT ci_resources_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_runner_namespaces ADD CONSTRAINT ci_runner_namespaces_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_runner_projects ADD CONSTRAINT ci_runner_projects_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_runners ADD CONSTRAINT ci_runners_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_sources_pipelines ADD CONSTRAINT ci_sources_pipelines_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_sources_projects ADD CONSTRAINT ci_sources_projects_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_stages ADD CONSTRAINT ci_stages_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_subscriptions_projects ADD CONSTRAINT ci_subscriptions_projects_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_trigger_requests ADD CONSTRAINT ci_trigger_requests_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_triggers ADD CONSTRAINT ci_triggers_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ci_variables ADD CONSTRAINT ci_variables_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.cluster_groups ADD CONSTRAINT cluster_groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.cluster_platforms_kubernetes ADD CONSTRAINT cluster_platforms_kubernetes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.cluster_projects ADD CONSTRAINT cluster_projects_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.cluster_providers_aws ADD CONSTRAINT cluster_providers_aws_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.cluster_providers_gcp ADD CONSTRAINT cluster_providers_gcp_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.clusters_applications_cert_managers ADD CONSTRAINT clusters_applications_cert_managers_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.clusters_applications_crossplane ADD CONSTRAINT clusters_applications_crossplane_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.clusters_applications_elastic_stacks ADD CONSTRAINT clusters_applications_elastic_stacks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.clusters_applications_helm ADD CONSTRAINT clusters_applications_helm_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.clusters_applications_ingress ADD CONSTRAINT clusters_applications_ingress_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.clusters_applications_jupyter ADD CONSTRAINT clusters_applications_jupyter_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.clusters_applications_knative ADD CONSTRAINT clusters_applications_knative_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.clusters_applications_prometheus ADD CONSTRAINT clusters_applications_prometheus_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.clusters_applications_runners ADD CONSTRAINT clusters_applications_runners_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.clusters_kubernetes_namespaces ADD CONSTRAINT clusters_kubernetes_namespaces_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.clusters ADD CONSTRAINT clusters_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.commit_user_mentions ADD CONSTRAINT commit_user_mentions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.container_expiration_policies ADD CONSTRAINT container_expiration_policies_pkey PRIMARY KEY (project_id);

ALTER TABLE ONLY public.container_repositories ADD CONSTRAINT container_repositories_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.conversational_development_index_metrics ADD CONSTRAINT conversational_development_index_metrics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.dependency_proxy_blobs ADD CONSTRAINT dependency_proxy_blobs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.dependency_proxy_group_settings ADD CONSTRAINT dependency_proxy_group_settings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.deploy_keys_projects ADD CONSTRAINT deploy_keys_projects_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.deploy_tokens ADD CONSTRAINT deploy_tokens_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.deployment_clusters ADD CONSTRAINT deployment_clusters_pkey PRIMARY KEY (deployment_id);

ALTER TABLE ONLY public.deployments ADD CONSTRAINT deployments_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.description_versions ADD CONSTRAINT description_versions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.design_management_designs ADD CONSTRAINT design_management_designs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.design_management_designs_versions ADD CONSTRAINT design_management_designs_versions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.design_management_versions ADD CONSTRAINT design_management_versions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.design_user_mentions ADD CONSTRAINT design_user_mentions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.draft_notes ADD CONSTRAINT draft_notes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.emails ADD CONSTRAINT emails_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.environments ADD CONSTRAINT environments_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.epic_issues ADD CONSTRAINT epic_issues_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.epic_metrics ADD CONSTRAINT epic_metrics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.epic_user_mentions ADD CONSTRAINT epic_user_mentions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.epics ADD CONSTRAINT epics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.events ADD CONSTRAINT events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.evidences ADD CONSTRAINT evidences_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.external_pull_requests ADD CONSTRAINT external_pull_requests_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.feature_gates ADD CONSTRAINT feature_gates_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.features ADD CONSTRAINT features_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.fork_network_members ADD CONSTRAINT fork_network_members_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.fork_networks ADD CONSTRAINT fork_networks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_cache_invalidation_events ADD CONSTRAINT geo_cache_invalidation_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_container_repository_updated_events ADD CONSTRAINT geo_container_repository_updated_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT geo_event_log_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_events ADD CONSTRAINT geo_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_hashed_storage_attachments_events ADD CONSTRAINT geo_hashed_storage_attachments_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_hashed_storage_migrated_events ADD CONSTRAINT geo_hashed_storage_migrated_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_job_artifact_deleted_events ADD CONSTRAINT geo_job_artifact_deleted_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_lfs_object_deleted_events ADD CONSTRAINT geo_lfs_object_deleted_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_node_namespace_links ADD CONSTRAINT geo_node_namespace_links_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_node_statuses ADD CONSTRAINT geo_node_statuses_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_nodes ADD CONSTRAINT geo_nodes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_repositories_changed_events ADD CONSTRAINT geo_repositories_changed_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_repository_created_events ADD CONSTRAINT geo_repository_created_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_repository_deleted_events ADD CONSTRAINT geo_repository_deleted_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_repository_renamed_events ADD CONSTRAINT geo_repository_renamed_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_repository_updated_events ADD CONSTRAINT geo_repository_updated_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_reset_checksum_events ADD CONSTRAINT geo_reset_checksum_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.geo_upload_deleted_events ADD CONSTRAINT geo_upload_deleted_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.gitlab_subscription_histories ADD CONSTRAINT gitlab_subscription_histories_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.gitlab_subscriptions ADD CONSTRAINT gitlab_subscriptions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.gpg_key_subkeys ADD CONSTRAINT gpg_key_subkeys_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.gpg_keys ADD CONSTRAINT gpg_keys_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.gpg_signatures ADD CONSTRAINT gpg_signatures_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.grafana_integrations ADD CONSTRAINT grafana_integrations_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.group_custom_attributes ADD CONSTRAINT group_custom_attributes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.group_deletion_schedules ADD CONSTRAINT group_deletion_schedules_pkey PRIMARY KEY (group_id);

ALTER TABLE ONLY public.group_deploy_tokens ADD CONSTRAINT group_deploy_tokens_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.group_group_links ADD CONSTRAINT group_group_links_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.historical_data ADD CONSTRAINT historical_data_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.identities ADD CONSTRAINT identities_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.import_export_uploads ADD CONSTRAINT import_export_uploads_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.import_failures ADD CONSTRAINT import_failures_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.index_statuses ADD CONSTRAINT index_statuses_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.insights ADD CONSTRAINT insights_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.internal_ids ADD CONSTRAINT internal_ids_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ip_restrictions ADD CONSTRAINT ip_restrictions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.issue_links ADD CONSTRAINT issue_links_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.issue_metrics ADD CONSTRAINT issue_metrics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.issue_tracker_data ADD CONSTRAINT issue_tracker_data_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.issue_user_mentions ADD CONSTRAINT issue_user_mentions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.issues ADD CONSTRAINT issues_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.jira_connect_installations ADD CONSTRAINT jira_connect_installations_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.jira_connect_subscriptions ADD CONSTRAINT jira_connect_subscriptions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.jira_tracker_data ADD CONSTRAINT jira_tracker_data_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.keys ADD CONSTRAINT keys_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.label_links ADD CONSTRAINT label_links_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.label_priorities ADD CONSTRAINT label_priorities_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.labels ADD CONSTRAINT labels_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ldap_group_links ADD CONSTRAINT ldap_group_links_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.lfs_file_locks ADD CONSTRAINT lfs_file_locks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.lfs_objects ADD CONSTRAINT lfs_objects_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.lfs_objects_projects ADD CONSTRAINT lfs_objects_projects_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.licenses ADD CONSTRAINT licenses_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.list_user_preferences ADD CONSTRAINT list_user_preferences_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.lists ADD CONSTRAINT lists_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.members ADD CONSTRAINT members_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.merge_request_assignees ADD CONSTRAINT merge_request_assignees_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.merge_request_blocks ADD CONSTRAINT merge_request_blocks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.merge_request_context_commits ADD CONSTRAINT merge_request_context_commits_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.merge_request_diffs ADD CONSTRAINT merge_request_diffs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.merge_request_metrics ADD CONSTRAINT merge_request_metrics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.merge_request_user_mentions ADD CONSTRAINT merge_request_user_mentions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.merge_requests_closing_issues ADD CONSTRAINT merge_requests_closing_issues_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.merge_requests ADD CONSTRAINT merge_requests_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.merge_trains ADD CONSTRAINT merge_trains_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.milestones ADD CONSTRAINT milestones_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.namespace_aggregation_schedules ADD CONSTRAINT namespace_aggregation_schedules_pkey PRIMARY KEY (namespace_id);

ALTER TABLE ONLY public.namespace_root_storage_statistics ADD CONSTRAINT namespace_root_storage_statistics_pkey PRIMARY KEY (namespace_id);

ALTER TABLE ONLY public.namespace_statistics ADD CONSTRAINT namespace_statistics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.namespaces ADD CONSTRAINT namespaces_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.note_diff_files ADD CONSTRAINT note_diff_files_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.notes ADD CONSTRAINT notes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.notification_settings ADD CONSTRAINT notification_settings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.oauth_access_grants ADD CONSTRAINT oauth_access_grants_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.oauth_access_tokens ADD CONSTRAINT oauth_access_tokens_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.oauth_applications ADD CONSTRAINT oauth_applications_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.oauth_openid_requests ADD CONSTRAINT oauth_openid_requests_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.open_project_tracker_data ADD CONSTRAINT open_project_tracker_data_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.operations_feature_flag_scopes ADD CONSTRAINT operations_feature_flag_scopes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.operations_feature_flags_clients ADD CONSTRAINT operations_feature_flags_clients_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.operations_feature_flags ADD CONSTRAINT operations_feature_flags_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.operations_scopes ADD CONSTRAINT operations_scopes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.operations_strategies ADD CONSTRAINT operations_strategies_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.packages_build_infos ADD CONSTRAINT packages_build_infos_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.packages_conan_file_metadata ADD CONSTRAINT packages_conan_file_metadata_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.packages_conan_metadata ADD CONSTRAINT packages_conan_metadata_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.packages_dependencies ADD CONSTRAINT packages_dependencies_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.packages_dependency_links ADD CONSTRAINT packages_dependency_links_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.packages_maven_metadata ADD CONSTRAINT packages_maven_metadata_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.packages_package_files ADD CONSTRAINT packages_package_files_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.packages_packages ADD CONSTRAINT packages_packages_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.packages_tags ADD CONSTRAINT packages_tags_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.pages_domain_acme_orders ADD CONSTRAINT pages_domain_acme_orders_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.pages_domains ADD CONSTRAINT pages_domains_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.path_locks ADD CONSTRAINT path_locks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.personal_access_tokens ADD CONSTRAINT personal_access_tokens_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.plan_limits ADD CONSTRAINT plan_limits_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.plans ADD CONSTRAINT plans_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.pool_repositories ADD CONSTRAINT pool_repositories_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.programming_languages ADD CONSTRAINT programming_languages_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_alerting_settings ADD CONSTRAINT project_alerting_settings_pkey PRIMARY KEY (project_id);

ALTER TABLE ONLY public.project_aliases ADD CONSTRAINT project_aliases_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_auto_devops ADD CONSTRAINT project_auto_devops_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_ci_cd_settings ADD CONSTRAINT project_ci_cd_settings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_custom_attributes ADD CONSTRAINT project_custom_attributes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_daily_statistics ADD CONSTRAINT project_daily_statistics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_deploy_tokens ADD CONSTRAINT project_deploy_tokens_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_error_tracking_settings ADD CONSTRAINT project_error_tracking_settings_pkey PRIMARY KEY (project_id);

ALTER TABLE ONLY public.project_export_jobs ADD CONSTRAINT project_export_jobs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_feature_usages ADD CONSTRAINT project_feature_usages_pkey PRIMARY KEY (project_id);

ALTER TABLE ONLY public.project_features ADD CONSTRAINT project_features_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_group_links ADD CONSTRAINT project_group_links_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_import_data ADD CONSTRAINT project_import_data_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_incident_management_settings ADD CONSTRAINT project_incident_management_settings_pkey PRIMARY KEY (project_id);

ALTER TABLE ONLY public.project_metrics_settings ADD CONSTRAINT project_metrics_settings_pkey PRIMARY KEY (project_id);

ALTER TABLE ONLY public.project_mirror_data ADD CONSTRAINT project_mirror_data_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_repositories ADD CONSTRAINT project_repositories_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_repository_states ADD CONSTRAINT project_repository_states_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_settings ADD CONSTRAINT project_settings_pkey PRIMARY KEY (project_id);

ALTER TABLE ONLY public.project_statistics ADD CONSTRAINT project_statistics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.project_tracing_settings ADD CONSTRAINT project_tracing_settings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.projects ADD CONSTRAINT projects_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.prometheus_alert_events ADD CONSTRAINT prometheus_alert_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.prometheus_alerts ADD CONSTRAINT prometheus_alerts_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.prometheus_metrics ADD CONSTRAINT prometheus_metrics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.protected_branch_merge_access_levels ADD CONSTRAINT protected_branch_merge_access_levels_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.protected_branch_push_access_levels ADD CONSTRAINT protected_branch_push_access_levels_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.protected_branch_unprotect_access_levels ADD CONSTRAINT protected_branch_unprotect_access_levels_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.protected_branches ADD CONSTRAINT protected_branches_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.protected_environment_deploy_access_levels ADD CONSTRAINT protected_environment_deploy_access_levels_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.protected_environments ADD CONSTRAINT protected_environments_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.protected_tag_create_access_levels ADD CONSTRAINT protected_tag_create_access_levels_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.protected_tags ADD CONSTRAINT protected_tags_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.push_rules ADD CONSTRAINT push_rules_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.redirect_routes ADD CONSTRAINT redirect_routes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.release_links ADD CONSTRAINT release_links_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.releases ADD CONSTRAINT releases_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.remote_mirrors ADD CONSTRAINT remote_mirrors_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.requirements ADD CONSTRAINT requirements_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.resource_label_events ADD CONSTRAINT resource_label_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.resource_milestone_events ADD CONSTRAINT resource_milestone_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.resource_weight_events ADD CONSTRAINT resource_weight_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.reviews ADD CONSTRAINT reviews_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.routes ADD CONSTRAINT routes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.saml_providers ADD CONSTRAINT saml_providers_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.schema_migrations ADD CONSTRAINT schema_migrations_pkey PRIMARY KEY (version);

ALTER TABLE ONLY public.scim_identities ADD CONSTRAINT scim_identities_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.scim_oauth_access_tokens ADD CONSTRAINT scim_oauth_access_tokens_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.security_scans ADD CONSTRAINT security_scans_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.self_managed_prometheus_alert_events ADD CONSTRAINT self_managed_prometheus_alert_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.sent_notifications ADD CONSTRAINT sent_notifications_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.sentry_issues ADD CONSTRAINT sentry_issues_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.serverless_domain_cluster ADD CONSTRAINT serverless_domain_cluster_pkey PRIMARY KEY (uuid);

ALTER TABLE ONLY public.service_desk_settings ADD CONSTRAINT service_desk_settings_pkey PRIMARY KEY (project_id);

ALTER TABLE ONLY public.services ADD CONSTRAINT services_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.shards ADD CONSTRAINT shards_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.slack_integrations ADD CONSTRAINT slack_integrations_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.smartcard_identities ADD CONSTRAINT smartcard_identities_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.snippet_repositories ADD CONSTRAINT snippet_repositories_pkey PRIMARY KEY (snippet_id);

ALTER TABLE ONLY public.snippet_user_mentions ADD CONSTRAINT snippet_user_mentions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.snippets ADD CONSTRAINT snippets_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.software_license_policies ADD CONSTRAINT software_license_policies_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.software_licenses ADD CONSTRAINT software_licenses_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.spam_logs ADD CONSTRAINT spam_logs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.status_page_settings ADD CONSTRAINT status_page_settings_pkey PRIMARY KEY (project_id);

ALTER TABLE ONLY public.subscriptions ADD CONSTRAINT subscriptions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.suggestions ADD CONSTRAINT suggestions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.system_note_metadata ADD CONSTRAINT system_note_metadata_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.taggings ADD CONSTRAINT taggings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.tags ADD CONSTRAINT tags_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.term_agreements ADD CONSTRAINT term_agreements_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.timelogs ADD CONSTRAINT timelogs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.todos ADD CONSTRAINT todos_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.trending_projects ADD CONSTRAINT trending_projects_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.u2f_registrations ADD CONSTRAINT u2f_registrations_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.uploads ADD CONSTRAINT uploads_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_agent_details ADD CONSTRAINT user_agent_details_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_callouts ADD CONSTRAINT user_callouts_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_canonical_emails ADD CONSTRAINT user_canonical_emails_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_custom_attributes ADD CONSTRAINT user_custom_attributes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_details ADD CONSTRAINT user_details_pkey PRIMARY KEY (user_id);

ALTER TABLE ONLY public.user_highest_roles ADD CONSTRAINT user_highest_roles_pkey PRIMARY KEY (user_id);

ALTER TABLE ONLY public.user_preferences ADD CONSTRAINT user_preferences_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_statuses ADD CONSTRAINT user_statuses_pkey PRIMARY KEY (user_id);

ALTER TABLE ONLY public.user_synced_attributes_metadata ADD CONSTRAINT user_synced_attributes_metadata_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.users_ops_dashboard_projects ADD CONSTRAINT users_ops_dashboard_projects_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.users ADD CONSTRAINT users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.users_star_projects ADD CONSTRAINT users_star_projects_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.users_statistics ADD CONSTRAINT users_statistics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT vulnerabilities_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.vulnerability_exports ADD CONSTRAINT vulnerability_exports_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.vulnerability_feedback ADD CONSTRAINT vulnerability_feedback_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.vulnerability_identifiers ADD CONSTRAINT vulnerability_identifiers_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.vulnerability_issue_links ADD CONSTRAINT vulnerability_issue_links_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.vulnerability_occurrence_identifiers ADD CONSTRAINT vulnerability_occurrence_identifiers_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.vulnerability_occurrence_pipelines ADD CONSTRAINT vulnerability_occurrence_pipelines_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.vulnerability_occurrences ADD CONSTRAINT vulnerability_occurrences_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.vulnerability_scanners ADD CONSTRAINT vulnerability_scanners_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.vulnerability_user_mentions ADD CONSTRAINT vulnerability_user_mentions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.web_hook_logs ADD CONSTRAINT web_hook_logs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.web_hooks ADD CONSTRAINT web_hooks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.wiki_page_meta ADD CONSTRAINT wiki_page_meta_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.wiki_page_slugs ADD CONSTRAINT wiki_page_slugs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.x509_certificates ADD CONSTRAINT x509_certificates_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.x509_commit_signatures ADD CONSTRAINT x509_commit_signatures_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.x509_issuers ADD CONSTRAINT x509_issuers_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.zoom_meetings ADD CONSTRAINT zoom_meetings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.epics ADD CONSTRAINT fk_013c9f36ca FOREIGN KEY (due_date_sourcing_epic_id) REFERENCES public.epics (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.clusters_applications_runners ADD CONSTRAINT fk_02de2ded36 FOREIGN KEY (runner_id) REFERENCES public.ci_runners (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.design_management_designs_versions ADD CONSTRAINT fk_03c671965c FOREIGN KEY (design_id) REFERENCES public.design_management_designs (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issues ADD CONSTRAINT fk_05f1e72feb FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.merge_requests ADD CONSTRAINT fk_06067f5644 FOREIGN KEY (latest_merge_request_diff_id) REFERENCES public.merge_request_diffs (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.user_interacted_projects ADD CONSTRAINT fk_0894651f08 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.web_hooks ADD CONSTRAINT fk_0c8ca6d9d1 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.notification_settings ADD CONSTRAINT fk_0c95e91db7 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.lists ADD CONSTRAINT fk_0d3f677137 FOREIGN KEY (board_id) REFERENCES public.boards (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.group_deletion_schedules ADD CONSTRAINT fk_11e3ebfcdd FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT fk_1302949740 FOREIGN KEY (last_edited_by_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT fk_131d289c65 FOREIGN KEY (milestone_id) REFERENCES public.milestones (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.internal_ids ADD CONSTRAINT fk_162941d509 FOREIGN KEY (namespace_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_176d3fbb5d FOREIGN KEY (job_artifact_deleted_event_id) REFERENCES public.geo_job_artifact_deleted_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_features ADD CONSTRAINT fk_18513d9b92 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_pipelines ADD CONSTRAINT fk_190998ef09 FOREIGN KEY (external_pull_request_id) REFERENCES public.external_pull_requests (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT fk_1d37cddf91 FOREIGN KEY (epic_id) REFERENCES public.epics (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_sources_pipelines ADD CONSTRAINT fk_1e53c97c0a FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.boards ADD CONSTRAINT fk_1e9a074a35 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.epics ADD CONSTRAINT fk_1fbed67632 FOREIGN KEY (start_date_sourcing_milestone_id) REFERENCES public.milestones (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.geo_container_repository_updated_events ADD CONSTRAINT fk_212c89c706 FOREIGN KEY (container_repository_id) REFERENCES public.container_repositories (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.users_star_projects ADD CONSTRAINT fk_22cd27ddfc FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_stages ADD CONSTRAINT fk_2360681d1d FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.import_failures ADD CONSTRAINT fk_24b824da43 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_ci_cd_settings ADD CONSTRAINT fk_24c15d2f2e FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.epics ADD CONSTRAINT fk_25b99c1be3 FOREIGN KEY (parent_id) REFERENCES public.epics (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.projects ADD CONSTRAINT fk_25d8780d11 FOREIGN KEY (marked_for_deletion_by_user_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_pipelines ADD CONSTRAINT fk_262d4c2d19 FOREIGN KEY (auto_canceled_by_id) REFERENCES public.ci_pipelines (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_build_trace_sections ADD CONSTRAINT fk_264e112c66 FOREIGN KEY (section_name_id) REFERENCES public.ci_build_trace_section_names (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_27548c6db3 FOREIGN KEY (hashed_storage_migrated_event_id) REFERENCES public.geo_hashed_storage_migrated_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.deployments ADD CONSTRAINT fk_289bba3222 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.notes ADD CONSTRAINT fk_2e82291620 FOREIGN KEY (review_id) REFERENCES public.reviews (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.members ADD CONSTRAINT fk_2e88fb7ce9 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approvals ADD CONSTRAINT fk_310d714958 FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.namespaces ADD CONSTRAINT fk_319256d87a FOREIGN KEY (file_template_project_id) REFERENCES public.projects (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.merge_requests ADD CONSTRAINT fk_3308fe130c FOREIGN KEY (source_project_id) REFERENCES public.projects (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_group_variables ADD CONSTRAINT fk_33ae4d58d8 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.epics ADD CONSTRAINT fk_3654b61b03 FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.push_event_payloads ADD CONSTRAINT fk_36c74129da FOREIGN KEY (event_id) REFERENCES public.events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_builds ADD CONSTRAINT fk_3a9eaa254d FOREIGN KEY (stage_id) REFERENCES public.ci_stages (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.epics ADD CONSTRAINT fk_3c1fd1cccc FOREIGN KEY (due_date_sourcing_milestone_id) REFERENCES public.milestones (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_pipelines ADD CONSTRAINT fk_3d34ab2e06 FOREIGN KEY (pipeline_schedule_id) REFERENCES public.ci_pipeline_schedules (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_pipeline_schedule_variables ADD CONSTRAINT fk_41c35fda51 FOREIGN KEY (pipeline_schedule_id) REFERENCES public.ci_pipeline_schedules (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_42c3b54bed FOREIGN KEY (cache_invalidation_event_id) REFERENCES public.geo_cache_invalidation_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.remote_mirrors ADD CONSTRAINT fk_43a9aa4ca8 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_runner_projects ADD CONSTRAINT fk_4478a6f1e4 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.todos ADD CONSTRAINT fk_45054f9c45 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.releases ADD CONSTRAINT fk_47fe2a0596 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_4a99ebfd60 FOREIGN KEY (repositories_changed_event_id) REFERENCES public.geo_repositories_changed_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_build_trace_sections ADD CONSTRAINT fk_4ebe41f502 FOREIGN KEY (build_id) REFERENCES public.ci_builds (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.path_locks ADD CONSTRAINT fk_5265c98f24 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters_applications_prometheus ADD CONSTRAINT fk_557e773639 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_feedback ADD CONSTRAINT fk_563ff1912e FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.deploy_keys_projects ADD CONSTRAINT fk_58a901ca7e FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issue_assignees ADD CONSTRAINT fk_5e0c8d9154 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_requests ADD CONSTRAINT fk_6149611a04 FOREIGN KEY (assignee_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.events ADD CONSTRAINT fk_61fbf6ca48 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_requests ADD CONSTRAINT fk_641731faff FOREIGN KEY (updated_by_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_builds ADD CONSTRAINT fk_6661f4f0e8 FOREIGN KEY (resource_group_id) REFERENCES public.ci_resource_groups (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.merge_requests ADD CONSTRAINT fk_6a5165a692 FOREIGN KEY (milestone_id) REFERENCES public.milestones (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_6ada82d42a FOREIGN KEY (container_repository_updated_event_id) REFERENCES public.geo_container_repository_updated_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.projects ADD CONSTRAINT fk_6e5c14658a FOREIGN KEY (pool_repository_id) REFERENCES public.pool_repositories (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.protected_branch_push_access_levels ADD CONSTRAINT fk_7111b68cdb FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.services ADD CONSTRAINT fk_71cce407f9 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.user_interacted_projects ADD CONSTRAINT fk_722ceba4f7 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT fk_725465b774 FOREIGN KEY (dismissed_by_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.index_statuses ADD CONSTRAINT fk_74b2492545 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT fk_76bc5f5455 FOREIGN KEY (resolved_by_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.oauth_openid_requests ADD CONSTRAINT fk_77114b3b09 FOREIGN KEY (access_grant_id) REFERENCES public.oauth_access_grants (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_resource_groups ADD CONSTRAINT fk_774722d144 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.users ADD CONSTRAINT fk_789cd90b35 FOREIGN KEY (accepted_term_id) REFERENCES public.application_setting_terms (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_78a6492f68 FOREIGN KEY (repository_updated_event_id) REFERENCES public.geo_repository_updated_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.lists ADD CONSTRAINT fk_7a5553d60f FOREIGN KEY (label_id) REFERENCES public.labels (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_branches ADD CONSTRAINT fk_7a9c6d93e7 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT fk_7ac31eacb9 FOREIGN KEY (updated_by_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT fk_7c5bb22a22 FOREIGN KEY (due_date_sourcing_milestone_id) REFERENCES public.milestones (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.labels ADD CONSTRAINT fk_7de4989a69 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_metrics ADD CONSTRAINT fk_7f28d925f3 FOREIGN KEY (merged_by_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.import_export_uploads ADD CONSTRAINT fk_83319d9721 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.push_rules ADD CONSTRAINT fk_83b29894de FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_diffs ADD CONSTRAINT fk_8483f3258f FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_pipelines ADD CONSTRAINT fk_86635dbd80 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_86c84214ec FOREIGN KEY (repository_renamed_event_id) REFERENCES public.geo_repository_renamed_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.packages_package_files ADD CONSTRAINT fk_86f0f182f8 FOREIGN KEY (package_id) REFERENCES public.packages_packages (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_builds ADD CONSTRAINT fk_87f4cefcda FOREIGN KEY (upstream_pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT fk_88b4d546ef FOREIGN KEY (start_date_sourcing_milestone_id) REFERENCES public.milestones (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.issues ADD CONSTRAINT fk_899c8f3231 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_branch_merge_access_levels ADD CONSTRAINT fk_8a3072ccb3 FOREIGN KEY (protected_branch_id) REFERENCES public.protected_branches (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.releases ADD CONSTRAINT fk_8e4456f90f FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.protected_tags ADD CONSTRAINT fk_8e4af87648 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_pipeline_schedules ADD CONSTRAINT fk_8ead60fcc4 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.todos ADD CONSTRAINT fk_91d1f47b13 FOREIGN KEY (note_id) REFERENCES public.notes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_feedback ADD CONSTRAINT fk_94f7c8a81e FOREIGN KEY (comment_author_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.milestones ADD CONSTRAINT fk_95650a40d4 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT fk_959d40ad0a FOREIGN KEY (confirmed_by_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.application_settings ADD CONSTRAINT fk_964370041d FOREIGN KEY (usage_stats_set_by_user_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.issues ADD CONSTRAINT fk_96b1dd429c FOREIGN KEY (milestone_id) REFERENCES public.milestones (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.vulnerability_occurrences ADD CONSTRAINT fk_97ffe77653 FOREIGN KEY (vulnerability_id) REFERENCES public.vulnerabilities (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.protected_branch_merge_access_levels ADD CONSTRAINT fk_98f3d044fe FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.notes ADD CONSTRAINT fk_99e097b079 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_9b9afb1916 FOREIGN KEY (repository_created_event_id) REFERENCES public.geo_repository_created_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.milestones ADD CONSTRAINT fk_9bd0a0c791 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issues ADD CONSTRAINT fk_9c4516d665 FOREIGN KEY (duplicated_to_id) REFERENCES public.issues (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.epics ADD CONSTRAINT fk_9d480c64b2 FOREIGN KEY (start_date_sourcing_epic_id) REFERENCES public.epics (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_pipeline_schedules ADD CONSTRAINT fk_9ea99f58d2 FOREIGN KEY (owner_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.protected_branch_push_access_levels ADD CONSTRAINT fk_9ffc86a3d9 FOREIGN KEY (protected_branch_id) REFERENCES public.protected_branches (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.deployment_merge_requests ADD CONSTRAINT fk_a064ff4453 FOREIGN KEY (environment_id) REFERENCES public.environments (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issues ADD CONSTRAINT fk_a194299be1 FOREIGN KEY (moved_to_id) REFERENCES public.issues (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_builds ADD CONSTRAINT fk_a2141b1522 FOREIGN KEY (auto_canceled_by_id) REFERENCES public.ci_pipelines (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_pipelines ADD CONSTRAINT fk_a23be95014 FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.users ADD CONSTRAINT fk_a4b8fefe3e FOREIGN KEY (managing_group_id) REFERENCES public.namespaces (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.merge_requests ADD CONSTRAINT fk_a6963e8447 FOREIGN KEY (target_project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.epics ADD CONSTRAINT fk_aa5798e761 FOREIGN KEY (closed_by_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.identities ADD CONSTRAINT fk_aade90f0fc FOREIGN KEY (saml_provider_id) REFERENCES public.saml_providers (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_sources_pipelines ADD CONSTRAINT fk_acd9737679 FOREIGN KEY (source_project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_requests ADD CONSTRAINT fk_ad525e1f87 FOREIGN KEY (merge_user_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_variables ADD CONSTRAINT fk_ada5eb64b3 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_metrics ADD CONSTRAINT fk_ae440388cc FOREIGN KEY (latest_closed_by_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.fork_network_members ADD CONSTRAINT fk_b01280dae4 FOREIGN KEY (forked_from_project_id) REFERENCES public.projects (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT fk_b1de915a15 FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.protected_tag_create_access_levels ADD CONSTRAINT fk_b4eb82fe3c FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issue_assignees ADD CONSTRAINT fk_b7d881734a FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_trigger_requests ADD CONSTRAINT fk_b8ec8b7245 FOREIGN KEY (trigger_id) REFERENCES public.ci_triggers (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.deployments ADD CONSTRAINT fk_b9a3851b82 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.gitlab_subscriptions ADD CONSTRAINT fk_bd0c4019c3 FOREIGN KEY (hosted_plan_id) REFERENCES public.plans (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.snippets ADD CONSTRAINT fk_be41fd4bb7 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_sources_pipelines ADD CONSTRAINT fk_be5624bf37 FOREIGN KEY (source_job_id) REFERENCES public.ci_builds (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.packages_maven_metadata ADD CONSTRAINT fk_be88aed360 FOREIGN KEY (package_id) REFERENCES public.packages_packages (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_builds ADD CONSTRAINT fk_befce0568a FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.design_management_versions ADD CONSTRAINT fk_c1440b4896 FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_c1f241c70d FOREIGN KEY (upload_deleted_event_id) REFERENCES public.geo_upload_deleted_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_c4b1c1f66e FOREIGN KEY (repository_deleted_event_id) REFERENCES public.geo_repository_deleted_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issues ADD CONSTRAINT fk_c63cbf6c25 FOREIGN KEY (closed_by_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.issue_links ADD CONSTRAINT fk_c900194ff2 FOREIGN KEY (source_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.todos ADD CONSTRAINT fk_ccf0373936 FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_cff7185ad2 FOREIGN KEY (reset_checksum_event_id) REFERENCES public.geo_reset_checksum_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_mirror_data ADD CONSTRAINT fk_d1aad367d7 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.environments ADD CONSTRAINT fk_d1c8c1da6a FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_builds ADD CONSTRAINT fk_d3130c9a7f FOREIGN KEY (commit_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_sources_pipelines ADD CONSTRAINT fk_d4e29af7d7 FOREIGN KEY (source_pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_d5af95fcd9 FOREIGN KEY (lfs_object_deleted_event_id) REFERENCES public.geo_lfs_object_deleted_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.lists ADD CONSTRAINT fk_d6cf4279f7 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.system_note_metadata ADD CONSTRAINT fk_d83a918cb1 FOREIGN KEY (note_id) REFERENCES public.notes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.todos ADD CONSTRAINT fk_d94154aa95 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.label_links ADD CONSTRAINT fk_d97dd08678 FOREIGN KEY (label_id) REFERENCES public.labels (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_group_links ADD CONSTRAINT fk_daa8cee94c FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.epics ADD CONSTRAINT fk_dccd3f98fc FOREIGN KEY (assignee_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.issues ADD CONSTRAINT fk_df75a7c8b8 FOREIGN KEY (promoted_to_epic_id) REFERENCES public.epics (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_resources ADD CONSTRAINT fk_e169a8e3d5 FOREIGN KEY (build_id) REFERENCES public.ci_builds (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_sources_pipelines ADD CONSTRAINT fk_e1bad85861 FOREIGN KEY (pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.gitlab_subscriptions ADD CONSTRAINT fk_e2595d00a1 FOREIGN KEY (namespace_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_triggers ADD CONSTRAINT fk_e3e63f966e FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_requests ADD CONSTRAINT fk_e719a85f8a FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.issue_links ADD CONSTRAINT fk_e71bb44f1f FOREIGN KEY (target_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.namespaces ADD CONSTRAINT fk_e7a0b20a6b FOREIGN KEY (custom_project_templates_group_id) REFERENCES public.namespaces (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.fork_networks ADD CONSTRAINT fk_e7b436b2b5 FOREIGN KEY (root_project_id) REFERENCES public.projects (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.application_settings ADD CONSTRAINT fk_e8a145f3a7 FOREIGN KEY (instance_administrators_group_id) REFERENCES public.namespaces (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_triggers ADD CONSTRAINT fk_e8e10d1964 FOREIGN KEY (owner_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.pages_domains ADD CONSTRAINT fk_ea2f6dfc6f FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.application_settings ADD CONSTRAINT fk_ec757bd087 FOREIGN KEY (file_template_project_id) REFERENCES public.projects (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.events ADD CONSTRAINT fk_edfd187b6f FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerabilities ADD CONSTRAINT fk_efb96ab1e2 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters ADD CONSTRAINT fk_f05c5e5a42 FOREIGN KEY (management_project_id) REFERENCES public.projects (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.epics ADD CONSTRAINT fk_f081aa4489 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.boards ADD CONSTRAINT fk_f15266b5f9 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_pipeline_variables ADD CONSTRAINT fk_f29c5f4380 FOREIGN KEY (pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.design_management_designs_versions ADD CONSTRAINT fk_f4d25ba00c FOREIGN KEY (version_id) REFERENCES public.design_management_versions (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_tag_create_access_levels ADD CONSTRAINT fk_f7dfda8c51 FOREIGN KEY (protected_tag_id) REFERENCES public.protected_tags (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_stages ADD CONSTRAINT fk_fb57e6cc56 FOREIGN KEY (pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.system_note_metadata ADD CONSTRAINT fk_fbd87415c9 FOREIGN KEY (description_version_id) REFERENCES public.description_versions (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.merge_requests ADD CONSTRAINT fk_fd82eae0b9 FOREIGN KEY (head_pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.namespaces ADD CONSTRAINT fk_fdd12e5b80 FOREIGN KEY (plan_id) REFERENCES public.plans (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.project_import_data ADD CONSTRAINT fk_ffb9ee3a10 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issues ADD CONSTRAINT fk_ffed080f01 FOREIGN KEY (updated_by_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.geo_event_log ADD CONSTRAINT fk_geo_event_log_on_geo_event_id FOREIGN KEY (geo_event_id) REFERENCES public.geo_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.personal_access_tokens ADD CONSTRAINT fk_personal_access_tokens_user_id FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_merge_request_rules ADD CONSTRAINT fk_rails_004ce82224 FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.namespace_statistics ADD CONSTRAINT fk_rails_0062050394 FOREIGN KEY (namespace_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters_applications_elastic_stacks ADD CONSTRAINT fk_rails_026f219f46 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.events ADD CONSTRAINT fk_rails_0434b48643 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ip_restrictions ADD CONSTRAINT fk_rails_04a93778d5 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_subscriptions_projects ADD CONSTRAINT fk_rails_0818751483 FOREIGN KEY (downstream_project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.trending_projects ADD CONSTRAINT fk_rails_09feecd872 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_deploy_tokens ADD CONSTRAINT fk_rails_0aca134388 FOREIGN KEY (deploy_token_id) REFERENCES public.deploy_tokens (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.packages_conan_file_metadata ADD CONSTRAINT fk_rails_0afabd9328 FOREIGN KEY (package_file_id) REFERENCES public.packages_package_files (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_node_statuses ADD CONSTRAINT fk_rails_0ecc699c2a FOREIGN KEY (geo_node_id) REFERENCES public.geo_nodes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_repository_states ADD CONSTRAINT fk_rails_0f2298ca8a FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.user_synced_attributes_metadata ADD CONSTRAINT fk_rails_0f4aa0981f FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_authorizations ADD CONSTRAINT fk_rails_0f84bb11f3 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_context_commits ADD CONSTRAINT fk_rails_0fe0039f60 FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_build_trace_chunks ADD CONSTRAINT fk_rails_1013b761f2 FOREIGN KEY (build_id) REFERENCES public.ci_builds (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_exports ADD CONSTRAINT fk_rails_1019162882 FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.prometheus_alert_events ADD CONSTRAINT fk_rails_106f901176 FOREIGN KEY (prometheus_alert_id) REFERENCES public.prometheus_alerts (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_sources_projects ADD CONSTRAINT fk_rails_10a1eb379a FOREIGN KEY (pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.zoom_meetings ADD CONSTRAINT fk_rails_1190f0e0fa FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.gpg_signatures ADD CONSTRAINT fk_rails_11ae8cb9a7 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_authorizations ADD CONSTRAINT fk_rails_11e7aa3ed9 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.description_versions ADD CONSTRAINT fk_rails_12b144011c FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_statistics ADD CONSTRAINT fk_rails_12c471002f FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.user_details ADD CONSTRAINT fk_rails_12e0b3043d FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.users_security_dashboard_projects ADD CONSTRAINT fk_rails_150cd5682c FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_deploy_tokens ADD CONSTRAINT fk_rails_170e03cbaf FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.analytics_cycle_analytics_project_stages ADD CONSTRAINT fk_rails_1722574860 FOREIGN KEY (start_event_label_id) REFERENCES public.labels (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.packages_build_infos ADD CONSTRAINT fk_rails_17a9a0dffc FOREIGN KEY (pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.clusters_applications_jupyter ADD CONSTRAINT fk_rails_17df21c98c FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.cluster_providers_aws ADD CONSTRAINT fk_rails_18983d9ea4 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.grafana_integrations ADD CONSTRAINT fk_rails_18d0e2b564 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.open_project_tracker_data ADD CONSTRAINT fk_rails_1987546e48 FOREIGN KEY (service_id) REFERENCES public.services (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.gpg_signatures ADD CONSTRAINT fk_rails_19d4f1c6f9 FOREIGN KEY (gpg_key_subkey_id) REFERENCES public.gpg_key_subkeys (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.vulnerability_user_mentions ADD CONSTRAINT fk_rails_1a41c485cd FOREIGN KEY (vulnerability_id) REFERENCES public.vulnerabilities (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.board_assignees ADD CONSTRAINT fk_rails_1c0ff59e82 FOREIGN KEY (assignee_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.epic_user_mentions ADD CONSTRAINT fk_rails_1c65976a49 FOREIGN KEY (note_id) REFERENCES public.notes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approver_groups ADD CONSTRAINT fk_rails_1cdcbd7723 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_refs ADD CONSTRAINT fk_rails_1da48d19ce FOREIGN KEY (last_updated_by_pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.packages_tags ADD CONSTRAINT fk_rails_1dfc868911 FOREIGN KEY (package_id) REFERENCES public.packages_packages (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_repository_created_events ADD CONSTRAINT fk_rails_1f49e46a61 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_merge_request_rules_groups ADD CONSTRAINT fk_rails_2020a7124a FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_feedback ADD CONSTRAINT fk_rails_20976e6fd9 FOREIGN KEY (pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.user_statuses ADD CONSTRAINT fk_rails_2178592333 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.users_ops_dashboard_projects ADD CONSTRAINT fk_rails_220a0562db FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters_applications_runners ADD CONSTRAINT fk_rails_22388594e9 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.service_desk_settings ADD CONSTRAINT fk_rails_223a296a85 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_tag_create_access_levels ADD CONSTRAINT fk_rails_2349b78b91 FOREIGN KEY (user_id) REFERENCES public.users (id);

ALTER TABLE ONLY public.group_custom_attributes ADD CONSTRAINT fk_rails_246e0db83a FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.lfs_file_locks ADD CONSTRAINT fk_rails_27a1d98fa8 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_alerting_settings ADD CONSTRAINT fk_rails_27a84b407d FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.reviews ADD CONSTRAINT fk_rails_29e6f859c4 FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.draft_notes ADD CONSTRAINT fk_rails_2a8dac9901 FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.group_group_links ADD CONSTRAINT fk_rails_2b2353ca49 FOREIGN KEY (shared_with_group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_repository_updated_events ADD CONSTRAINT fk_rails_2b70854c08 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_branch_unprotect_access_levels ADD CONSTRAINT fk_rails_2d2aba21ef FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.saml_providers ADD CONSTRAINT fk_rails_306d459be7 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_diff_commits ADD CONSTRAINT fk_rails_316aaceda3 FOREIGN KEY (merge_request_diff_id) REFERENCES public.merge_request_diffs (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.zoom_meetings ADD CONSTRAINT fk_rails_3263f29616 FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.container_repositories ADD CONSTRAINT fk_rails_32f7bf5aad FOREIGN KEY (project_id) REFERENCES public.projects (id);

ALTER TABLE ONLY public.clusters_applications_jupyter ADD CONSTRAINT fk_rails_331f0aff78 FOREIGN KEY (oauth_application_id) REFERENCES public.oauth_applications (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.merge_request_metrics ADD CONSTRAINT fk_rails_33ae169d48 FOREIGN KEY (pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.suggestions ADD CONSTRAINT fk_rails_33b03a535c FOREIGN KEY (note_id) REFERENCES public.notes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.requirements ADD CONSTRAINT fk_rails_33fed8aa4e FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.wiki_page_slugs ADD CONSTRAINT fk_rails_358b46be14 FOREIGN KEY (wiki_page_meta_id) REFERENCES public.wiki_page_meta (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.board_labels ADD CONSTRAINT fk_rails_362b0600a3 FOREIGN KEY (label_id) REFERENCES public.labels (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_blocks ADD CONSTRAINT fk_rails_364d4bea8b FOREIGN KEY (blocked_merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.analytics_cycle_analytics_project_stages ADD CONSTRAINT fk_rails_3829e49b66 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issue_user_mentions ADD CONSTRAINT fk_rails_3861d9fefa FOREIGN KEY (note_id) REFERENCES public.notes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.self_managed_prometheus_alert_events ADD CONSTRAINT fk_rails_3936dadc62 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_project_rules_groups ADD CONSTRAINT fk_rails_396841e79e FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.self_managed_prometheus_alert_events ADD CONSTRAINT fk_rails_39d83d1b65 FOREIGN KEY (environment_id) REFERENCES public.environments (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.chat_teams ADD CONSTRAINT fk_rails_3b543909cb FOREIGN KEY (namespace_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_build_needs ADD CONSTRAINT fk_rails_3cf221d4ed FOREIGN KEY (build_id) REFERENCES public.ci_builds (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.cluster_groups ADD CONSTRAINT fk_rails_3d28377556 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.note_diff_files ADD CONSTRAINT fk_rails_3d66047aeb FOREIGN KEY (diff_note_id) REFERENCES public.notes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.snippet_user_mentions ADD CONSTRAINT fk_rails_3e00189191 FOREIGN KEY (snippet_id) REFERENCES public.snippets (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters_applications_helm ADD CONSTRAINT fk_rails_3e2b1c06bc FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.epic_user_mentions ADD CONSTRAINT fk_rails_3eaf4d88cc FOREIGN KEY (epic_id) REFERENCES public.epics (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.analytics_cycle_analytics_project_stages ADD CONSTRAINT fk_rails_3ec9fd7912 FOREIGN KEY (end_event_label_id) REFERENCES public.labels (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.board_assignees ADD CONSTRAINT fk_rails_3f6f926bd5 FOREIGN KEY (board_id) REFERENCES public.boards (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.description_versions ADD CONSTRAINT fk_rails_3ff658220b FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters_kubernetes_namespaces ADD CONSTRAINT fk_rails_40cc7ccbc3 FOREIGN KEY (cluster_project_id) REFERENCES public.cluster_projects (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.geo_node_namespace_links ADD CONSTRAINT fk_rails_41ff5fb854 FOREIGN KEY (namespace_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.epic_issues ADD CONSTRAINT fk_rails_4209981af6 FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_refs ADD CONSTRAINT fk_rails_4249db8cc3 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_resources ADD CONSTRAINT fk_rails_430336af2d FOREIGN KEY (resource_group_id) REFERENCES public.ci_resource_groups (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.lfs_file_locks ADD CONSTRAINT fk_rails_43df7a0412 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_assignees ADD CONSTRAINT fk_rails_443443ce6f FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.packages_dependency_links ADD CONSTRAINT fk_rails_4437bf4070 FOREIGN KEY (dependency_id) REFERENCES public.packages_dependencies (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_auto_devops ADD CONSTRAINT fk_rails_45436b12b2 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_requests_closing_issues ADD CONSTRAINT fk_rails_458eda8667 FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_environment_deploy_access_levels ADD CONSTRAINT fk_rails_45cc02a931 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.prometheus_alert_events ADD CONSTRAINT fk_rails_4675865839 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.smartcard_identities ADD CONSTRAINT fk_rails_4689f889a9 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_feedback ADD CONSTRAINT fk_rails_472f69b043 FOREIGN KEY (author_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.user_custom_attributes ADD CONSTRAINT fk_rails_47b91868a8 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.group_deletion_schedules ADD CONSTRAINT fk_rails_4b8c694a6c FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.design_management_designs ADD CONSTRAINT fk_rails_4bb1073360 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issue_metrics ADD CONSTRAINT fk_rails_4bb543d85d FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_metrics_settings ADD CONSTRAINT fk_rails_4c6037ee4f FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.prometheus_metrics ADD CONSTRAINT fk_rails_4c8957a707 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.scim_identities ADD CONSTRAINT fk_rails_4d2056ebd9 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.snippet_user_mentions ADD CONSTRAINT fk_rails_4d3f96b2cb FOREIGN KEY (note_id) REFERENCES public.notes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.deployment_clusters ADD CONSTRAINT fk_rails_4e6243e120 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_repository_renamed_events ADD CONSTRAINT fk_rails_4e6524febb FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.aws_roles ADD CONSTRAINT fk_rails_4ed56f4720 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.security_scans ADD CONSTRAINT fk_rails_4ef1e6b4c6 FOREIGN KEY (build_id) REFERENCES public.ci_builds (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_diff_files ADD CONSTRAINT fk_rails_501aa0a391 FOREIGN KEY (merge_request_diff_id) REFERENCES public.merge_request_diffs (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.status_page_settings ADD CONSTRAINT fk_rails_506e5ba391 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.x509_commit_signatures ADD CONSTRAINT fk_rails_53fe41188f FOREIGN KEY (x509_certificate_id) REFERENCES public.x509_certificates (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_node_namespace_links ADD CONSTRAINT fk_rails_546bf08d3e FOREIGN KEY (geo_node_id) REFERENCES public.geo_nodes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters_applications_knative ADD CONSTRAINT fk_rails_54fc91e0a0 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issue_user_mentions ADD CONSTRAINT fk_rails_57581fda73 FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_assignees ADD CONSTRAINT fk_rails_579d375628 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.analytics_cycle_analytics_group_stages ADD CONSTRAINT fk_rails_5a22f40223 FOREIGN KEY (start_event_label_id) REFERENCES public.labels (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.badges ADD CONSTRAINT fk_rails_5a7c055bdc FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.resource_label_events ADD CONSTRAINT fk_rails_5ac1d2fc24 FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_merge_request_rules_groups ADD CONSTRAINT fk_rails_5b2ecf6139 FOREIGN KEY (approval_merge_request_rule_id) REFERENCES public.approval_merge_request_rules (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_environment_deploy_access_levels ADD CONSTRAINT fk_rails_5b9f6970fe FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_branch_unprotect_access_levels ADD CONSTRAINT fk_rails_5be1abfc25 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.cluster_providers_gcp ADD CONSTRAINT fk_rails_5c2c3bc814 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.insights ADD CONSTRAINT fk_rails_5c4391f60a FOREIGN KEY (namespace_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_scanners ADD CONSTRAINT fk_rails_5c9d42a221 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.reviews ADD CONSTRAINT fk_rails_5ca11d8c31 FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.epic_issues ADD CONSTRAINT fk_rails_5d942936b4 FOREIGN KEY (epic_id) REFERENCES public.epics (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.resource_weight_events ADD CONSTRAINT fk_rails_5eb5cb92a1 FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_project_rules ADD CONSTRAINT fk_rails_5fb4dd100b FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_branch_merge_access_levels ADD CONSTRAINT fk_rails_5ffb4f3590 FOREIGN KEY (user_id) REFERENCES public.users (id);

ALTER TABLE ONLY public.user_highest_roles ADD CONSTRAINT fk_rails_60f6c325a6 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.dependency_proxy_group_settings ADD CONSTRAINT fk_rails_616ddd680a FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.group_deploy_tokens ADD CONSTRAINT fk_rails_61a572b41a FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.deployment_clusters ADD CONSTRAINT fk_rails_6359a164df FOREIGN KEY (deployment_id) REFERENCES public.deployments (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.evidences ADD CONSTRAINT fk_rails_6388b435a6 FOREIGN KEY (release_id) REFERENCES public.releases (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_occurrence_pipelines ADD CONSTRAINT fk_rails_6421e35d7d FOREIGN KEY (pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.group_deploy_tokens ADD CONSTRAINT fk_rails_6477b01f6b FOREIGN KEY (deploy_token_id) REFERENCES public.deploy_tokens (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.reviews ADD CONSTRAINT fk_rails_64798be025 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.operations_feature_flags ADD CONSTRAINT fk_rails_648e241be7 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_sources_projects ADD CONSTRAINT fk_rails_64b6855cbc FOREIGN KEY (source_project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.board_group_recent_visits ADD CONSTRAINT fk_rails_64bfc19bc5 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_merge_request_rule_sources ADD CONSTRAINT fk_rails_64e8ed3c7e FOREIGN KEY (approval_project_rule_id) REFERENCES public.approval_project_rules (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_pipeline_chat_data ADD CONSTRAINT fk_rails_64ebfab6b3 FOREIGN KEY (pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_project_rules_protected_branches ADD CONSTRAINT fk_rails_65203aa786 FOREIGN KEY (approval_project_rule_id) REFERENCES public.approval_project_rules (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.design_management_versions ADD CONSTRAINT fk_rails_6574200d99 FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_merge_request_rules_approved_approvers ADD CONSTRAINT fk_rails_6577725edb FOREIGN KEY (approval_merge_request_rule_id) REFERENCES public.approval_merge_request_rules (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.operations_feature_flags_clients ADD CONSTRAINT fk_rails_6650ed902c FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.web_hook_logs ADD CONSTRAINT fk_rails_666826e111 FOREIGN KEY (web_hook_id) REFERENCES public.web_hooks (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_hashed_storage_migrated_events ADD CONSTRAINT fk_rails_687ed7d7c5 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.plan_limits ADD CONSTRAINT fk_rails_69f8b6184f FOREIGN KEY (plan_id) REFERENCES public.plans (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.prometheus_alerts ADD CONSTRAINT fk_rails_6d9b283465 FOREIGN KEY (environment_id) REFERENCES public.environments (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.term_agreements ADD CONSTRAINT fk_rails_6ea6520e4a FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.users_security_dashboard_projects ADD CONSTRAINT fk_rails_6f6cf8e66e FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_builds_runner_session ADD CONSTRAINT fk_rails_70707857d3 FOREIGN KEY (build_id) REFERENCES public.ci_builds (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.list_user_preferences ADD CONSTRAINT fk_rails_70b2ef5ce2 FOREIGN KEY (list_id) REFERENCES public.lists (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_custom_attributes ADD CONSTRAINT fk_rails_719c3dccc5 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.slack_integrations ADD CONSTRAINT fk_rails_73db19721a FOREIGN KEY (service_id) REFERENCES public.services (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_context_commit_diff_files ADD CONSTRAINT fk_rails_74a00a1787 FOREIGN KEY (merge_request_context_commit_id) REFERENCES public.merge_request_context_commits (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters_applications_ingress ADD CONSTRAINT fk_rails_753a7b41c1 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.release_links ADD CONSTRAINT fk_rails_753be7ae29 FOREIGN KEY (release_id) REFERENCES public.releases (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.milestone_releases ADD CONSTRAINT fk_rails_754f27dbfa FOREIGN KEY (release_id) REFERENCES public.releases (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_repositories_changed_events ADD CONSTRAINT fk_rails_75ec0fefcc FOREIGN KEY (geo_node_id) REFERENCES public.geo_nodes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.resource_label_events ADD CONSTRAINT fk_rails_75efb0a653 FOREIGN KEY (epic_id) REFERENCES public.epics (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.path_locks ADD CONSTRAINT fk_rails_762cdcf942 FOREIGN KEY (user_id) REFERENCES public.users (id);

ALTER TABLE ONLY public.x509_certificates ADD CONSTRAINT fk_rails_76479fb5b4 FOREIGN KEY (x509_issuer_id) REFERENCES public.x509_issuers (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.pages_domain_acme_orders ADD CONSTRAINT fk_rails_76581b1c16 FOREIGN KEY (pages_domain_id) REFERENCES public.pages_domains (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_subscriptions_projects ADD CONSTRAINT fk_rails_7871f9a97b FOREIGN KEY (upstream_project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.software_license_policies ADD CONSTRAINT fk_rails_7a7a2a92de FOREIGN KEY (software_license_id) REFERENCES public.software_licenses (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_repositories ADD CONSTRAINT fk_rails_7a810d4121 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.operations_scopes ADD CONSTRAINT fk_rails_7a9358853b FOREIGN KEY (strategy_id) REFERENCES public.operations_strategies (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.milestone_releases ADD CONSTRAINT fk_rails_7ae0756a2d FOREIGN KEY (milestone_id) REFERENCES public.milestones (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.application_settings ADD CONSTRAINT fk_rails_7e112a9599 FOREIGN KEY (instance_administration_project_id) REFERENCES public.projects (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.clusters_kubernetes_namespaces ADD CONSTRAINT fk_rails_7e7688ecaf FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_merge_request_rules_users ADD CONSTRAINT fk_rails_80e6801803 FOREIGN KEY (approval_merge_request_rule_id) REFERENCES public.approval_merge_request_rules (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.deployment_merge_requests ADD CONSTRAINT fk_rails_86a6d8bf12 FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.analytics_language_trend_repository_languages ADD CONSTRAINT fk_rails_86cc9aef5f FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters_applications_crossplane ADD CONSTRAINT fk_rails_87186702df FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_runner_namespaces ADD CONSTRAINT fk_rails_8767676b7a FOREIGN KEY (runner_id) REFERENCES public.ci_runners (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.software_license_policies ADD CONSTRAINT fk_rails_87b2247ce5 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_environment_deploy_access_levels ADD CONSTRAINT fk_rails_898a13b650 FOREIGN KEY (protected_environment_id) REFERENCES public.protected_environments (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.snippet_repositories ADD CONSTRAINT fk_rails_8afd7e2f71 FOREIGN KEY (snippet_id) REFERENCES public.snippets (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.gpg_key_subkeys ADD CONSTRAINT fk_rails_8b2c90b046 FOREIGN KEY (gpg_key_id) REFERENCES public.gpg_keys (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.allowed_email_domains ADD CONSTRAINT fk_rails_8b5da859f9 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.cluster_projects ADD CONSTRAINT fk_rails_8b8c5caf07 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_pages_metadata ADD CONSTRAINT fk_rails_8c28a61485 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.packages_conan_metadata ADD CONSTRAINT fk_rails_8c68cfec8b FOREIGN KEY (package_id) REFERENCES public.packages_packages (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_feedback ADD CONSTRAINT fk_rails_8c77e5891a FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.approval_merge_request_rules_approved_approvers ADD CONSTRAINT fk_rails_8dc94cff4d FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_branch_push_access_levels ADD CONSTRAINT fk_rails_8dcb712d65 FOREIGN KEY (user_id) REFERENCES public.users (id);

ALTER TABLE ONLY public.design_user_mentions ADD CONSTRAINT fk_rails_8de8c6d632 FOREIGN KEY (note_id) REFERENCES public.notes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters_kubernetes_namespaces ADD CONSTRAINT fk_rails_8df789f3ab FOREIGN KEY (environment_id) REFERENCES public.environments (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.project_daily_statistics ADD CONSTRAINT fk_rails_8e549b272d FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_pipelines_config ADD CONSTRAINT fk_rails_906c9a2533 FOREIGN KEY (pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_project_rules_groups ADD CONSTRAINT fk_rails_9071e863d1 FOREIGN KEY (approval_project_rule_id) REFERENCES public.approval_project_rules (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_occurrences ADD CONSTRAINT fk_rails_90fed4faba FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_reset_checksum_events ADD CONSTRAINT fk_rails_910a06f12b FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_error_tracking_settings ADD CONSTRAINT fk_rails_910a2b8bd9 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.list_user_preferences ADD CONSTRAINT fk_rails_916d72cafd FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.board_labels ADD CONSTRAINT fk_rails_9374a16edd FOREIGN KEY (board_id) REFERENCES public.boards (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.scim_identities ADD CONSTRAINT fk_rails_9421a0bffb FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.packages_dependency_links ADD CONSTRAINT fk_rails_96ef1c00d3 FOREIGN KEY (package_id) REFERENCES public.packages_packages (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.resource_label_events ADD CONSTRAINT fk_rails_9851a00031 FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_job_artifacts ADD CONSTRAINT fk_rails_9862d392f9 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.board_project_recent_visits ADD CONSTRAINT fk_rails_98f8843922 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters_kubernetes_namespaces ADD CONSTRAINT fk_rails_98fe21e486 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.vulnerability_exports ADD CONSTRAINT fk_rails_9aff2c3b45 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.users_ops_dashboard_projects ADD CONSTRAINT fk_rails_9b4ebf005b FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_incident_management_settings ADD CONSTRAINT fk_rails_9c2ea1b7dd FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.gpg_keys ADD CONSTRAINT fk_rails_9d1f5d8719 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.analytics_language_trend_repository_languages ADD CONSTRAINT fk_rails_9d851d566c FOREIGN KEY (programming_language_id) REFERENCES public.programming_languages (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.badges ADD CONSTRAINT fk_rails_9df4a56538 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters_applications_cert_managers ADD CONSTRAINT fk_rails_9e4f2cb4b2 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.resource_milestone_events ADD CONSTRAINT fk_rails_a006df5590 FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.namespace_root_storage_statistics ADD CONSTRAINT fk_rails_a0702c430b FOREIGN KEY (namespace_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_aliases ADD CONSTRAINT fk_rails_a1804f74a7 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_user_mentions ADD CONSTRAINT fk_rails_a18600f210 FOREIGN KEY (note_id) REFERENCES public.notes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.todos ADD CONSTRAINT fk_rails_a27c483435 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.jira_tracker_data ADD CONSTRAINT fk_rails_a299066916 FOREIGN KEY (service_id) REFERENCES public.services (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_environments ADD CONSTRAINT fk_rails_a354313d11 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.jira_connect_subscriptions ADD CONSTRAINT fk_rails_a3c10bcf7d FOREIGN KEY (namespace_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.fork_network_members ADD CONSTRAINT fk_rails_a40860a1ca FOREIGN KEY (fork_network_id) REFERENCES public.fork_networks (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.operations_feature_flag_scopes ADD CONSTRAINT fk_rails_a50a04d0a4 FOREIGN KEY (feature_flag_id) REFERENCES public.operations_feature_flags (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.cluster_projects ADD CONSTRAINT fk_rails_a5a958bca1 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.commit_user_mentions ADD CONSTRAINT fk_rails_a6760813e0 FOREIGN KEY (note_id) REFERENCES public.notes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_identifiers ADD CONSTRAINT fk_rails_a67a16c885 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.user_preferences ADD CONSTRAINT fk_rails_a69bfcfd81 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.sentry_issues ADD CONSTRAINT fk_rails_a6a9612965 FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.repository_languages ADD CONSTRAINT fk_rails_a750ec87a8 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.resource_milestone_events ADD CONSTRAINT fk_rails_a788026e85 FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.term_agreements ADD CONSTRAINT fk_rails_a88721bcdf FOREIGN KEY (term_id) REFERENCES public.application_setting_terms (id);

ALTER TABLE ONLY public.merge_request_user_mentions ADD CONSTRAINT fk_rails_aa1b2961b1 FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.x509_commit_signatures ADD CONSTRAINT fk_rails_ab07452314 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_build_trace_sections ADD CONSTRAINT fk_rails_ab7c104e26 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.clusters ADD CONSTRAINT fk_rails_ac3a663d79 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.analytics_cycle_analytics_group_stages ADD CONSTRAINT fk_rails_ae5da3409b FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.pool_repositories ADD CONSTRAINT fk_rails_af3f8c5d62 FOREIGN KEY (shard_id) REFERENCES public.shards (id) ON DELETE RESTRICT;

ALTER TABLE ONLY public.resource_label_events ADD CONSTRAINT fk_rails_b126799f57 FOREIGN KEY (label_id) REFERENCES public.labels (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.packages_build_infos ADD CONSTRAINT fk_rails_b18868292d FOREIGN KEY (package_id) REFERENCES public.packages_packages (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_trains ADD CONSTRAINT fk_rails_b29261ce31 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.board_project_recent_visits ADD CONSTRAINT fk_rails_b315dd0c80 FOREIGN KEY (board_id) REFERENCES public.boards (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issues_prometheus_alert_events ADD CONSTRAINT fk_rails_b32edb790f FOREIGN KEY (prometheus_alert_event_id) REFERENCES public.prometheus_alert_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_trains ADD CONSTRAINT fk_rails_b374b5225d FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.application_settings ADD CONSTRAINT fk_rails_b53e481273 FOREIGN KEY (custom_project_templates_group_id) REFERENCES public.namespaces (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.namespace_aggregation_schedules ADD CONSTRAINT fk_rails_b565c8d16c FOREIGN KEY (namespace_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_project_rules_protected_branches ADD CONSTRAINT fk_rails_b7567b031b FOREIGN KEY (protected_branch_id) REFERENCES public.protected_branches (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.alerts_service_data ADD CONSTRAINT fk_rails_b93215a42c FOREIGN KEY (service_id) REFERENCES public.services (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_trains ADD CONSTRAINT fk_rails_b9d67af01d FOREIGN KEY (target_project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_project_rules_users ADD CONSTRAINT fk_rails_b9e9394efb FOREIGN KEY (approval_project_rule_id) REFERENCES public.approval_project_rules (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.lists ADD CONSTRAINT fk_rails_baed5f39b7 FOREIGN KEY (milestone_id) REFERENCES public.milestones (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_merge_request_rules_users ADD CONSTRAINT fk_rails_bc8972fa55 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.external_pull_requests ADD CONSTRAINT fk_rails_bcae9b5c7b FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.elasticsearch_indexed_projects ADD CONSTRAINT fk_rails_bd13bbdc3d FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.elasticsearch_indexed_namespaces ADD CONSTRAINT fk_rails_bdcf044f37 FOREIGN KEY (namespace_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_occurrence_identifiers ADD CONSTRAINT fk_rails_be2e49e1d0 FOREIGN KEY (identifier_id) REFERENCES public.vulnerability_identifiers (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_occurrences ADD CONSTRAINT fk_rails_bf5b788ca7 FOREIGN KEY (scanner_id) REFERENCES public.vulnerability_scanners (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.resource_weight_events ADD CONSTRAINT fk_rails_bfc406b47c FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.design_management_designs ADD CONSTRAINT fk_rails_bfe283ec3c FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.u2f_registrations ADD CONSTRAINT fk_rails_bfe6a84544 FOREIGN KEY (user_id) REFERENCES public.users (id);

ALTER TABLE ONLY public.serverless_domain_cluster ADD CONSTRAINT fk_rails_c09009dee1 FOREIGN KEY (pages_domain_id) REFERENCES public.pages_domains (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.labels ADD CONSTRAINT fk_rails_c1ac5161d8 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_feature_usages ADD CONSTRAINT fk_rails_c22a50024b FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.user_canonical_emails ADD CONSTRAINT fk_rails_c2bd828b51 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_repositories ADD CONSTRAINT fk_rails_c3258dc63b FOREIGN KEY (shard_id) REFERENCES public.shards (id) ON DELETE RESTRICT;

ALTER TABLE ONLY public.merge_request_user_mentions ADD CONSTRAINT fk_rails_c440b9ea31 FOREIGN KEY (note_id) REFERENCES public.notes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_job_artifacts ADD CONSTRAINT fk_rails_c5137cb2c1 FOREIGN KEY (job_id) REFERENCES public.ci_builds (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_settings ADD CONSTRAINT fk_rails_c6df6e6328 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.container_expiration_policies ADD CONSTRAINT fk_rails_c7360f09ad FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.wiki_page_meta ADD CONSTRAINT fk_rails_c7a0c59cf1 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.scim_oauth_access_tokens ADD CONSTRAINT fk_rails_c84404fb6c FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_occurrences ADD CONSTRAINT fk_rails_c8661a61eb FOREIGN KEY (primary_identifier_id) REFERENCES public.vulnerability_identifiers (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_export_jobs ADD CONSTRAINT fk_rails_c88d8db2e1 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.resource_milestone_events ADD CONSTRAINT fk_rails_c940fb9fc5 FOREIGN KEY (milestone_id) REFERENCES public.milestones (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.gpg_signatures ADD CONSTRAINT fk_rails_c97176f5f7 FOREIGN KEY (gpg_key_id) REFERENCES public.gpg_keys (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.board_group_recent_visits ADD CONSTRAINT fk_rails_ca04c38720 FOREIGN KEY (board_id) REFERENCES public.boards (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_daily_report_results ADD CONSTRAINT fk_rails_cc5caec7d9 FOREIGN KEY (last_pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issues_self_managed_prometheus_alert_events ADD CONSTRAINT fk_rails_cc5d88bbb0 FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issue_tracker_data ADD CONSTRAINT fk_rails_ccc0840427 FOREIGN KEY (service_id) REFERENCES public.services (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.resource_milestone_events ADD CONSTRAINT fk_rails_cedf8cce4d FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.epic_metrics ADD CONSTRAINT fk_rails_d071904753 FOREIGN KEY (epic_id) REFERENCES public.epics (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.subscriptions ADD CONSTRAINT fk_rails_d0c8bda804 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.operations_strategies ADD CONSTRAINT fk_rails_d183b6e6dd FOREIGN KEY (feature_flag_id) REFERENCES public.operations_feature_flags (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.pool_repositories ADD CONSTRAINT fk_rails_d2711daad4 FOREIGN KEY (source_project_id) REFERENCES public.projects (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.group_group_links ADD CONSTRAINT fk_rails_d3a0488427 FOREIGN KEY (shared_group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_issue_links ADD CONSTRAINT fk_rails_d459c19036 FOREIGN KEY (vulnerability_id) REFERENCES public.vulnerabilities (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.geo_hashed_storage_attachments_events ADD CONSTRAINT fk_rails_d496b088e9 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.dependency_proxy_blobs ADD CONSTRAINT fk_rails_db58bbc5d7 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issues_prometheus_alert_events ADD CONSTRAINT fk_rails_db5b756534 FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_occurrence_pipelines ADD CONSTRAINT fk_rails_dc3ae04693 FOREIGN KEY (occurrence_id) REFERENCES public.vulnerability_occurrences (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.deployment_merge_requests ADD CONSTRAINT fk_rails_dcbce9f4df FOREIGN KEY (deployment_id) REFERENCES public.deployments (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.user_callouts ADD CONSTRAINT fk_rails_ddfdd80f3d FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_feedback ADD CONSTRAINT fk_rails_debd54e456 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.analytics_cycle_analytics_group_stages ADD CONSTRAINT fk_rails_dfb37c880d FOREIGN KEY (end_event_label_id) REFERENCES public.labels (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.label_priorities ADD CONSTRAINT fk_rails_e161058b0f FOREIGN KEY (label_id) REFERENCES public.labels (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.packages_packages ADD CONSTRAINT fk_rails_e1ac527425 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.cluster_platforms_kubernetes ADD CONSTRAINT fk_rails_e1e2cf841a FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_builds_metadata ADD CONSTRAINT fk_rails_e20479742e FOREIGN KEY (build_id) REFERENCES public.ci_builds (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_occurrence_identifiers ADD CONSTRAINT fk_rails_e4ef6d027c FOREIGN KEY (occurrence_id) REFERENCES public.vulnerability_occurrences (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.serverless_domain_cluster ADD CONSTRAINT fk_rails_e59e868733 FOREIGN KEY (clusters_applications_knative_id) REFERENCES public.clusters_applications_knative (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_merge_request_rule_sources ADD CONSTRAINT fk_rails_e605a04f76 FOREIGN KEY (approval_merge_request_rule_id) REFERENCES public.approval_merge_request_rules (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.prometheus_alerts ADD CONSTRAINT fk_rails_e6351447ec FOREIGN KEY (prometheus_metric_id) REFERENCES public.prometheus_metrics (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_metrics ADD CONSTRAINT fk_rails_e6d7c24d1b FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.draft_notes ADD CONSTRAINT fk_rails_e753681674 FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.description_versions ADD CONSTRAINT fk_rails_e8f4caf9c7 FOREIGN KEY (epic_id) REFERENCES public.epics (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.vulnerability_issue_links ADD CONSTRAINT fk_rails_e9180d534b FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_request_blocks ADD CONSTRAINT fk_rails_e9387863bc FOREIGN KEY (blocking_merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.protected_branch_unprotect_access_levels ADD CONSTRAINT fk_rails_e9eb8dc025 FOREIGN KEY (protected_branch_id) REFERENCES public.protected_branches (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_daily_report_results ADD CONSTRAINT fk_rails_ebc2931b90 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.cluster_providers_aws ADD CONSTRAINT fk_rails_ed1fdfaeb2 FOREIGN KEY (created_by_user_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.label_priorities ADD CONSTRAINT fk_rails_ef916d14fa FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.fork_network_members ADD CONSTRAINT fk_rails_efccadc4ec FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.prometheus_alerts ADD CONSTRAINT fk_rails_f0e8db86aa FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.import_export_uploads ADD CONSTRAINT fk_rails_f129140f9e FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.jira_connect_subscriptions ADD CONSTRAINT fk_rails_f1d617343f FOREIGN KEY (jira_connect_installation_id) REFERENCES public.jira_connect_installations (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.requirements ADD CONSTRAINT fk_rails_f212e67e63 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.snippet_repositories ADD CONSTRAINT fk_rails_f21f899728 FOREIGN KEY (shard_id) REFERENCES public.shards (id) ON DELETE RESTRICT;

ALTER TABLE ONLY public.ci_pipeline_chat_data ADD CONSTRAINT fk_rails_f300456b63 FOREIGN KEY (chat_name_id) REFERENCES public.chat_names (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.approval_project_rules_users ADD CONSTRAINT fk_rails_f365da8250 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.insights ADD CONSTRAINT fk_rails_f36fda3932 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.board_group_recent_visits ADD CONSTRAINT fk_rails_f410736518 FOREIGN KEY (group_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.design_user_mentions ADD CONSTRAINT fk_rails_f7075a53c1 FOREIGN KEY (design_id) REFERENCES public.design_management_designs (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.internal_ids ADD CONSTRAINT fk_rails_f7d46b66c6 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.issues_self_managed_prometheus_alert_events ADD CONSTRAINT fk_rails_f7db2d72eb FOREIGN KEY (self_managed_prometheus_alert_event_id) REFERENCES public.self_managed_prometheus_alert_events (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_requests_closing_issues ADD CONSTRAINT fk_rails_f8540692be FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.ci_build_trace_section_names ADD CONSTRAINT fk_rails_f8cd72cd26 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.merge_trains ADD CONSTRAINT fk_rails_f90820cb08 FOREIGN KEY (pipeline_id) REFERENCES public.ci_pipelines (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_runner_namespaces ADD CONSTRAINT fk_rails_f9d9ed3308 FOREIGN KEY (namespace_id) REFERENCES public.namespaces (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.board_project_recent_visits ADD CONSTRAINT fk_rails_fb6fc419cb FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.serverless_domain_cluster ADD CONSTRAINT fk_rails_fbdba67eb1 FOREIGN KEY (creator_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_job_variables ADD CONSTRAINT fk_rails_fbf3b34792 FOREIGN KEY (job_id) REFERENCES public.ci_builds (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.cluster_groups ADD CONSTRAINT fk_rails_fdb8648a96 FOREIGN KEY (cluster_id) REFERENCES public.clusters (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.project_tracing_settings ADD CONSTRAINT fk_rails_fe56f57fc6 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.resource_label_events ADD CONSTRAINT fk_rails_fe91ece594 FOREIGN KEY (user_id) REFERENCES public.users (id) ON DELETE SET NULL;

ALTER TABLE ONLY public.ci_builds_metadata ADD CONSTRAINT fk_rails_ffcf702a02 FOREIGN KEY (project_id) REFERENCES public.projects (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.timelogs ADD CONSTRAINT fk_timelogs_issues_issue_id FOREIGN KEY (issue_id) REFERENCES public.issues (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.timelogs ADD CONSTRAINT fk_timelogs_merge_requests_merge_request_id FOREIGN KEY (merge_request_id) REFERENCES public.merge_requests (id) ON DELETE CASCADE;

-- WeTune schema patches
ALTER TABLE "vulnerability_occurrences" ALTER COLUMN "vulnerability_id" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "description" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "last_activity_at" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "last_repository_check_at" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "last_repository_check_failed" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "last_repository_updated_at" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "marked_for_deletion_at" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "creator_id" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "mirror_last_successful_update_at" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "mirror_user_id" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "path" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "pending_delete" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "runners_token" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "runners_token_encrypted" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "updated_at" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "marked_for_deletion_by_user_id" SET NOT NULL;
ALTER TABLE "projects" ALTER COLUMN "pool_repository_id" SET NOT NULL;
ALTER TABLE "vulnerability_feedback" ALTER COLUMN "merge_request_id" SET NOT NULL;
ALTER TABLE "vulnerability_feedback" ALTER COLUMN "comment_author_id" SET NOT NULL;
ALTER TABLE "vulnerability_feedback" ALTER COLUMN "pipeline_id" SET NOT NULL;
ALTER TABLE "vulnerability_feedback" ALTER COLUMN "issue_id" SET NOT NULL;
ALTER TABLE "protected_environment_deploy_access_levels" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "protected_environment_deploy_access_levels" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "events" ALTER COLUMN "target_type" SET NOT NULL;
ALTER TABLE "events" ALTER COLUMN "target_id" SET NOT NULL;
ALTER TABLE "events" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "events" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "push_rules" ALTER COLUMN "is_sample" SET NOT NULL;
ALTER TABLE "push_rules" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "gpg_key_subkeys" ALTER COLUMN "fingerprint" SET NOT NULL;
ALTER TABLE "gpg_key_subkeys" ALTER COLUMN "keyid" SET NOT NULL;
ALTER TABLE "labels" ALTER COLUMN "template" SET NOT NULL;
ALTER TABLE "labels" ALTER COLUMN "title" SET NOT NULL;
ALTER TABLE "labels" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "labels" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "labels" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "tags" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "project_mirror_data" ALTER COLUMN "next_execution_timestamp" SET NOT NULL;
ALTER TABLE "project_mirror_data" ALTER COLUMN "last_successful_update_at" SET NOT NULL;
ALTER TABLE "project_mirror_data" ALTER COLUMN "last_update_at" SET NOT NULL;
ALTER TABLE "project_mirror_data" ALTER COLUMN "status" SET NOT NULL;
ALTER TABLE "prometheus_alert_events" ALTER COLUMN "payload_key" SET NOT NULL;
ALTER TABLE "prometheus_alert_events" ALTER COLUMN "status" SET NOT NULL;
ALTER TABLE "plans" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "iid" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "sha" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "source" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "config_source" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "updated_at" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "ref" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "status" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "external_pull_request_id" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "auto_canceled_by_id" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "pipeline_schedule_id" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "ci_pipelines" ALTER COLUMN "merge_request_id" SET NOT NULL;
ALTER TABLE "issue_metrics" ALTER COLUMN "first_mentioned_in_commit_at" SET NOT NULL;
ALTER TABLE "issue_metrics" ALTER COLUMN "first_associated_with_milestone_at" SET NOT NULL;
ALTER TABLE "issue_metrics" ALTER COLUMN "first_added_to_board_at" SET NOT NULL;
ALTER TABLE "approvers" ALTER COLUMN "target_type" SET NOT NULL;
ALTER TABLE "pool_repositories" ALTER COLUMN "disk_path" SET NOT NULL;
ALTER TABLE "pool_repositories" ALTER COLUMN "source_project_id" SET NOT NULL;
ALTER TABLE "prometheus_metrics" ALTER COLUMN "identifier" SET NOT NULL;
ALTER TABLE "prometheus_metrics" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "identities" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "identities" ALTER COLUMN "saml_provider_id" SET NOT NULL;
ALTER TABLE "snippets" ALTER COLUMN "content" SET NOT NULL;
ALTER TABLE "snippets" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "snippets" ALTER COLUMN "description" SET NOT NULL;
ALTER TABLE "snippets" ALTER COLUMN "file_name" SET NOT NULL;
ALTER TABLE "snippets" ALTER COLUMN "title" SET NOT NULL;
ALTER TABLE "snippets" ALTER COLUMN "updated_at" SET NOT NULL;
ALTER TABLE "snippets" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "container_expiration_policies" ALTER COLUMN "next_run_at" SET NOT NULL;
ALTER TABLE "operations_feature_flags_clients" ALTER COLUMN "token_encrypted" SET NOT NULL;
ALTER TABLE "elasticsearch_indexed_projects" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "packages_packages" ALTER COLUMN "version" SET NOT NULL;
ALTER TABLE "gpg_signatures" ALTER COLUMN "commit_sha" SET NOT NULL;
ALTER TABLE "gpg_signatures" ALTER COLUMN "gpg_key_primary_keyid" SET NOT NULL;
ALTER TABLE "gpg_signatures" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "gpg_signatures" ALTER COLUMN "gpg_key_subkey_id" SET NOT NULL;
ALTER TABLE "gpg_signatures" ALTER COLUMN "gpg_key_id" SET NOT NULL;
ALTER TABLE "user_highest_roles" ALTER COLUMN "highest_access_level" SET NOT NULL;
ALTER TABLE "ci_runner_projects" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "clusters_applications_jupyter" ALTER COLUMN "oauth_application_id" SET NOT NULL;
ALTER TABLE "project_import_data" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "services" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "services" ALTER COLUMN "template" SET NOT NULL;
ALTER TABLE "services" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "releases" ALTER COLUMN "tag" SET NOT NULL;
ALTER TABLE "releases" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "releases" ALTER COLUMN "author_id" SET NOT NULL;
ALTER TABLE "analytics_cycle_analytics_group_stages" ALTER COLUMN "relative_position" SET NOT NULL;
ALTER TABLE "analytics_cycle_analytics_group_stages" ALTER COLUMN "start_event_label_id" SET NOT NULL;
ALTER TABLE "analytics_cycle_analytics_group_stages" ALTER COLUMN "end_event_label_id" SET NOT NULL;
ALTER TABLE "todos" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "todos" ALTER COLUMN "commit_id" SET NOT NULL;
ALTER TABLE "todos" ALTER COLUMN "target_id" SET NOT NULL;
ALTER TABLE "todos" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "todos" ALTER COLUMN "note_id" SET NOT NULL;
ALTER TABLE "todos" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "oauth_applications" ALTER COLUMN "owner_id" SET NOT NULL;
ALTER TABLE "oauth_applications" ALTER COLUMN "owner_type" SET NOT NULL;
ALTER TABLE "subscriptions" ALTER COLUMN "subscribable_id" SET NOT NULL;
ALTER TABLE "subscriptions" ALTER COLUMN "subscribable_type" SET NOT NULL;
ALTER TABLE "subscriptions" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "subscriptions" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "notes" ALTER COLUMN "author_id" SET NOT NULL;
ALTER TABLE "notes" ALTER COLUMN "commit_id" SET NOT NULL;
ALTER TABLE "notes" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "notes" ALTER COLUMN "discussion_id" SET NOT NULL;
ALTER TABLE "notes" ALTER COLUMN "line_code" SET NOT NULL;
ALTER TABLE "notes" ALTER COLUMN "note" SET NOT NULL;
ALTER TABLE "notes" ALTER COLUMN "noteable_id" SET NOT NULL;
ALTER TABLE "notes" ALTER COLUMN "noteable_type" SET NOT NULL;
ALTER TABLE "notes" ALTER COLUMN "review_id" SET NOT NULL;
ALTER TABLE "notes" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "oauth_access_tokens" ALTER COLUMN "application_id" SET NOT NULL;
ALTER TABLE "oauth_access_tokens" ALTER COLUMN "refresh_token" SET NOT NULL;
ALTER TABLE "oauth_access_tokens" ALTER COLUMN "resource_owner_id" SET NOT NULL;
ALTER TABLE "ci_job_artifacts" ALTER COLUMN "expire_at" SET NOT NULL;
ALTER TABLE "ci_job_artifacts" ALTER COLUMN "file_store" SET NOT NULL;
ALTER TABLE "protected_branch_push_access_levels" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "protected_branch_push_access_levels" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "issue_user_mentions" ALTER COLUMN "note_id" SET NOT NULL;
ALTER TABLE "fork_networks" ALTER COLUMN "root_project_id" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "artifacts_expire_at" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "ref" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "protected" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "queued_at" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "runner_id" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "token" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "token_encrypted" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "updated_at" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "status" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "scheduled_at" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "stage_idx" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "stage_id" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "resource_group_id" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "upstream_pipeline_id" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "auto_canceled_by_id" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "ci_builds" ALTER COLUMN "commit_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "hashed_storage_attachments_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "job_artifact_deleted_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "hashed_storage_migrated_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "cache_invalidation_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "repositories_changed_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "container_repository_updated_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "repository_updated_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "repository_renamed_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "repository_created_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "upload_deleted_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "repository_deleted_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "reset_checksum_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "lfs_object_deleted_event_id" SET NOT NULL;
ALTER TABLE "geo_event_log" ALTER COLUMN "geo_event_id" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "bot_type" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "confirmation_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "feed_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "ghost" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "group_view" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "incoming_email_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "reset_password_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "state" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "static_object_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "unconfirmed_email" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "unlock_token" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "user_type" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "username" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "accepted_term_id" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "managing_group_id" SET NOT NULL;
ALTER TABLE "ci_trigger_requests" ALTER COLUMN "commit_id" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "tag_id" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "taggable_id" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "taggable_type" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "context" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "tagger_id" SET NOT NULL;
ALTER TABLE "taggings" ALTER COLUMN "tagger_type" SET NOT NULL;
ALTER TABLE "draft_notes" ALTER COLUMN "discussion_id" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "merge_jid" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "description" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "lock_version" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "iid" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "title" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "merge_commit_sha" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "latest_merge_request_diff_id" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "source_project_id" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "assignee_id" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "updated_by_id" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "milestone_id" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "merge_user_id" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "author_id" SET NOT NULL;
ALTER TABLE "merge_requests" ALTER COLUMN "head_pipeline_id" SET NOT NULL;
ALTER TABLE "notification_settings" ALTER COLUMN "source_id" SET NOT NULL;
ALTER TABLE "notification_settings" ALTER COLUMN "source_type" SET NOT NULL;
ALTER TABLE "protected_branch_merge_access_levels" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "protected_branch_merge_access_levels" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "import_export_uploads" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "import_export_uploads" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "emails" ALTER COLUMN "confirmation_token" SET NOT NULL;
ALTER TABLE "board_group_recent_visits" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "board_group_recent_visits" ALTER COLUMN "board_id" SET NOT NULL;
ALTER TABLE "board_group_recent_visits" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "deployment_clusters" ALTER COLUMN "kubernetes_namespace" SET NOT NULL;
ALTER TABLE "resource_label_events" ALTER COLUMN "issue_id" SET NOT NULL;
ALTER TABLE "resource_label_events" ALTER COLUMN "epic_id" SET NOT NULL;
ALTER TABLE "resource_label_events" ALTER COLUMN "merge_request_id" SET NOT NULL;
ALTER TABLE "resource_label_events" ALTER COLUMN "label_id" SET NOT NULL;
ALTER TABLE "resource_label_events" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "web_hooks" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "web_hooks" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "web_hooks" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "application_settings" ALTER COLUMN "usage_stats_set_by_user_id" SET NOT NULL;
ALTER TABLE "application_settings" ALTER COLUMN "instance_administrators_group_id" SET NOT NULL;
ALTER TABLE "application_settings" ALTER COLUMN "file_template_project_id" SET NOT NULL;
ALTER TABLE "application_settings" ALTER COLUMN "instance_administration_project_id" SET NOT NULL;
ALTER TABLE "application_settings" ALTER COLUMN "custom_project_templates_group_id" SET NOT NULL;
ALTER TABLE "clusters" ALTER COLUMN "enabled" SET NOT NULL;
ALTER TABLE "clusters" ALTER COLUMN "provider_type" SET NOT NULL;
ALTER TABLE "clusters" ALTER COLUMN "management_project_id" SET NOT NULL;
ALTER TABLE "clusters" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "epic_user_mentions" ALTER COLUMN "note_id" SET NOT NULL;
ALTER TABLE "project_daily_statistics" ALTER COLUMN "date" SET NOT NULL;
ALTER TABLE "uploads" ALTER COLUMN "checksum" SET NOT NULL;
ALTER TABLE "uploads" ALTER COLUMN "model_id" SET NOT NULL;
ALTER TABLE "uploads" ALTER COLUMN "model_type" SET NOT NULL;
ALTER TABLE "uploads" ALTER COLUMN "store" SET NOT NULL;
ALTER TABLE "ci_pipeline_schedules" ALTER COLUMN "next_run_at" SET NOT NULL;
ALTER TABLE "ci_pipeline_schedules" ALTER COLUMN "active" SET NOT NULL;
ALTER TABLE "ci_pipeline_schedules" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "ci_pipeline_schedules" ALTER COLUMN "owner_id" SET NOT NULL;
ALTER TABLE "resource_weight_events" ALTER COLUMN "weight" SET NOT NULL;
ALTER TABLE "ci_triggers" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "vulnerabilities" ALTER COLUMN "last_edited_by_id" SET NOT NULL;
ALTER TABLE "vulnerabilities" ALTER COLUMN "milestone_id" SET NOT NULL;
ALTER TABLE "vulnerabilities" ALTER COLUMN "epic_id" SET NOT NULL;
ALTER TABLE "vulnerabilities" ALTER COLUMN "dismissed_by_id" SET NOT NULL;
ALTER TABLE "vulnerabilities" ALTER COLUMN "resolved_by_id" SET NOT NULL;
ALTER TABLE "vulnerabilities" ALTER COLUMN "updated_by_id" SET NOT NULL;
ALTER TABLE "vulnerabilities" ALTER COLUMN "due_date_sourcing_milestone_id" SET NOT NULL;
ALTER TABLE "vulnerabilities" ALTER COLUMN "start_date_sourcing_milestone_id" SET NOT NULL;
ALTER TABLE "vulnerabilities" ALTER COLUMN "confirmed_by_id" SET NOT NULL;
ALTER TABLE "gitlab_subscriptions" ALTER COLUMN "hosted_plan_id" SET NOT NULL;
ALTER TABLE "gitlab_subscriptions" ALTER COLUMN "namespace_id" SET NOT NULL;
ALTER TABLE "milestones" ALTER COLUMN "description" SET NOT NULL;
ALTER TABLE "milestones" ALTER COLUMN "due_date" SET NOT NULL;
ALTER TABLE "milestones" ALTER COLUMN "iid" SET NOT NULL;
ALTER TABLE "milestones" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "milestones" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "ci_resources" ALTER COLUMN "build_id" SET NOT NULL;
ALTER TABLE "clusters_applications_runners" ALTER COLUMN "runner_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "due_date" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "description" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "lock_version" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "external_key" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "iid" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "relative_position" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "title" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "updated_at" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "author_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "milestone_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "duplicated_to_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "moved_to_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "closed_by_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "promoted_to_epic_id" SET NOT NULL;
ALTER TABLE "issues" ALTER COLUMN "updated_by_id" SET NOT NULL;
ALTER TABLE "protected_branch_unprotect_access_levels" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "protected_branch_unprotect_access_levels" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "lfs_objects" ALTER COLUMN "file_store" SET NOT NULL;
ALTER TABLE "import_failures" ALTER COLUMN "correlation_id_value" SET NOT NULL;
ALTER TABLE "import_failures" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "import_failures" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "label_links" ALTER COLUMN "target_id" SET NOT NULL;
ALTER TABLE "label_links" ALTER COLUMN "target_type" SET NOT NULL;
ALTER TABLE "label_links" ALTER COLUMN "label_id" SET NOT NULL;
ALTER TABLE "u2f_registrations" ALTER COLUMN "key_handle" SET NOT NULL;
ALTER TABLE "u2f_registrations" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "ci_runners" ALTER COLUMN "contacted_at" SET NOT NULL;
ALTER TABLE "ci_runners" ALTER COLUMN "is_shared" SET NOT NULL;
ALTER TABLE "ci_runners" ALTER COLUMN "token" SET NOT NULL;
ALTER TABLE "ci_runners" ALTER COLUMN "token_encrypted" SET NOT NULL;
ALTER TABLE "ci_stages" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "ci_stages" ALTER COLUMN "position" SET NOT NULL;
ALTER TABLE "ci_stages" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "ci_stages" ALTER COLUMN "pipeline_id" SET NOT NULL;
ALTER TABLE "elasticsearch_indexed_namespaces" ALTER COLUMN "namespace_id" SET NOT NULL;
ALTER TABLE "personal_access_tokens" ALTER COLUMN "expires_at" SET NOT NULL;
ALTER TABLE "personal_access_tokens" ALTER COLUMN "token_digest" SET NOT NULL;
ALTER TABLE "award_emoji" ALTER COLUMN "awardable_type" SET NOT NULL;
ALTER TABLE "award_emoji" ALTER COLUMN "awardable_id" SET NOT NULL;
ALTER TABLE "award_emoji" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "award_emoji" ALTER COLUMN "name" SET NOT NULL;
ALTER TABLE "lfs_file_locks" ALTER COLUMN "path" SET NOT NULL;
ALTER TABLE "serverless_domain_cluster" ALTER COLUMN "creator_id" SET NOT NULL;
ALTER TABLE "system_note_metadata" ALTER COLUMN "description_version_id" SET NOT NULL;
ALTER TABLE "sent_notifications" ALTER COLUMN "noteable_id" SET NOT NULL;
ALTER TABLE "fork_network_members" ALTER COLUMN "forked_from_project_id" SET NOT NULL;
ALTER TABLE "self_managed_prometheus_alert_events" ALTER COLUMN "environment_id" SET NOT NULL;
ALTER TABLE "merge_request_context_commits" ALTER COLUMN "merge_request_id" SET NOT NULL;
ALTER TABLE "remote_mirrors" ALTER COLUMN "last_successful_update_at" SET NOT NULL;
ALTER TABLE "remote_mirrors" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "project_repository_states" ALTER COLUMN "last_repository_verification_ran_at" SET NOT NULL;
ALTER TABLE "project_repository_states" ALTER COLUMN "last_wiki_verification_ran_at" SET NOT NULL;
ALTER TABLE "project_repository_states" ALTER COLUMN "last_repository_verification_failure" SET NOT NULL;
ALTER TABLE "project_repository_states" ALTER COLUMN "last_wiki_verification_failure" SET NOT NULL;
ALTER TABLE "design_management_designs" ALTER COLUMN "issue_id" SET NOT NULL;
ALTER TABLE "board_project_recent_visits" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "board_project_recent_visits" ALTER COLUMN "board_id" SET NOT NULL;
ALTER TABLE "board_project_recent_visits" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "gpg_keys" ALTER COLUMN "fingerprint" SET NOT NULL;
ALTER TABLE "gpg_keys" ALTER COLUMN "primary_keyid" SET NOT NULL;
ALTER TABLE "gpg_keys" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "internal_ids" ALTER COLUMN "namespace_id" SET NOT NULL;
ALTER TABLE "internal_ids" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "environments" ALTER COLUMN "environment_type" SET NOT NULL;
ALTER TABLE "environments" ALTER COLUMN "auto_stop_at" SET NOT NULL;
ALTER TABLE "deployment_merge_requests" ALTER COLUMN "environment_id" SET NOT NULL;
ALTER TABLE "reviews" ALTER COLUMN "author_id" SET NOT NULL;
ALTER TABLE "snippet_user_mentions" ALTER COLUMN "note_id" SET NOT NULL;
ALTER TABLE "jira_connect_installations" ALTER COLUMN "client_key" SET NOT NULL;
ALTER TABLE "merge_trains" ALTER COLUMN "pipeline_id" SET NOT NULL;
ALTER TABLE "members" ALTER COLUMN "expires_at" SET NOT NULL;
ALTER TABLE "members" ALTER COLUMN "invite_email" SET NOT NULL;
ALTER TABLE "members" ALTER COLUMN "invite_token" SET NOT NULL;
ALTER TABLE "members" ALTER COLUMN "requested_at" SET NOT NULL;
ALTER TABLE "members" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "members" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "lists" ALTER COLUMN "label_id" SET NOT NULL;
ALTER TABLE "lists" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "lists" ALTER COLUMN "milestone_id" SET NOT NULL;
ALTER TABLE "merge_request_metrics" ALTER COLUMN "first_deployed_to_production_at" SET NOT NULL;
ALTER TABLE "merge_request_metrics" ALTER COLUMN "latest_closed_at" SET NOT NULL;
ALTER TABLE "merge_request_metrics" ALTER COLUMN "merged_at" SET NOT NULL;
ALTER TABLE "merge_request_metrics" ALTER COLUMN "merged_by_id" SET NOT NULL;
ALTER TABLE "merge_request_metrics" ALTER COLUMN "latest_closed_by_id" SET NOT NULL;
ALTER TABLE "merge_request_metrics" ALTER COLUMN "pipeline_id" SET NOT NULL;
ALTER TABLE "abuse_reports" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "ci_runner_namespaces" ALTER COLUMN "runner_id" SET NOT NULL;
ALTER TABLE "ci_runner_namespaces" ALTER COLUMN "namespace_id" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "ldap_sync_last_successful_update_at" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "ldap_sync_last_update_at" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "owner_id" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "runners_token" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "runners_token_encrypted" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "shared_runners_minutes_limit" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "extra_shared_runners_minutes_limit" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "trial_ends_on" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "file_template_project_id" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "custom_project_templates_group_id" SET NOT NULL;
ALTER TABLE "namespaces" ALTER COLUMN "plan_id" SET NOT NULL;
ALTER TABLE "pages_domains" ALTER COLUMN "certificate_valid_not_after" SET NOT NULL;
ALTER TABLE "pages_domains" ALTER COLUMN "domain" SET NOT NULL;
ALTER TABLE "pages_domains" ALTER COLUMN "remove_at" SET NOT NULL;
ALTER TABLE "pages_domains" ALTER COLUMN "verified_at" SET NOT NULL;
ALTER TABLE "pages_domains" ALTER COLUMN "enabled_until" SET NOT NULL;
ALTER TABLE "pages_domains" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "vulnerability_user_mentions" ALTER COLUMN "note_id" SET NOT NULL;
ALTER TABLE "ci_sources_pipelines" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "ci_sources_pipelines" ALTER COLUMN "source_project_id" SET NOT NULL;
ALTER TABLE "ci_sources_pipelines" ALTER COLUMN "source_job_id" SET NOT NULL;
ALTER TABLE "ci_sources_pipelines" ALTER COLUMN "source_pipeline_id" SET NOT NULL;
ALTER TABLE "ci_sources_pipelines" ALTER COLUMN "pipeline_id" SET NOT NULL;
ALTER TABLE "timelogs" ALTER COLUMN "spent_at" SET NOT NULL;
ALTER TABLE "timelogs" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "timelogs" ALTER COLUMN "issue_id" SET NOT NULL;
ALTER TABLE "timelogs" ALTER COLUMN "merge_request_id" SET NOT NULL;
ALTER TABLE "keys" ALTER COLUMN "fingerprint" SET NOT NULL;
ALTER TABLE "keys" ALTER COLUMN "fingerprint_sha256" SET NOT NULL;
ALTER TABLE "keys" ALTER COLUMN "last_used_at" SET NOT NULL;
ALTER TABLE "keys" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "keys" ALTER COLUMN "type" SET NOT NULL;
ALTER TABLE "clusters_kubernetes_namespaces" ALTER COLUMN "cluster_project_id" SET NOT NULL;
ALTER TABLE "clusters_kubernetes_namespaces" ALTER COLUMN "environment_id" SET NOT NULL;
ALTER TABLE "clusters_kubernetes_namespaces" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "boards" ALTER COLUMN "milestone_id" SET NOT NULL;
ALTER TABLE "boards" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "boards" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "analytics_cycle_analytics_project_stages" ALTER COLUMN "relative_position" SET NOT NULL;
ALTER TABLE "analytics_cycle_analytics_project_stages" ALTER COLUMN "start_event_label_id" SET NOT NULL;
ALTER TABLE "analytics_cycle_analytics_project_stages" ALTER COLUMN "end_event_label_id" SET NOT NULL;
ALTER TABLE "epics" ALTER COLUMN "end_date" SET NOT NULL;
ALTER TABLE "epics" ALTER COLUMN "external_key" SET NOT NULL;
ALTER TABLE "epics" ALTER COLUMN "lock_version" SET NOT NULL;
ALTER TABLE "epics" ALTER COLUMN "start_date" SET NOT NULL;
ALTER TABLE "epics" ALTER COLUMN "due_date_sourcing_epic_id" SET NOT NULL;
ALTER TABLE "epics" ALTER COLUMN "start_date_sourcing_milestone_id" SET NOT NULL;
ALTER TABLE "epics" ALTER COLUMN "parent_id" SET NOT NULL;
ALTER TABLE "epics" ALTER COLUMN "due_date_sourcing_milestone_id" SET NOT NULL;
ALTER TABLE "epics" ALTER COLUMN "start_date_sourcing_epic_id" SET NOT NULL;
ALTER TABLE "epics" ALTER COLUMN "closed_by_id" SET NOT NULL;
ALTER TABLE "epics" ALTER COLUMN "assignee_id" SET NOT NULL;
ALTER TABLE "description_versions" ALTER COLUMN "merge_request_id" SET NOT NULL;
ALTER TABLE "description_versions" ALTER COLUMN "issue_id" SET NOT NULL;
ALTER TABLE "description_versions" ALTER COLUMN "epic_id" SET NOT NULL;
ALTER TABLE "geo_nodes" ALTER COLUMN "access_key" SET NOT NULL;
ALTER TABLE "project_feature_usages" ALTER COLUMN "jira_dvcs_cloud_last_sync_at" SET NOT NULL;
ALTER TABLE "project_feature_usages" ALTER COLUMN "jira_dvcs_server_last_sync_at" SET NOT NULL;
ALTER TABLE "ci_refs" ALTER COLUMN "last_updated_by_pipeline_id" SET NOT NULL;
ALTER TABLE "merge_request_context_commit_diff_files" ALTER COLUMN "merge_request_context_commit_id" SET NOT NULL;
ALTER TABLE "requirements" ALTER COLUMN "author_id" SET NOT NULL;
ALTER TABLE "design_management_versions" ALTER COLUMN "author_id" SET NOT NULL;
ALTER TABLE "design_management_versions" ALTER COLUMN "issue_id" SET NOT NULL;
ALTER TABLE "protected_tag_create_access_levels" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "protected_tag_create_access_levels" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "resource_milestone_events" ALTER COLUMN "merge_request_id" SET NOT NULL;
ALTER TABLE "resource_milestone_events" ALTER COLUMN "issue_id" SET NOT NULL;
ALTER TABLE "resource_milestone_events" ALTER COLUMN "milestone_id" SET NOT NULL;
ALTER TABLE "audit_events" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "deploy_tokens" ALTER COLUMN "token" SET NOT NULL;
ALTER TABLE "deploy_tokens" ALTER COLUMN "token_encrypted" SET NOT NULL;
ALTER TABLE "path_locks" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "path_locks" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "software_licenses" ALTER COLUMN "spdx_identifier" SET NOT NULL;
ALTER TABLE "cluster_providers_aws" ALTER COLUMN "created_by_user_id" SET NOT NULL;
ALTER TABLE "feature_gates" ALTER COLUMN "value" SET NOT NULL;
ALTER TABLE "badges" ALTER COLUMN "project_id" SET NOT NULL;
ALTER TABLE "badges" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "merge_request_user_mentions" ALTER COLUMN "note_id" SET NOT NULL;
ALTER TABLE "deployments" ALTER COLUMN "deployable_type" SET NOT NULL;
ALTER TABLE "deployments" ALTER COLUMN "deployable_id" SET NOT NULL;
ALTER TABLE "deployments" ALTER COLUMN "updated_at" SET NOT NULL;
ALTER TABLE "deployments" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "deployments" ALTER COLUMN "created_at" SET NOT NULL;
ALTER TABLE "deployments" ALTER COLUMN "cluster_id" SET NOT NULL;
ALTER TABLE "packages_build_infos" ALTER COLUMN "pipeline_id" SET NOT NULL;
ALTER TABLE "redirect_routes" ADD CONSTRAINT "wetune_u_4e3319d44a2d0c40" UNIQUE ("path");
ALTER TABLE "scim_identities" ADD CONSTRAINT "wetune_u_8f56fe7f8846fb64" UNIQUE ("extern_uid");
ALTER TABLE "deployments" ADD CONSTRAINT "wetune_fk_7f6b748394d43a3f" FOREIGN KEY ("environment_id") REFERENCES "environments" ("id");
ALTER TABLE "identities" ADD CONSTRAINT "wetune_fk_6a36aa20419b151f" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "projects" ADD CONSTRAINT "wetune_fk_f7f0b2345f924a92" FOREIGN KEY ("namespace_id") REFERENCES "namespaces" ("id");
ALTER TABLE "deployments" ADD CONSTRAINT "wetune_fk_3a10161837f84c5c" FOREIGN KEY ("deployable_id") REFERENCES "ci_builds" ("id");
ALTER TABLE "routes" ADD CONSTRAINT "wetune_fk_76240f874c0a925f" FOREIGN KEY ("source_id") REFERENCES "projects" ("id");
