CREATE TABLE public.anonymous_users (id BIGINT NOT NULL, user_id INT NOT NULL, master_user_id INT NOT NULL, active BOOLEAN NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.api_keys (id INT NOT NULL, user_id INT, created_by_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, allowed_ips INET[], hidden BOOLEAN DEFAULT FALSE NOT NULL, last_used_at TIMESTAMP, revoked_at TIMESTAMP, description TEXT, key_hash VARCHAR NOT NULL, truncated_key VARCHAR NOT NULL);

CREATE TABLE public.application_requests (id INT NOT NULL, date DATE NOT NULL, req_type INT NOT NULL, count INT DEFAULT 0 NOT NULL);

CREATE TABLE public.ar_internal_metadata (key VARCHAR NOT NULL, value VARCHAR, created_at TIMESTAMP(6) NOT NULL, updated_at TIMESTAMP(6) NOT NULL);

CREATE TABLE public.backup_draft_posts (id BIGINT NOT NULL, user_id INT NOT NULL, post_id INT NOT NULL, key VARCHAR NOT NULL, created_at TIMESTAMP(6) NOT NULL, updated_at TIMESTAMP(6) NOT NULL);

CREATE TABLE public.backup_draft_topics (id BIGINT NOT NULL, user_id INT NOT NULL, topic_id INT NOT NULL, created_at TIMESTAMP(6) NOT NULL, updated_at TIMESTAMP(6) NOT NULL);

CREATE TABLE public.backup_metadata (id BIGINT NOT NULL, name VARCHAR NOT NULL, value VARCHAR);

CREATE TABLE public.badge_groupings (id INT NOT NULL, name VARCHAR NOT NULL, description TEXT, "position" INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.categories (id INT NOT NULL, name VARCHAR(50) NOT NULL, color VARCHAR(6) DEFAULT CAST('0088CC' AS VARCHAR) NOT NULL, topic_id INT, topic_count INT DEFAULT 0 NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, user_id INT NOT NULL, topics_year INT DEFAULT 0, topics_month INT DEFAULT 0, topics_week INT DEFAULT 0, slug VARCHAR NOT NULL, description TEXT, text_color VARCHAR(6) DEFAULT CAST('FFFFFF' AS VARCHAR) NOT NULL, read_restricted BOOLEAN DEFAULT FALSE NOT NULL, auto_close_hours DOUBLE PRECISION, post_count INT DEFAULT 0 NOT NULL, latest_post_id INT, latest_topic_id INT, "position" INT, parent_category_id INT, posts_year INT DEFAULT 0, posts_month INT DEFAULT 0, posts_week INT DEFAULT 0, email_in VARCHAR, email_in_allow_strangers BOOLEAN DEFAULT FALSE, topics_day INT DEFAULT 0, posts_day INT DEFAULT 0, allow_badges BOOLEAN DEFAULT TRUE NOT NULL, name_lower VARCHAR(50) NOT NULL, auto_close_based_on_last_post BOOLEAN DEFAULT FALSE, topic_template TEXT, contains_messages BOOLEAN, sort_order VARCHAR, sort_ascending BOOLEAN, uploaded_logo_id INT, uploaded_background_id INT, topic_featured_link_allowed BOOLEAN DEFAULT TRUE, all_topics_wiki BOOLEAN DEFAULT FALSE NOT NULL, show_subcategory_list BOOLEAN DEFAULT FALSE, num_featured_topics INT DEFAULT 3, default_view VARCHAR(50), subcategory_list_style VARCHAR(50) DEFAULT CAST('rows_with_featured_topics' AS VARCHAR), default_top_period VARCHAR(20) DEFAULT CAST('all' AS VARCHAR), mailinglist_mirror BOOLEAN DEFAULT FALSE NOT NULL, minimum_required_tags INT DEFAULT 0 NOT NULL, navigate_to_first_post_after_read BOOLEAN DEFAULT FALSE NOT NULL, search_priority INT DEFAULT 0, allow_global_tags BOOLEAN DEFAULT FALSE NOT NULL, reviewable_by_group_id INT, required_tag_group_id INT, min_tags_from_required_group INT DEFAULT 1 NOT NULL);

CREATE TABLE public.posts (id INT NOT NULL, user_id INT, topic_id INT NOT NULL, post_number INT NOT NULL, raw TEXT NOT NULL, cooked TEXT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, reply_to_post_number INT, reply_count INT DEFAULT 0 NOT NULL, quote_count INT DEFAULT 0 NOT NULL, deleted_at TIMESTAMP, off_topic_count INT DEFAULT 0 NOT NULL, like_count INT DEFAULT 0 NOT NULL, incoming_link_count INT DEFAULT 0 NOT NULL, bookmark_count INT DEFAULT 0 NOT NULL, avg_time INT, score DOUBLE PRECISION, reads INT DEFAULT 0 NOT NULL, post_type INT DEFAULT 1 NOT NULL, sort_order INT, last_editor_id INT, hidden BOOLEAN DEFAULT FALSE NOT NULL, hidden_reason_id INT, notify_moderators_count INT DEFAULT 0 NOT NULL, spam_count INT DEFAULT 0 NOT NULL, illegal_count INT DEFAULT 0 NOT NULL, inappropriate_count INT DEFAULT 0 NOT NULL, last_version_at TIMESTAMP NOT NULL, user_deleted BOOLEAN DEFAULT FALSE NOT NULL, reply_to_user_id INT, percent_rank DOUBLE PRECISION DEFAULT 1.0, notify_user_count INT DEFAULT 0 NOT NULL, like_score INT DEFAULT 0 NOT NULL, deleted_by_id INT, edit_reason VARCHAR, word_count INT, version INT DEFAULT 1 NOT NULL, cook_method INT DEFAULT 1 NOT NULL, wiki BOOLEAN DEFAULT FALSE NOT NULL, baked_at TIMESTAMP, baked_version INT, hidden_at TIMESTAMP, self_edits INT DEFAULT 0 NOT NULL, reply_quoted BOOLEAN DEFAULT FALSE NOT NULL, via_email BOOLEAN DEFAULT FALSE NOT NULL, raw_email TEXT, public_version INT DEFAULT 1 NOT NULL, action_code VARCHAR, image_url VARCHAR, locked_by_id INT);

CREATE TABLE public.topics (id INT NOT NULL, title VARCHAR NOT NULL, last_posted_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, views INT DEFAULT 0 NOT NULL, posts_count INT DEFAULT 0 NOT NULL, user_id INT, last_post_user_id INT NOT NULL, reply_count INT DEFAULT 0 NOT NULL, featured_user1_id INT, featured_user2_id INT, featured_user3_id INT, avg_time INT, deleted_at TIMESTAMP, highest_post_number INT DEFAULT 0 NOT NULL, image_url VARCHAR, like_count INT DEFAULT 0 NOT NULL, incoming_link_count INT DEFAULT 0 NOT NULL, category_id INT, visible BOOLEAN DEFAULT TRUE NOT NULL, moderator_posts_count INT DEFAULT 0 NOT NULL, closed BOOLEAN DEFAULT FALSE NOT NULL, archived BOOLEAN DEFAULT FALSE NOT NULL, bumped_at TIMESTAMP NOT NULL, has_summary BOOLEAN DEFAULT FALSE NOT NULL, archetype VARCHAR DEFAULT CAST('regular' AS VARCHAR) NOT NULL, featured_user4_id INT, notify_moderators_count INT DEFAULT 0 NOT NULL, spam_count INT DEFAULT 0 NOT NULL, pinned_at TIMESTAMP, score DOUBLE PRECISION, percent_rank DOUBLE PRECISION DEFAULT 1.0 NOT NULL, subtype VARCHAR, slug VARCHAR, deleted_by_id INT, participant_count INT DEFAULT 1, word_count INT, excerpt VARCHAR(1000), pinned_globally BOOLEAN DEFAULT FALSE NOT NULL, pinned_until TIMESTAMP, fancy_title VARCHAR(400), highest_staff_post_number INT DEFAULT 0 NOT NULL, featured_link VARCHAR, reviewable_score DOUBLE PRECISION DEFAULT 0.0 NOT NULL, CONSTRAINT has_category_id CHECK (((NOT category_id IS NULL) OR (CAST((archetype) AS TEXT) <> CAST('regular' AS TEXT)))), CONSTRAINT pm_has_no_category CHECK (((category_id IS NULL) OR (CAST((archetype) AS TEXT) <> CAST('private_message' AS TEXT)))));

CREATE TABLE public.badge_types (id INT NOT NULL, name VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.badges (id INT NOT NULL, name VARCHAR NOT NULL, description TEXT, badge_type_id INT NOT NULL, grant_count INT DEFAULT 0 NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, allow_title BOOLEAN DEFAULT FALSE NOT NULL, multiple_grant BOOLEAN DEFAULT FALSE NOT NULL, icon VARCHAR DEFAULT CAST('fa-certificate' AS VARCHAR), listable BOOLEAN DEFAULT TRUE, target_posts BOOLEAN DEFAULT FALSE, query TEXT, enabled BOOLEAN DEFAULT TRUE NOT NULL, auto_revoke BOOLEAN DEFAULT TRUE NOT NULL, badge_grouping_id INT DEFAULT 5 NOT NULL, trigger INT, show_posts BOOLEAN DEFAULT FALSE NOT NULL, system BOOLEAN DEFAULT FALSE NOT NULL, image VARCHAR(255), long_description TEXT);

CREATE TABLE public.bookmarks (id BIGINT NOT NULL, user_id BIGINT NOT NULL, topic_id BIGINT NOT NULL, post_id BIGINT NOT NULL, name VARCHAR, reminder_type INT, reminder_at TIMESTAMP, created_at TIMESTAMP(6) NOT NULL, updated_at TIMESTAMP(6) NOT NULL, reminder_last_sent_at TIMESTAMP, reminder_set_at TIMESTAMP);

CREATE TABLE public.categories_web_hooks (web_hook_id INT NOT NULL, category_id INT NOT NULL);

CREATE TABLE public.category_custom_fields (id INT NOT NULL, category_id INT NOT NULL, name VARCHAR(256) NOT NULL, value TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.category_featured_topics (category_id INT NOT NULL, topic_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, rank INT DEFAULT 0 NOT NULL, id INT NOT NULL);

CREATE TABLE public.category_groups (id INT NOT NULL, category_id INT NOT NULL, group_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, permission_type INT DEFAULT 1);

CREATE TABLE public.category_search_data (category_id INT NOT NULL, search_data tsvector, raw_data TEXT, locale TEXT, version INT DEFAULT 0);

CREATE TABLE public.category_tag_groups (id INT NOT NULL, category_id INT NOT NULL, tag_group_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.category_tag_stats (id BIGINT NOT NULL, category_id BIGINT NOT NULL, tag_id BIGINT NOT NULL, topic_count INT DEFAULT 0 NOT NULL);

CREATE TABLE public.category_tags (id INT NOT NULL, category_id INT NOT NULL, tag_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.category_users (id INT NOT NULL, category_id INT NOT NULL, user_id INT NOT NULL, notification_level INT, last_seen_at TIMESTAMP);

CREATE TABLE public.child_themes (id INT NOT NULL, parent_theme_id INT, child_theme_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.color_scheme_colors (id INT NOT NULL, name VARCHAR NOT NULL, hex VARCHAR NOT NULL, color_scheme_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.color_schemes (id INT NOT NULL, name VARCHAR NOT NULL, version INT DEFAULT 1 NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, via_wizard BOOLEAN DEFAULT FALSE NOT NULL, base_scheme_id VARCHAR, theme_id INT);

CREATE TABLE public.custom_emojis (id INT NOT NULL, name VARCHAR NOT NULL, upload_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.developers (id INT NOT NULL, user_id INT NOT NULL);

CREATE TABLE public.directory_items (id INT NOT NULL, period_type INT NOT NULL, user_id INT NOT NULL, likes_received INT NOT NULL, likes_given INT NOT NULL, topics_entered INT NOT NULL, topic_count INT NOT NULL, post_count INT NOT NULL, created_at TIMESTAMP, updated_at TIMESTAMP, days_visited INT DEFAULT 0 NOT NULL, posts_read INT DEFAULT 0 NOT NULL);

CREATE TABLE public.draft_sequences (id INT NOT NULL, user_id INT NOT NULL, draft_key VARCHAR NOT NULL, sequence INT NOT NULL);

CREATE TABLE public.drafts (id INT NOT NULL, user_id INT NOT NULL, draft_key VARCHAR NOT NULL, data TEXT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, sequence INT DEFAULT 0 NOT NULL, revisions INT DEFAULT 1 NOT NULL, owner VARCHAR);

CREATE TABLE public.email_change_requests (id INT NOT NULL, user_id INT NOT NULL, old_email VARCHAR NOT NULL, new_email VARCHAR NOT NULL, old_email_token_id INT, new_email_token_id INT, change_state INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.email_logs (id INT NOT NULL, to_address VARCHAR NOT NULL, email_type VARCHAR NOT NULL, user_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, post_id INT, bounce_key UUID, bounced BOOLEAN DEFAULT FALSE NOT NULL, message_id VARCHAR);

CREATE TABLE public.email_tokens (id INT NOT NULL, user_id INT NOT NULL, email VARCHAR NOT NULL, token VARCHAR NOT NULL, confirmed BOOLEAN DEFAULT FALSE NOT NULL, expired BOOLEAN DEFAULT FALSE NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.embeddable_hosts (id INT NOT NULL, host VARCHAR NOT NULL, category_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, path_whitelist VARCHAR, class_name VARCHAR);

CREATE TABLE public.github_user_infos (id INT NOT NULL, user_id INT NOT NULL, screen_name VARCHAR NOT NULL, github_user_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.given_daily_likes (user_id INT NOT NULL, likes_given INT NOT NULL, given_date DATE NOT NULL, limit_reached BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.group_archived_messages (id INT NOT NULL, group_id INT NOT NULL, topic_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.group_custom_fields (id INT NOT NULL, group_id INT NOT NULL, name VARCHAR(256) NOT NULL, value TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.group_histories (id INT NOT NULL, group_id INT NOT NULL, acting_user_id INT NOT NULL, target_user_id INT, action INT NOT NULL, subject VARCHAR, prev_value TEXT, new_value TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.group_mentions (id INT NOT NULL, post_id INT, group_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.group_requests (id BIGINT NOT NULL, group_id INT, user_id INT, reason TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.group_users (id INT NOT NULL, group_id INT NOT NULL, user_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, owner BOOLEAN DEFAULT FALSE NOT NULL, notification_level INT DEFAULT 2 NOT NULL);

CREATE TABLE public.groups (id INT NOT NULL, name VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, automatic BOOLEAN DEFAULT FALSE NOT NULL, user_count INT DEFAULT 0 NOT NULL, automatic_membership_email_domains TEXT, automatic_membership_retroactive BOOLEAN DEFAULT FALSE, primary_group BOOLEAN DEFAULT FALSE NOT NULL, title VARCHAR, grant_trust_level INT, incoming_email VARCHAR, has_messages BOOLEAN DEFAULT FALSE NOT NULL, flair_url VARCHAR, flair_bg_color VARCHAR, flair_color VARCHAR, bio_raw TEXT, bio_cooked TEXT, allow_membership_requests BOOLEAN DEFAULT FALSE NOT NULL, full_name VARCHAR, default_notification_level INT DEFAULT 3 NOT NULL, visibility_level INT DEFAULT 0 NOT NULL, public_exit BOOLEAN DEFAULT FALSE NOT NULL, public_admission BOOLEAN DEFAULT FALSE NOT NULL, membership_request_template TEXT, messageable_level INT DEFAULT 0, mentionable_level INT DEFAULT 0, publish_read_state BOOLEAN DEFAULT FALSE NOT NULL, members_visibility_level INT DEFAULT 0 NOT NULL);

CREATE TABLE public.groups_web_hooks (web_hook_id INT NOT NULL, group_id INT NOT NULL);

CREATE TABLE public.ignored_users (id BIGINT NOT NULL, user_id INT NOT NULL, ignored_user_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, summarized_at TIMESTAMP, expiring_at TIMESTAMP);

CREATE TABLE public.incoming_domains (id INT NOT NULL, name VARCHAR(100) NOT NULL, https BOOLEAN DEFAULT FALSE NOT NULL, port INT NOT NULL);

CREATE TABLE public.incoming_emails (id INT NOT NULL, user_id INT, topic_id INT, post_id INT, raw TEXT, error TEXT, message_id TEXT, from_address TEXT, to_addresses TEXT, cc_addresses TEXT, subject TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, rejection_message TEXT, is_auto_generated BOOLEAN DEFAULT FALSE, is_bounce BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.incoming_links (id INT NOT NULL, created_at TIMESTAMP NOT NULL, user_id INT, ip_address INET, current_user_id INT, post_id INT NOT NULL, incoming_referer_id INT);

CREATE TABLE public.incoming_referers (id INT NOT NULL, path VARCHAR(1000) NOT NULL, incoming_domain_id INT NOT NULL);

CREATE TABLE public.invited_groups (id INT NOT NULL, group_id INT, invite_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.invites (id INT NOT NULL, invite_key VARCHAR(32) NOT NULL, email VARCHAR, invited_by_id INT NOT NULL, user_id INT, redeemed_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, deleted_at TIMESTAMP, deleted_by_id INT, invalidated_at TIMESTAMP, moderator BOOLEAN DEFAULT FALSE NOT NULL, custom_message TEXT, emailed_status INT);

CREATE TABLE public.javascript_caches (id BIGINT NOT NULL, theme_field_id BIGINT, digest VARCHAR, content TEXT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, theme_id BIGINT);

CREATE TABLE public.message_bus (id INT NOT NULL, name VARCHAR, context VARCHAR, data TEXT, created_at TIMESTAMP NOT NULL);

CREATE TABLE public.muted_users (id INT NOT NULL, user_id INT NOT NULL, muted_user_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.notifications (id INT NOT NULL, notification_type INT NOT NULL, user_id INT NOT NULL, data VARCHAR(1000) NOT NULL, read BOOLEAN DEFAULT FALSE NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, topic_id INT, post_number INT, post_action_id INT);

CREATE TABLE public.oauth2_user_infos (id INT NOT NULL, user_id INT NOT NULL, uid VARCHAR NOT NULL, provider VARCHAR NOT NULL, email VARCHAR, name VARCHAR, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.onceoff_logs (id INT NOT NULL, job_name VARCHAR, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.optimized_images (id INT NOT NULL, sha1 VARCHAR(40) NOT NULL, extension VARCHAR(10) NOT NULL, width INT NOT NULL, height INT NOT NULL, upload_id INT NOT NULL, url VARCHAR NOT NULL, filesize INT, etag VARCHAR, version INT);

CREATE TABLE public.permalinks (id INT NOT NULL, url VARCHAR(1000) NOT NULL, topic_id INT, post_id INT, category_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, external_url VARCHAR(1000));

CREATE TABLE public.plugin_store_rows (id INT NOT NULL, plugin_name VARCHAR NOT NULL, key VARCHAR NOT NULL, type_name VARCHAR NOT NULL, value TEXT);

CREATE TABLE public.poll_options (id BIGINT NOT NULL, poll_id BIGINT, digest VARCHAR NOT NULL, html TEXT NOT NULL, anonymous_votes INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.poll_votes (poll_id BIGINT, poll_option_id BIGINT, user_id BIGINT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.polls (id BIGINT NOT NULL, post_id BIGINT, name VARCHAR DEFAULT CAST('poll' AS VARCHAR) NOT NULL, close_at TIMESTAMP, type INT DEFAULT 0 NOT NULL, status INT DEFAULT 0 NOT NULL, results INT DEFAULT 0 NOT NULL, visibility INT DEFAULT 0 NOT NULL, min INT, max INT, step INT, anonymous_voters INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, chart_type INT DEFAULT 0 NOT NULL, groups VARCHAR);

CREATE TABLE public.post_action_types (name_key VARCHAR(50) NOT NULL, is_flag BOOLEAN DEFAULT FALSE NOT NULL, icon VARCHAR(20), created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, id INT NOT NULL, "position" INT DEFAULT 0 NOT NULL, score_bonus DOUBLE PRECISION DEFAULT 0.0 NOT NULL, reviewable_priority INT DEFAULT 0 NOT NULL);

CREATE TABLE public.post_actions (id INT NOT NULL, post_id INT NOT NULL, user_id INT NOT NULL, post_action_type_id INT NOT NULL, deleted_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, deleted_by_id INT, related_post_id INT, staff_took_action BOOLEAN DEFAULT FALSE NOT NULL, deferred_by_id INT, targets_topic BOOLEAN DEFAULT FALSE NOT NULL, agreed_at TIMESTAMP, agreed_by_id INT, deferred_at TIMESTAMP, disagreed_at TIMESTAMP, disagreed_by_id INT);

CREATE TABLE public.post_custom_fields (id INT NOT NULL, post_id INT NOT NULL, name VARCHAR(256) NOT NULL, value TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.post_details (id INT NOT NULL, post_id INT, key VARCHAR, value VARCHAR, extra TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.post_replies (post_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, reply_post_id INT);

CREATE TABLE public.post_reply_keys (id BIGINT NOT NULL, user_id INT NOT NULL, post_id INT NOT NULL, reply_key UUID NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.post_revisions (id INT NOT NULL, user_id INT, post_id INT, modifications TEXT, number INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, hidden BOOLEAN DEFAULT FALSE NOT NULL);

CREATE TABLE public.post_search_data (post_id INT NOT NULL, search_data tsvector, raw_data TEXT, locale VARCHAR, version INT DEFAULT 0);

CREATE TABLE public.post_stats (id INT NOT NULL, post_id INT, drafts_saved INT, typing_duration_msecs INT, composer_open_duration_msecs INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.post_timings (topic_id INT NOT NULL, post_number INT NOT NULL, user_id INT NOT NULL, msecs INT NOT NULL);

CREATE TABLE public.post_uploads (id INT NOT NULL, post_id INT NOT NULL, upload_id INT NOT NULL);

CREATE TABLE public.push_subscriptions (id BIGINT NOT NULL, user_id INT NOT NULL, data VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.quoted_posts (id INT NOT NULL, post_id INT NOT NULL, quoted_post_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.remote_themes (id INT NOT NULL, remote_url VARCHAR NOT NULL, remote_version VARCHAR, local_version VARCHAR, about_url VARCHAR, license_url VARCHAR, commits_behind INT, remote_updated_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, private_key TEXT, branch VARCHAR, last_error_text TEXT, authors VARCHAR, theme_version VARCHAR, minimum_discourse_version VARCHAR, maximum_discourse_version VARCHAR);

CREATE TABLE public.reviewable_claimed_topics (id BIGINT NOT NULL, user_id INT NOT NULL, topic_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.reviewable_histories (id BIGINT NOT NULL, reviewable_id INT NOT NULL, reviewable_history_type INT NOT NULL, status INT NOT NULL, created_by_id INT NOT NULL, edited JSON, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.reviewable_scores (id BIGINT NOT NULL, reviewable_id INT NOT NULL, user_id INT NOT NULL, reviewable_score_type INT NOT NULL, status INT NOT NULL, score DOUBLE PRECISION DEFAULT 0.0 NOT NULL, take_action_bonus DOUBLE PRECISION DEFAULT 0.0 NOT NULL, reviewed_by_id INT, reviewed_at TIMESTAMP, meta_topic_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, reason VARCHAR, user_accuracy_bonus DOUBLE PRECISION DEFAULT 0.0 NOT NULL);

CREATE TABLE public.reviewables (id BIGINT NOT NULL, type VARCHAR NOT NULL, status INT DEFAULT 0 NOT NULL, created_by_id INT NOT NULL, reviewable_by_moderator BOOLEAN DEFAULT FALSE NOT NULL, reviewable_by_group_id INT, category_id INT, topic_id INT, score DOUBLE PRECISION DEFAULT 0.0 NOT NULL, potential_spam BOOLEAN DEFAULT FALSE NOT NULL, target_id INT, target_type VARCHAR, target_created_by_id INT, payload JSON, version INT DEFAULT 0 NOT NULL, latest_score TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.scheduler_stats (id INT NOT NULL, name VARCHAR NOT NULL, hostname VARCHAR NOT NULL, pid INT NOT NULL, duration_ms INT, live_slots_start INT, live_slots_finish INT, started_at TIMESTAMP NOT NULL, success BOOLEAN, error TEXT);

CREATE TABLE public.schema_migration_details (id INT NOT NULL, version VARCHAR NOT NULL, name VARCHAR, hostname VARCHAR, git_version VARCHAR, rails_version VARCHAR, duration INT, direction VARCHAR, created_at TIMESTAMP NOT NULL);

CREATE TABLE public.schema_migrations (version VARCHAR NOT NULL);

CREATE TABLE public.screened_emails (id INT NOT NULL, email VARCHAR NOT NULL, action_type INT NOT NULL, match_count INT DEFAULT 0 NOT NULL, last_match_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, ip_address INET);

CREATE TABLE public.screened_ip_addresses (id INT NOT NULL, ip_address INET NOT NULL, action_type INT NOT NULL, match_count INT DEFAULT 0 NOT NULL, last_match_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.screened_urls (id INT NOT NULL, url VARCHAR NOT NULL, domain VARCHAR NOT NULL, action_type INT NOT NULL, match_count INT DEFAULT 0 NOT NULL, last_match_at TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, ip_address INET);

CREATE TABLE public.search_logs (id INT NOT NULL, term VARCHAR NOT NULL, user_id INT, ip_address INET, search_result_id INT, search_type INT NOT NULL, created_at TIMESTAMP NOT NULL, search_result_type INT);

CREATE TABLE public.shared_drafts (topic_id INT NOT NULL, category_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, id BIGINT NOT NULL);

CREATE TABLE public.single_sign_on_records (id INT NOT NULL, user_id INT NOT NULL, external_id VARCHAR NOT NULL, last_payload TEXT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, external_username VARCHAR, external_email VARCHAR, external_name VARCHAR, external_avatar_url VARCHAR(1000), external_profile_background_url VARCHAR, external_card_background_url VARCHAR);

CREATE TABLE public.site_settings (id INT NOT NULL, name VARCHAR NOT NULL, data_type INT NOT NULL, value TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.skipped_email_logs (id BIGINT NOT NULL, email_type VARCHAR NOT NULL, to_address VARCHAR NOT NULL, user_id INT, post_id INT, reason_type INT NOT NULL, custom_reason TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.stylesheet_cache (id INT NOT NULL, target VARCHAR NOT NULL, digest VARCHAR NOT NULL, content TEXT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, theme_id INT DEFAULT CAST('-1' AS INT) NOT NULL, source_map TEXT);

CREATE TABLE public.tag_group_memberships (id INT NOT NULL, tag_id INT NOT NULL, tag_group_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.tag_group_permissions (id BIGINT NOT NULL, tag_group_id BIGINT NOT NULL, group_id BIGINT NOT NULL, permission_type INT DEFAULT 1 NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.tag_groups (id INT NOT NULL, name VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, parent_tag_id INT, one_per_topic BOOLEAN DEFAULT FALSE);

CREATE TABLE public.tag_search_data (tag_id INT NOT NULL, search_data tsvector, raw_data TEXT, locale TEXT, version INT DEFAULT 0);

CREATE TABLE public.tag_users (id INT NOT NULL, tag_id INT NOT NULL, user_id INT NOT NULL, notification_level INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.tags (id INT NOT NULL, name VARCHAR NOT NULL, topic_count INT DEFAULT 0 NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, pm_topic_count INT DEFAULT 0 NOT NULL, target_tag_id INT);

CREATE TABLE public.tags_web_hooks (web_hook_id BIGINT NOT NULL, tag_id BIGINT NOT NULL);

CREATE TABLE public.theme_fields (id INT NOT NULL, theme_id INT NOT NULL, target_id INT NOT NULL, name VARCHAR(255) NOT NULL, value TEXT NOT NULL, value_baked TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, compiler_version VARCHAR(50) DEFAULT 0 NOT NULL, error VARCHAR, upload_id INT, type_id INT DEFAULT 0 NOT NULL);

CREATE TABLE public.theme_modifier_sets (id BIGINT NOT NULL, theme_id BIGINT NOT NULL, serialize_topic_excerpts BOOLEAN, csp_extensions VARCHAR[], svg_icons VARCHAR[]);

CREATE TABLE public.theme_settings (id BIGINT NOT NULL, name VARCHAR(255) NOT NULL, data_type INT NOT NULL, value TEXT, theme_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.theme_translation_overrides (id BIGINT NOT NULL, theme_id INT NOT NULL, locale VARCHAR NOT NULL, translation_key VARCHAR NOT NULL, value VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.themes (id INT NOT NULL, name VARCHAR NOT NULL, user_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, compiler_version INT DEFAULT 0 NOT NULL, user_selectable BOOLEAN DEFAULT FALSE NOT NULL, hidden BOOLEAN DEFAULT FALSE NOT NULL, color_scheme_id INT, remote_theme_id INT, component BOOLEAN DEFAULT FALSE NOT NULL, enabled BOOLEAN DEFAULT TRUE NOT NULL);

CREATE TABLE public.top_topics (id INT NOT NULL, topic_id INT, yearly_posts_count INT DEFAULT 0 NOT NULL, yearly_views_count INT DEFAULT 0 NOT NULL, yearly_likes_count INT DEFAULT 0 NOT NULL, monthly_posts_count INT DEFAULT 0 NOT NULL, monthly_views_count INT DEFAULT 0 NOT NULL, monthly_likes_count INT DEFAULT 0 NOT NULL, weekly_posts_count INT DEFAULT 0 NOT NULL, weekly_views_count INT DEFAULT 0 NOT NULL, weekly_likes_count INT DEFAULT 0 NOT NULL, daily_posts_count INT DEFAULT 0 NOT NULL, daily_views_count INT DEFAULT 0 NOT NULL, daily_likes_count INT DEFAULT 0 NOT NULL, daily_score DOUBLE PRECISION DEFAULT 0.0, weekly_score DOUBLE PRECISION DEFAULT 0.0, monthly_score DOUBLE PRECISION DEFAULT 0.0, yearly_score DOUBLE PRECISION DEFAULT 0.0, all_score DOUBLE PRECISION DEFAULT 0.0, daily_op_likes_count INT DEFAULT 0 NOT NULL, weekly_op_likes_count INT DEFAULT 0 NOT NULL, monthly_op_likes_count INT DEFAULT 0 NOT NULL, yearly_op_likes_count INT DEFAULT 0 NOT NULL, quarterly_posts_count INT DEFAULT 0 NOT NULL, quarterly_views_count INT DEFAULT 0 NOT NULL, quarterly_likes_count INT DEFAULT 0 NOT NULL, quarterly_score DOUBLE PRECISION DEFAULT 0.0, quarterly_op_likes_count INT DEFAULT 0 NOT NULL);

CREATE TABLE public.topic_allowed_groups (id INT NOT NULL, group_id INT NOT NULL, topic_id INT NOT NULL);

CREATE TABLE public.topic_allowed_users (id INT NOT NULL, user_id INT NOT NULL, topic_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.topic_custom_fields (id INT NOT NULL, topic_id INT NOT NULL, name VARCHAR(256) NOT NULL, value TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.topic_embeds (id INT NOT NULL, topic_id INT NOT NULL, post_id INT NOT NULL, embed_url VARCHAR(1000) NOT NULL, content_sha1 VARCHAR(40), created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, deleted_at TIMESTAMP, deleted_by_id INT);

CREATE TABLE public.topic_groups (id BIGINT NOT NULL, group_id INT NOT NULL, topic_id INT NOT NULL, last_read_post_number INT DEFAULT 0 NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.topic_invites (id INT NOT NULL, topic_id INT NOT NULL, invite_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.topic_link_clicks (id INT NOT NULL, topic_link_id INT NOT NULL, user_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, ip_address INET);

CREATE TABLE public.topic_links (id INT NOT NULL, topic_id INT NOT NULL, post_id INT, user_id INT NOT NULL, url VARCHAR(500) NOT NULL, domain VARCHAR(100) NOT NULL, internal BOOLEAN DEFAULT FALSE NOT NULL, link_topic_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, reflection BOOLEAN DEFAULT FALSE, clicks INT DEFAULT 0 NOT NULL, link_post_id INT, title VARCHAR, crawled_at TIMESTAMP, quote BOOLEAN DEFAULT FALSE NOT NULL, extension VARCHAR(10));

CREATE TABLE public.topic_search_data (topic_id INT NOT NULL, raw_data TEXT, locale VARCHAR NOT NULL, search_data tsvector, version INT DEFAULT 0);

CREATE TABLE public.topic_tags (id INT NOT NULL, topic_id INT NOT NULL, tag_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.topic_timers (id INT NOT NULL, execute_at TIMESTAMP NOT NULL, status_type INT NOT NULL, user_id INT NOT NULL, topic_id INT NOT NULL, based_on_last_post BOOLEAN DEFAULT FALSE NOT NULL, deleted_at TIMESTAMP, deleted_by_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, category_id INT, public_type BOOLEAN DEFAULT TRUE, duration INT);

CREATE TABLE public.topic_users (user_id INT NOT NULL, topic_id INT NOT NULL, posted BOOLEAN DEFAULT FALSE NOT NULL, last_read_post_number INT, highest_seen_post_number INT, last_visited_at TIMESTAMP, first_visited_at TIMESTAMP, notification_level INT DEFAULT 1 NOT NULL, notifications_changed_at TIMESTAMP, notifications_reason_id INT, total_msecs_viewed INT DEFAULT 0 NOT NULL, cleared_pinned_at TIMESTAMP, id INT NOT NULL, last_emailed_post_number INT, liked BOOLEAN DEFAULT FALSE, bookmarked BOOLEAN DEFAULT FALSE);

CREATE TABLE public.topic_views (topic_id INT NOT NULL, viewed_at DATE NOT NULL, user_id INT, ip_address INET);

CREATE TABLE public.translation_overrides (id INT NOT NULL, locale VARCHAR NOT NULL, translation_key VARCHAR NOT NULL, value VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, compiled_js TEXT);

CREATE TABLE public.unsubscribe_keys (key VARCHAR(64) NOT NULL, user_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, unsubscribe_key_type VARCHAR, topic_id INT, post_id INT);

CREATE TABLE public.uploads (id INT NOT NULL, user_id INT NOT NULL, original_filename VARCHAR NOT NULL, filesize INT NOT NULL, width INT, height INT, url VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, sha1 VARCHAR(40), origin VARCHAR(1000), retain_hours INT, extension VARCHAR(10), thumbnail_width INT, thumbnail_height INT, etag VARCHAR, secure BOOLEAN DEFAULT FALSE NOT NULL, access_control_post_id BIGINT, original_sha1 VARCHAR);

CREATE TABLE public.user_actions (id INT NOT NULL, action_type INT NOT NULL, user_id INT NOT NULL, target_topic_id INT, target_post_id INT, target_user_id INT, acting_user_id INT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.user_api_keys (id INT NOT NULL, user_id INT NOT NULL, client_id VARCHAR NOT NULL, key VARCHAR NOT NULL, application_name VARCHAR NOT NULL, push_url VARCHAR, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, revoked_at TIMESTAMP, scopes TEXT[] DEFAULT CAST('{}' AS TEXT[]) NOT NULL, last_used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL);

CREATE TABLE public.user_archived_messages (id INT NOT NULL, user_id INT NOT NULL, topic_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.user_associated_accounts (id BIGINT NOT NULL, provider_name VARCHAR NOT NULL, provider_uid VARCHAR NOT NULL, user_id INT, last_used TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL, info JSONB DEFAULT CAST('{}' AS JSONB) NOT NULL, credentials JSONB DEFAULT CAST('{}' AS JSONB) NOT NULL, extra JSONB DEFAULT CAST('{}' AS JSONB) NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.user_auth_token_logs (id INT NOT NULL, action VARCHAR NOT NULL, user_auth_token_id INT, user_id INT, client_ip INET, user_agent VARCHAR, auth_token VARCHAR, created_at TIMESTAMP, path VARCHAR);

CREATE TABLE public.user_auth_tokens (id INT NOT NULL, user_id INT NOT NULL, auth_token VARCHAR NOT NULL, prev_auth_token VARCHAR NOT NULL, user_agent VARCHAR, auth_token_seen BOOLEAN DEFAULT FALSE NOT NULL, client_ip INET, rotated_at TIMESTAMP NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, seen_at TIMESTAMP);

CREATE TABLE public.user_avatars (id INT NOT NULL, user_id INT NOT NULL, custom_upload_id INT, gravatar_upload_id INT, last_gravatar_download_attempt TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.user_badges (id INT NOT NULL, badge_id INT NOT NULL, user_id INT NOT NULL, granted_at TIMESTAMP NOT NULL, granted_by_id INT NOT NULL, post_id INT, notification_id INT, seq INT DEFAULT 0 NOT NULL, featured_rank INT);

CREATE TABLE public.user_custom_fields (id INT NOT NULL, user_id INT NOT NULL, name VARCHAR(256) NOT NULL, value TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.user_emails (id INT NOT NULL, user_id INT NOT NULL, email VARCHAR(513) NOT NULL, "primary" BOOLEAN DEFAULT FALSE NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.user_exports (id INT NOT NULL, file_name VARCHAR NOT NULL, user_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, upload_id INT, topic_id INT);

CREATE TABLE public.user_field_options (id INT NOT NULL, user_field_id INT NOT NULL, value VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.user_fields (id INT NOT NULL, name VARCHAR NOT NULL, field_type VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, editable BOOLEAN DEFAULT FALSE NOT NULL, description VARCHAR NOT NULL, required BOOLEAN DEFAULT TRUE NOT NULL, show_on_profile BOOLEAN DEFAULT FALSE NOT NULL, "position" INT DEFAULT 0, show_on_user_card BOOLEAN DEFAULT FALSE NOT NULL, external_name VARCHAR, external_type VARCHAR);

CREATE TABLE public.user_histories (id INT NOT NULL, action INT NOT NULL, acting_user_id INT, target_user_id INT, details TEXT, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, context VARCHAR, ip_address VARCHAR, email VARCHAR, subject TEXT, previous_value TEXT, new_value TEXT, topic_id INT, admin_only BOOLEAN DEFAULT FALSE, post_id INT, custom_type VARCHAR, category_id INT);

CREATE TABLE public.user_open_ids (id INT NOT NULL, user_id INT NOT NULL, email VARCHAR NOT NULL, url VARCHAR NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, active BOOLEAN NOT NULL);

CREATE TABLE public.user_options (user_id INT NOT NULL, mailing_list_mode BOOLEAN DEFAULT FALSE NOT NULL, email_digests BOOLEAN, external_links_in_new_tab BOOLEAN DEFAULT FALSE NOT NULL, enable_quoting BOOLEAN DEFAULT TRUE NOT NULL, dynamic_favicon BOOLEAN DEFAULT FALSE NOT NULL, disable_jump_reply BOOLEAN DEFAULT FALSE NOT NULL, automatically_unpin_topics BOOLEAN DEFAULT TRUE NOT NULL, digest_after_minutes INT, auto_track_topics_after_msecs INT, new_topic_duration_minutes INT, last_redirected_to_top_at TIMESTAMP, email_previous_replies INT DEFAULT 2 NOT NULL, email_in_reply_to BOOLEAN DEFAULT TRUE NOT NULL, like_notification_frequency INT DEFAULT 1 NOT NULL, mailing_list_mode_frequency INT DEFAULT 1 NOT NULL, include_tl0_in_digests BOOLEAN DEFAULT FALSE, notification_level_when_replying INT, theme_key_seq INT DEFAULT 0 NOT NULL, allow_private_messages BOOLEAN DEFAULT TRUE NOT NULL, homepage_id INT, theme_ids INT[] DEFAULT CAST('{}' AS INT[]) NOT NULL, hide_profile_and_presence BOOLEAN DEFAULT FALSE NOT NULL, text_size_key INT DEFAULT 0 NOT NULL, text_size_seq INT DEFAULT 0 NOT NULL, email_level INT DEFAULT 1 NOT NULL, email_messages_level INT DEFAULT 0 NOT NULL, title_count_mode_key INT DEFAULT 0 NOT NULL, enable_defer BOOLEAN DEFAULT FALSE NOT NULL, timezone VARCHAR);

CREATE TABLE public.user_profile_views (id INT NOT NULL, user_profile_id INT NOT NULL, viewed_at TIMESTAMP NOT NULL, ip_address INET, user_id INT);

CREATE TABLE public.user_profiles (user_id INT NOT NULL, location VARCHAR, website VARCHAR, bio_raw TEXT, bio_cooked TEXT, dismissed_banner_key INT, bio_cooked_version INT, badge_granted_title BOOLEAN DEFAULT FALSE, views INT DEFAULT 0 NOT NULL, profile_background_upload_id INT, card_background_upload_id INT, granted_title_badge_id BIGINT, featured_topic_id INT);

CREATE TABLE public.user_search_data (user_id INT NOT NULL, search_data tsvector, raw_data TEXT, locale TEXT, version INT DEFAULT 0);

CREATE TABLE public.user_second_factors (id BIGINT NOT NULL, user_id INT NOT NULL, method INT NOT NULL, data VARCHAR NOT NULL, enabled BOOLEAN DEFAULT FALSE NOT NULL, last_used TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, name VARCHAR);

CREATE TABLE public.user_security_keys (id BIGINT NOT NULL, user_id BIGINT NOT NULL, credential_id VARCHAR NOT NULL, public_key VARCHAR NOT NULL, factor_type INT DEFAULT 0 NOT NULL, enabled BOOLEAN DEFAULT TRUE NOT NULL, name VARCHAR NOT NULL, last_used TIMESTAMP, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.user_stats (user_id INT NOT NULL, topics_entered INT DEFAULT 0 NOT NULL, time_read INT DEFAULT 0 NOT NULL, days_visited INT DEFAULT 0 NOT NULL, posts_read_count INT DEFAULT 0 NOT NULL, likes_given INT DEFAULT 0 NOT NULL, likes_received INT DEFAULT 0 NOT NULL, topic_reply_count INT DEFAULT 0 NOT NULL, new_since TIMESTAMP NOT NULL, read_faq TIMESTAMP, first_post_created_at TIMESTAMP, post_count INT DEFAULT 0 NOT NULL, topic_count INT DEFAULT 0 NOT NULL, bounce_score DOUBLE PRECISION DEFAULT 0 NOT NULL, reset_bounce_score_after TIMESTAMP, flags_agreed INT DEFAULT 0 NOT NULL, flags_disagreed INT DEFAULT 0 NOT NULL, flags_ignored INT DEFAULT 0 NOT NULL, first_unread_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL, distinct_badge_count INT DEFAULT 0 NOT NULL);

CREATE TABLE public.user_uploads (id BIGINT NOT NULL, upload_id INT NOT NULL, user_id INT NOT NULL, created_at TIMESTAMP NOT NULL);

CREATE TABLE public.user_visits (id INT NOT NULL, user_id INT NOT NULL, visited_at DATE NOT NULL, posts_read INT DEFAULT 0, mobile BOOLEAN DEFAULT FALSE, time_read INT DEFAULT 0 NOT NULL);

CREATE TABLE public.user_warnings (id INT NOT NULL, topic_id INT NOT NULL, user_id INT NOT NULL, created_by_id INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.users (id INT NOT NULL, username VARCHAR(60) NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL, name VARCHAR, seen_notification_id INT DEFAULT 0 NOT NULL, last_posted_at TIMESTAMP, password_hash VARCHAR(64), salt VARCHAR(32), active BOOLEAN DEFAULT FALSE NOT NULL, username_lower VARCHAR(60) NOT NULL, last_seen_at TIMESTAMP, admin BOOLEAN DEFAULT FALSE NOT NULL, last_emailed_at TIMESTAMP, trust_level INT NOT NULL, approved BOOLEAN DEFAULT FALSE NOT NULL, approved_by_id INT, approved_at TIMESTAMP, previous_visit_at TIMESTAMP, suspended_at TIMESTAMP, suspended_till TIMESTAMP, date_of_birth DATE, views INT DEFAULT 0 NOT NULL, flag_level INT DEFAULT 0 NOT NULL, ip_address INET, moderator BOOLEAN DEFAULT FALSE, title VARCHAR, uploaded_avatar_id INT, locale VARCHAR(10), primary_group_id INT, registration_ip_address INET, staged BOOLEAN DEFAULT FALSE NOT NULL, first_seen_at TIMESTAMP, silenced_till TIMESTAMP, group_locked_trust_level INT, manual_locked_trust_level INT, secure_identifier VARCHAR);

CREATE TABLE public.watched_words (id INT NOT NULL, word VARCHAR NOT NULL, action INT NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.web_crawler_requests (id BIGINT NOT NULL, date DATE NOT NULL, user_agent VARCHAR NOT NULL, count INT DEFAULT 0 NOT NULL);

CREATE TABLE public.web_hook_event_types (id INT NOT NULL, name VARCHAR NOT NULL);

CREATE TABLE public.web_hook_event_types_hooks (web_hook_id INT NOT NULL, web_hook_event_type_id INT NOT NULL);

CREATE TABLE public.web_hook_events (id INT NOT NULL, web_hook_id INT NOT NULL, headers VARCHAR, payload TEXT, status INT DEFAULT 0, response_headers VARCHAR, response_body TEXT, duration INT DEFAULT 0, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

CREATE TABLE public.web_hooks (id INT NOT NULL, payload_url VARCHAR NOT NULL, content_type INT DEFAULT 1 NOT NULL, last_delivery_status INT DEFAULT 1 NOT NULL, status INT DEFAULT 1 NOT NULL, secret VARCHAR DEFAULT CAST('' AS VARCHAR), wildcard_web_hook BOOLEAN DEFAULT FALSE NOT NULL, verify_certificate BOOLEAN DEFAULT TRUE NOT NULL, active BOOLEAN DEFAULT FALSE NOT NULL, created_at TIMESTAMP NOT NULL, updated_at TIMESTAMP NOT NULL);

ALTER TABLE ONLY public.anonymous_users ADD CONSTRAINT anonymous_users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.api_keys ADD CONSTRAINT api_keys_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.application_requests ADD CONSTRAINT application_requests_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ar_internal_metadata ADD CONSTRAINT ar_internal_metadata_pkey PRIMARY KEY (key);

ALTER TABLE ONLY public.backup_draft_posts ADD CONSTRAINT backup_draft_posts_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.backup_draft_topics ADD CONSTRAINT backup_draft_topics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.backup_metadata ADD CONSTRAINT backup_metadata_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.badge_groupings ADD CONSTRAINT badge_groupings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.badge_types ADD CONSTRAINT badge_types_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.badges ADD CONSTRAINT badges_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.bookmarks ADD CONSTRAINT bookmarks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.categories ADD CONSTRAINT categories_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.category_search_data ADD CONSTRAINT categories_search_pkey PRIMARY KEY (category_id);

ALTER TABLE ONLY public.category_custom_fields ADD CONSTRAINT category_custom_fields_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.category_featured_topics ADD CONSTRAINT category_featured_topics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.category_groups ADD CONSTRAINT category_groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.category_tag_groups ADD CONSTRAINT category_tag_groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.category_tag_stats ADD CONSTRAINT category_tag_stats_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.category_tags ADD CONSTRAINT category_tags_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.category_users ADD CONSTRAINT category_users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.child_themes ADD CONSTRAINT child_themes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.color_scheme_colors ADD CONSTRAINT color_scheme_colors_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.color_schemes ADD CONSTRAINT color_schemes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.custom_emojis ADD CONSTRAINT custom_emojis_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.developers ADD CONSTRAINT developers_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.unsubscribe_keys ADD CONSTRAINT digest_unsubscribe_keys_pkey PRIMARY KEY (key);

ALTER TABLE ONLY public.directory_items ADD CONSTRAINT directory_items_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.draft_sequences ADD CONSTRAINT draft_sequences_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.drafts ADD CONSTRAINT drafts_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.email_change_requests ADD CONSTRAINT email_change_requests_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.email_logs ADD CONSTRAINT email_logs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.email_tokens ADD CONSTRAINT email_tokens_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.embeddable_hosts ADD CONSTRAINT embeddable_hosts_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.github_user_infos ADD CONSTRAINT github_user_infos_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.group_archived_messages ADD CONSTRAINT group_archived_messages_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.group_custom_fields ADD CONSTRAINT group_custom_fields_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.group_histories ADD CONSTRAINT group_histories_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.group_mentions ADD CONSTRAINT group_mentions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.group_requests ADD CONSTRAINT group_requests_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.group_users ADD CONSTRAINT group_users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.groups ADD CONSTRAINT groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.ignored_users ADD CONSTRAINT ignored_users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.incoming_domains ADD CONSTRAINT incoming_domains_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.incoming_emails ADD CONSTRAINT incoming_emails_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.incoming_links ADD CONSTRAINT incoming_links_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.incoming_referers ADD CONSTRAINT incoming_referers_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.invited_groups ADD CONSTRAINT invited_groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.invites ADD CONSTRAINT invites_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.javascript_caches ADD CONSTRAINT javascript_caches_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.message_bus ADD CONSTRAINT message_bus_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.muted_users ADD CONSTRAINT muted_users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.notifications ADD CONSTRAINT notifications_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.oauth2_user_infos ADD CONSTRAINT oauth2_user_infos_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.onceoff_logs ADD CONSTRAINT onceoff_logs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.optimized_images ADD CONSTRAINT optimized_images_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.permalinks ADD CONSTRAINT permalinks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.plugin_store_rows ADD CONSTRAINT plugin_store_rows_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.poll_options ADD CONSTRAINT poll_options_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.polls ADD CONSTRAINT polls_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.post_action_types ADD CONSTRAINT post_action_types_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.post_actions ADD CONSTRAINT post_actions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.post_custom_fields ADD CONSTRAINT post_custom_fields_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.post_details ADD CONSTRAINT post_details_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.post_reply_keys ADD CONSTRAINT post_reply_keys_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.post_revisions ADD CONSTRAINT post_revisions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.post_stats ADD CONSTRAINT post_stats_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.post_uploads ADD CONSTRAINT post_uploads_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.posts ADD CONSTRAINT posts_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.post_search_data ADD CONSTRAINT posts_search_pkey PRIMARY KEY (post_id);

ALTER TABLE ONLY public.push_subscriptions ADD CONSTRAINT push_subscriptions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.quoted_posts ADD CONSTRAINT quoted_posts_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.remote_themes ADD CONSTRAINT remote_themes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.reviewable_claimed_topics ADD CONSTRAINT reviewable_claimed_topics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.reviewable_histories ADD CONSTRAINT reviewable_histories_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.reviewable_scores ADD CONSTRAINT reviewable_scores_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.reviewables ADD CONSTRAINT reviewables_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.scheduler_stats ADD CONSTRAINT scheduler_stats_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.schema_migration_details ADD CONSTRAINT schema_migration_details_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.schema_migrations ADD CONSTRAINT schema_migrations_pkey PRIMARY KEY (version);

ALTER TABLE ONLY public.screened_emails ADD CONSTRAINT screened_emails_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.screened_ip_addresses ADD CONSTRAINT screened_ip_addresses_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.screened_urls ADD CONSTRAINT screened_urls_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.search_logs ADD CONSTRAINT search_logs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.shared_drafts ADD CONSTRAINT shared_drafts_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.single_sign_on_records ADD CONSTRAINT single_sign_on_records_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.site_settings ADD CONSTRAINT site_settings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.skipped_email_logs ADD CONSTRAINT skipped_email_logs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.stylesheet_cache ADD CONSTRAINT stylesheet_cache_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.tag_group_memberships ADD CONSTRAINT tag_group_memberships_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.tag_group_permissions ADD CONSTRAINT tag_group_permissions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.tag_groups ADD CONSTRAINT tag_groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.tag_search_data ADD CONSTRAINT tag_search_data_pkey PRIMARY KEY (tag_id);

ALTER TABLE ONLY public.tag_users ADD CONSTRAINT tag_users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.tags ADD CONSTRAINT tags_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.theme_fields ADD CONSTRAINT theme_fields_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.theme_modifier_sets ADD CONSTRAINT theme_modifier_sets_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.theme_settings ADD CONSTRAINT theme_settings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.theme_translation_overrides ADD CONSTRAINT theme_translation_overrides_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.themes ADD CONSTRAINT themes_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.top_topics ADD CONSTRAINT top_topics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topic_allowed_groups ADD CONSTRAINT topic_allowed_groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topic_allowed_users ADD CONSTRAINT topic_allowed_users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topic_custom_fields ADD CONSTRAINT topic_custom_fields_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topic_embeds ADD CONSTRAINT topic_embeds_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topic_groups ADD CONSTRAINT topic_groups_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topic_invites ADD CONSTRAINT topic_invites_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topic_link_clicks ADD CONSTRAINT topic_link_clicks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topic_links ADD CONSTRAINT topic_links_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topic_search_data ADD CONSTRAINT topic_search_data_pkey PRIMARY KEY (topic_id);

ALTER TABLE ONLY public.topic_tags ADD CONSTRAINT topic_tags_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topic_timers ADD CONSTRAINT topic_timers_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topic_users ADD CONSTRAINT topic_users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.topics ADD CONSTRAINT topics_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.translation_overrides ADD CONSTRAINT translation_overrides_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.uploads ADD CONSTRAINT uploads_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_actions ADD CONSTRAINT user_actions_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_api_keys ADD CONSTRAINT user_api_keys_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_archived_messages ADD CONSTRAINT user_archived_messages_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_associated_accounts ADD CONSTRAINT user_associated_accounts_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_auth_token_logs ADD CONSTRAINT user_auth_token_logs_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_auth_tokens ADD CONSTRAINT user_auth_tokens_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_avatars ADD CONSTRAINT user_avatars_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_badges ADD CONSTRAINT user_badges_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_custom_fields ADD CONSTRAINT user_custom_fields_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_emails ADD CONSTRAINT user_emails_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_exports ADD CONSTRAINT user_exports_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_field_options ADD CONSTRAINT user_field_options_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_fields ADD CONSTRAINT user_fields_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_histories ADD CONSTRAINT user_histories_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_open_ids ADD CONSTRAINT user_open_ids_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_profile_views ADD CONSTRAINT user_profile_views_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_profiles ADD CONSTRAINT user_profiles_pkey PRIMARY KEY (user_id);

ALTER TABLE ONLY public.user_second_factors ADD CONSTRAINT user_second_factors_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_security_keys ADD CONSTRAINT user_security_keys_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_stats ADD CONSTRAINT user_stats_pkey PRIMARY KEY (user_id);

ALTER TABLE ONLY public.user_uploads ADD CONSTRAINT user_uploads_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_visits ADD CONSTRAINT user_visits_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_warnings ADD CONSTRAINT user_warnings_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.users ADD CONSTRAINT users_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_search_data ADD CONSTRAINT users_search_pkey PRIMARY KEY (user_id);

ALTER TABLE ONLY public.watched_words ADD CONSTRAINT watched_words_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.web_crawler_requests ADD CONSTRAINT web_crawler_requests_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.web_hook_event_types ADD CONSTRAINT web_hook_event_types_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.web_hook_events ADD CONSTRAINT web_hook_events_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.web_hooks ADD CONSTRAINT web_hooks_pkey PRIMARY KEY (id);

ALTER TABLE ONLY public.user_profiles ADD CONSTRAINT fk_rails_1d362f2e97 FOREIGN KEY (profile_background_upload_id) REFERENCES public.uploads (id);

ALTER TABLE ONLY public.bookmarks ADD CONSTRAINT fk_rails_272c56774b FOREIGN KEY (topic_id) REFERENCES public.topics (id);

ALTER TABLE ONLY public.user_profiles ADD CONSTRAINT fk_rails_38ea484ed4 FOREIGN KEY (granted_title_badge_id) REFERENCES public.badges (id);

ALTER TABLE ONLY public.javascript_caches ADD CONSTRAINT fk_rails_58f94aecc4 FOREIGN KEY (theme_id) REFERENCES public.themes (id) ON DELETE CASCADE;

ALTER TABLE ONLY public.poll_votes ADD CONSTRAINT fk_rails_848ece0184 FOREIGN KEY (poll_option_id) REFERENCES public.poll_options (id);

ALTER TABLE ONLY public.uploads ADD CONSTRAINT fk_rails_8b89adf296 FOREIGN KEY (access_control_post_id) REFERENCES public.posts (id);

ALTER TABLE ONLY public.user_security_keys ADD CONSTRAINT fk_rails_90999b0454 FOREIGN KEY (user_id) REFERENCES public.users (id);

ALTER TABLE ONLY public.poll_votes ADD CONSTRAINT fk_rails_a6e6974b7e FOREIGN KEY (poll_id) REFERENCES public.polls (id);

ALTER TABLE ONLY public.poll_options ADD CONSTRAINT fk_rails_aa85becb42 FOREIGN KEY (poll_id) REFERENCES public.polls (id);

ALTER TABLE ONLY public.polls ADD CONSTRAINT fk_rails_b50b782d08 FOREIGN KEY (post_id) REFERENCES public.posts (id);

ALTER TABLE ONLY public.poll_votes ADD CONSTRAINT fk_rails_b64de9b025 FOREIGN KEY (user_id) REFERENCES public.users (id);

ALTER TABLE ONLY public.bookmarks ADD CONSTRAINT fk_rails_c1ff6fa4ac FOREIGN KEY (user_id) REFERENCES public.users (id);

ALTER TABLE ONLY public.user_profiles ADD CONSTRAINT fk_rails_ca64aa462b FOREIGN KEY (card_background_upload_id) REFERENCES public.uploads (id);

ALTER TABLE ONLY public.bookmarks ADD CONSTRAINT fk_rails_d8b54790ff FOREIGN KEY (post_id) REFERENCES public.posts (id);

ALTER TABLE ONLY public.javascript_caches ADD CONSTRAINT fk_rails_ed33506dbd FOREIGN KEY (theme_field_id) REFERENCES public.theme_fields (id) ON DELETE CASCADE;

-- WeTune schema patches
ALTER TABLE "skipped_email_logs" ALTER COLUMN "post_id" SET NOT NULL;
ALTER TABLE "skipped_email_logs" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "themes" ALTER COLUMN "remote_theme_id" SET NOT NULL;
ALTER TABLE "reviewables" ALTER COLUMN "reviewable_by_group_id" SET NOT NULL;
ALTER TABLE "reviewables" ALTER COLUMN "topic_id" SET NOT NULL;
ALTER TABLE "reviewables" ALTER COLUMN "target_id" SET NOT NULL;
ALTER TABLE "user_actions" ALTER COLUMN "target_topic_id" SET NOT NULL;
ALTER TABLE "user_actions" ALTER COLUMN "acting_user_id" SET NOT NULL;
ALTER TABLE "user_actions" ALTER COLUMN "target_post_id" SET NOT NULL;
ALTER TABLE "user_actions" ALTER COLUMN "target_user_id" SET NOT NULL;
ALTER TABLE "group_mentions" ALTER COLUMN "post_id" SET NOT NULL;
ALTER TABLE "group_mentions" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "optimized_images" ALTER COLUMN "etag" SET NOT NULL;
ALTER TABLE "notifications" ALTER COLUMN "post_action_id" SET NOT NULL;
ALTER TABLE "notifications" ALTER COLUMN "topic_id" SET NOT NULL;
ALTER TABLE "notifications" ALTER COLUMN "post_number" SET NOT NULL;
ALTER TABLE "group_histories" ALTER COLUMN "target_user_id" SET NOT NULL;
ALTER TABLE "topic_search_data" ALTER COLUMN "search_data" SET NOT NULL;
ALTER TABLE "topic_search_data" ALTER COLUMN "version" SET NOT NULL;
ALTER TABLE "tag_search_data" ALTER COLUMN "search_data" SET NOT NULL;
ALTER TABLE "invites" ALTER COLUMN "email" SET NOT NULL;
ALTER TABLE "invites" ALTER COLUMN "emailed_status" SET NOT NULL;
ALTER TABLE "categories" ALTER COLUMN "email_in" SET NOT NULL;
ALTER TABLE "categories" ALTER COLUMN "reviewable_by_group_id" SET NOT NULL;
ALTER TABLE "categories" ALTER COLUMN "search_priority" SET NOT NULL;
ALTER TABLE "categories" ALTER COLUMN "parent_category_id" SET NOT NULL;
ALTER TABLE "api_keys" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "slug" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "deleted_at" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "pinned_at" SET NOT NULL;
ALTER TABLE "topics" ALTER COLUMN "category_id" SET NOT NULL;
ALTER TABLE "top_topics" ALTER COLUMN "all_score" SET NOT NULL;
ALTER TABLE "top_topics" ALTER COLUMN "daily_score" SET NOT NULL;
ALTER TABLE "top_topics" ALTER COLUMN "monthly_score" SET NOT NULL;
ALTER TABLE "top_topics" ALTER COLUMN "topic_id" SET NOT NULL;
ALTER TABLE "top_topics" ALTER COLUMN "weekly_score" SET NOT NULL;
ALTER TABLE "top_topics" ALTER COLUMN "yearly_score" SET NOT NULL;
ALTER TABLE "javascript_caches" ALTER COLUMN "digest" SET NOT NULL;
ALTER TABLE "javascript_caches" ALTER COLUMN "theme_id" SET NOT NULL;
ALTER TABLE "javascript_caches" ALTER COLUMN "theme_field_id" SET NOT NULL;
ALTER TABLE "post_actions" ALTER COLUMN "disagreed_at" SET NOT NULL;
ALTER TABLE "screened_emails" ALTER COLUMN "last_match_at" SET NOT NULL;
ALTER TABLE "post_search_data" ALTER COLUMN "search_data" SET NOT NULL;
ALTER TABLE "post_search_data" ALTER COLUMN "version" SET NOT NULL;
ALTER TABLE "post_search_data" ALTER COLUMN "locale" SET NOT NULL;
ALTER TABLE "incoming_links" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "user_auth_token_logs" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "onceoff_logs" ALTER COLUMN "job_name" SET NOT NULL;
ALTER TABLE "posts" ALTER COLUMN "baked_version" SET NOT NULL;
ALTER TABLE "posts" ALTER COLUMN "reply_to_post_number" SET NOT NULL;
ALTER TABLE "posts" ALTER COLUMN "percent_rank" SET NOT NULL;
ALTER TABLE "posts" ALTER COLUMN "sort_order" SET NOT NULL;
ALTER TABLE "posts" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "bookmarks" ALTER COLUMN "reminder_at" SET NOT NULL;
ALTER TABLE "bookmarks" ALTER COLUMN "reminder_set_at" SET NOT NULL;
ALTER TABLE "bookmarks" ALTER COLUMN "reminder_type" SET NOT NULL;
ALTER TABLE "user_security_keys" ALTER COLUMN "last_used" SET NOT NULL;
ALTER TABLE "screened_urls" ALTER COLUMN "last_match_at" SET NOT NULL;
ALTER TABLE "topic_custom_fields" ALTER COLUMN "value" SET NOT NULL;
ALTER TABLE "child_themes" ALTER COLUMN "parent_theme_id" SET NOT NULL;
ALTER TABLE "child_themes" ALTER COLUMN "child_theme_id" SET NOT NULL;
ALTER TABLE "user_profiles" ALTER COLUMN "bio_cooked_version" SET NOT NULL;
ALTER TABLE "user_profiles" ALTER COLUMN "profile_background_upload_id" SET NOT NULL;
ALTER TABLE "user_profiles" ALTER COLUMN "granted_title_badge_id" SET NOT NULL;
ALTER TABLE "user_profiles" ALTER COLUMN "card_background_upload_id" SET NOT NULL;
ALTER TABLE "topic_links" ALTER COLUMN "extension" SET NOT NULL;
ALTER TABLE "topic_links" ALTER COLUMN "link_post_id" SET NOT NULL;
ALTER TABLE "topic_links" ALTER COLUMN "reflection" SET NOT NULL;
ALTER TABLE "topic_links" ALTER COLUMN "post_id" SET NOT NULL;
ALTER TABLE "category_search_data" ALTER COLUMN "search_data" SET NOT NULL;
ALTER TABLE "post_stats" ALTER COLUMN "post_id" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "last_posted_at" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "last_seen_at" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "secure_identifier" SET NOT NULL;
ALTER TABLE "users" ALTER COLUMN "uploaded_avatar_id" SET NOT NULL;
ALTER TABLE "category_users" ALTER COLUMN "last_seen_at" SET NOT NULL;
ALTER TABLE "post_revisions" ALTER COLUMN "post_id" SET NOT NULL;
ALTER TABLE "post_revisions" ALTER COLUMN "number" SET NOT NULL;
ALTER TABLE "group_requests" ALTER COLUMN "group_id" SET NOT NULL;
ALTER TABLE "group_requests" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "screened_ip_addresses" ALTER COLUMN "last_match_at" SET NOT NULL;
ALTER TABLE "user_search_data" ALTER COLUMN "search_data" SET NOT NULL;
ALTER TABLE "topic_views" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "topic_views" ALTER COLUMN "ip_address" SET NOT NULL;
ALTER TABLE "post_replies" ALTER COLUMN "post_id" SET NOT NULL;
ALTER TABLE "post_replies" ALTER COLUMN "reply_post_id" SET NOT NULL;
ALTER TABLE "polls" ALTER COLUMN "post_id" SET NOT NULL;
ALTER TABLE "poll_options" ALTER COLUMN "poll_id" SET NOT NULL;
ALTER TABLE "user_avatars" ALTER COLUMN "custom_upload_id" SET NOT NULL;
ALTER TABLE "user_avatars" ALTER COLUMN "gravatar_upload_id" SET NOT NULL;
ALTER TABLE "user_badges" ALTER COLUMN "post_id" SET NOT NULL;
ALTER TABLE "email_logs" ALTER COLUMN "bounce_key" SET NOT NULL;
ALTER TABLE "email_logs" ALTER COLUMN "message_id" SET NOT NULL;
ALTER TABLE "email_logs" ALTER COLUMN "post_id" SET NOT NULL;
ALTER TABLE "email_logs" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "incoming_emails" ALTER COLUMN "error" SET NOT NULL;
ALTER TABLE "incoming_emails" ALTER COLUMN "message_id" SET NOT NULL;
ALTER TABLE "incoming_emails" ALTER COLUMN "post_id" SET NOT NULL;
ALTER TABLE "incoming_emails" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "groups" ALTER COLUMN "incoming_email" SET NOT NULL;
ALTER TABLE "post_details" ALTER COLUMN "post_id" SET NOT NULL;
ALTER TABLE "post_details" ALTER COLUMN "key" SET NOT NULL;
ALTER TABLE "user_visits" ALTER COLUMN "mobile" SET NOT NULL;
ALTER TABLE "uploads" ALTER COLUMN "etag" SET NOT NULL;
ALTER TABLE "uploads" ALTER COLUMN "original_sha1" SET NOT NULL;
ALTER TABLE "uploads" ALTER COLUMN "sha1" SET NOT NULL;
ALTER TABLE "uploads" ALTER COLUMN "access_control_post_id" SET NOT NULL;
ALTER TABLE "uploads" ALTER COLUMN "extension" SET NOT NULL;
ALTER TABLE "poll_votes" ALTER COLUMN "poll_option_id" SET NOT NULL;
ALTER TABLE "poll_votes" ALTER COLUMN "poll_id" SET NOT NULL;
ALTER TABLE "poll_votes" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "user_profile_views" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "user_profile_views" ALTER COLUMN "ip_address" SET NOT NULL;
ALTER TABLE "user_associated_accounts" ALTER COLUMN "user_id" SET NOT NULL;
ALTER TABLE "user_histories" ALTER COLUMN "acting_user_id" SET NOT NULL;
ALTER TABLE "user_histories" ALTER COLUMN "category_id" SET NOT NULL;
ALTER TABLE "user_histories" ALTER COLUMN "subject" SET NOT NULL;
ALTER TABLE "user_histories" ALTER COLUMN "topic_id" SET NOT NULL;
ALTER TABLE "user_histories" ALTER COLUMN "target_user_id" SET NOT NULL;
ALTER TABLE "user_emails" ADD CONSTRAINT "wetune_u_2d582938a9687bd8" UNIQUE ("email");
ALTER TABLE "tags" ADD CONSTRAINT "wetune_u_340effabd8635563" UNIQUE ("name");
ALTER TABLE "topics" ADD CONSTRAINT "wetune_u_d7b247fd40ff4cd1" UNIQUE ("title");
ALTER TABLE "uploads" ADD CONSTRAINT "wetune_u_4a9954fd8a660bdf" UNIQUE ("extension");
ALTER TABLE "categories" ADD CONSTRAINT "wetune_u_527758b24209fd97" UNIQUE ("parent_category_id");
ALTER TABLE "tag_group_memberships" ADD CONSTRAINT "wetune_u_04cd806125c7740a" UNIQUE ("tag_id", "tag_group_id");
ALTER TABLE "category_tag_groups" ADD CONSTRAINT "wetune_u_02c956277fc57524" UNIQUE ("category_id", "tag_group_id");
ALTER TABLE "category_groups" ADD CONSTRAINT "wetune_u_b74af370a1679102" UNIQUE ("category_id", "group_id");
ALTER TABLE "tag_group_permissions" ADD CONSTRAINT "wetune_u_dd65135368bc64b7" UNIQUE ("group_id", "permission_type", "tag_group_id");
ALTER TABLE "category_search_data" ADD CONSTRAINT "wetune_fk_eb9e5866919564e0" FOREIGN KEY ("category_id") REFERENCES "categories" ("id");
ALTER TABLE "anonymous_users" ADD CONSTRAINT "wetune_fk_06bdb2b52d92e8c2" FOREIGN KEY ("master_user_id") REFERENCES "users" ("id");
ALTER TABLE "posts" ADD CONSTRAINT "wetune_fk_e95b582c7bffc24e" FOREIGN KEY ("deleted_by_id") REFERENCES "users" ("id");
ALTER TABLE "topic_links" ADD CONSTRAINT "wetune_fk_6f98c7b58faedc98" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "user_archived_messages" ADD CONSTRAINT "wetune_fk_70bc444e4592ec1a" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "category_groups" ADD CONSTRAINT "wetune_fk_221c1be4bc69c1e6" FOREIGN KEY ("category_id") REFERENCES "categories" ("id");
ALTER TABLE "reviewable_scores" ADD CONSTRAINT "wetune_fk_6e3e6b4f431842b4" FOREIGN KEY ("reviewable_id") REFERENCES "reviewables" ("id");
ALTER TABLE "groups_web_hooks" ADD CONSTRAINT "wetune_fk_dc5716815ab5b411" FOREIGN KEY ("group_id") REFERENCES "groups" ("id");
ALTER TABLE "user_avatars" ADD CONSTRAINT "wetune_fk_7c7aef1cdbde318e" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "user_actions" ADD CONSTRAINT "wetune_fk_94e712424f4632b4" FOREIGN KEY ("target_post_id") REFERENCES "posts" ("id");
ALTER TABLE "skipped_email_logs" ADD CONSTRAINT "wetune_fk_c831eae5d0da1fa8" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "user_custom_fields" ADD CONSTRAINT "wetune_fk_126241dfa40751ba" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "category_featured_topics" ADD CONSTRAINT "wetune_fk_b800ca2ed982a131" FOREIGN KEY ("category_id") REFERENCES "categories" ("id");
ALTER TABLE "post_replies" ADD CONSTRAINT "wetune_fk_4dcd746e7a5b4c43" FOREIGN KEY ("reply_post_id") REFERENCES "posts" ("id");
ALTER TABLE "posts" ADD CONSTRAINT "wetune_fk_c0efe10a9d499142" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "topic_users" ADD CONSTRAINT "wetune_fk_7be90a4f1b335b2b" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "user_actions" ADD CONSTRAINT "wetune_fk_f907d95a548821f3" FOREIGN KEY ("target_topic_id") REFERENCES "topics" ("id");
ALTER TABLE "incoming_links" ADD CONSTRAINT "wetune_fk_8695e31b5849f9dc" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "muted_users" ADD CONSTRAINT "wetune_fk_501be9bd2bbea618" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "reviewables" ADD CONSTRAINT "wetune_fk_29d961d102c9a546" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "post_details" ADD CONSTRAINT "wetune_fk_61d6e7ac72a68fde" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "ignored_users" ADD CONSTRAINT "wetune_fk_e73d2eac247271f6" FOREIGN KEY ("ignored_user_id") REFERENCES "users" ("id");
ALTER TABLE "tag_users" ADD CONSTRAINT "wetune_fk_7d348e1960d4d9fe" FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");
ALTER TABLE "group_requests" ADD CONSTRAINT "wetune_fk_e19e94c5a5e40cf6" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "topic_allowed_groups" ADD CONSTRAINT "wetune_fk_4b293c4070ac9e41" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "group_users" ADD CONSTRAINT "wetune_fk_8ef5c2c78819b272" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "email_tokens" ADD CONSTRAINT "wetune_fk_6a034fc94ca397ef" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "topic_allowed_groups" ADD CONSTRAINT "wetune_fk_7091e461ccf494bb" FOREIGN KEY ("group_id") REFERENCES "groups" ("id");
ALTER TABLE "categories_web_hooks" ADD CONSTRAINT "wetune_fk_2bfd2a8360282d52" FOREIGN KEY ("category_id") REFERENCES "categories" ("id");
ALTER TABLE "muted_users" ADD CONSTRAINT "wetune_fk_4f6c62efa631a1b4" FOREIGN KEY ("muted_user_id") REFERENCES "users" ("id");
ALTER TABLE "incoming_referers" ADD CONSTRAINT "wetune_fk_5fa89a803f8fdd11" FOREIGN KEY ("incoming_domain_id") REFERENCES "incoming_domains" ("id");
ALTER TABLE "invites" ADD CONSTRAINT "wetune_fk_99f7c795a632fbde" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "top_topics" ADD CONSTRAINT "wetune_fk_507489bc2edb3b2d" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "post_replies" ADD CONSTRAINT "wetune_fk_ba171beedffe0d4e" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "tag_group_permissions" ADD CONSTRAINT "wetune_fk_d1d58b1625c03340" FOREIGN KEY ("tag_group_id") REFERENCES "tag_groups" ("id");
ALTER TABLE "topic_tags" ADD CONSTRAINT "wetune_fk_35e2a928be0d4216" FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");
ALTER TABLE "post_custom_fields" ADD CONSTRAINT "wetune_fk_3116e5bc9d4f9844" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "category_tag_groups" ADD CONSTRAINT "wetune_fk_62a191d13b8af0cf" FOREIGN KEY ("category_id") REFERENCES "categories" ("id");
ALTER TABLE "topic_invites" ADD CONSTRAINT "wetune_fk_4bed4121eaeb2380" FOREIGN KEY ("invite_id") REFERENCES "invites" ("id");
ALTER TABLE "user_badges" ADD CONSTRAINT "wetune_fk_468cc29aa3224ee9" FOREIGN KEY ("badge_id") REFERENCES "badges" ("id");
ALTER TABLE "user_emails" ADD CONSTRAINT "wetune_fk_4139065dad297014" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "user_options" ADD CONSTRAINT "wetune_fk_01f48998efe98c66" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "skipped_email_logs" ADD CONSTRAINT "wetune_fk_891374f0e6299dfd" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "user_auth_tokens" ADD CONSTRAINT "wetune_fk_29049202348e816f" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "email_logs" ADD CONSTRAINT "wetune_fk_a2773b11c7c5abb2" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "category_users" ADD CONSTRAINT "wetune_fk_e9508242aa92209a" FOREIGN KEY ("category_id") REFERENCES "categories" ("id");
ALTER TABLE "categories" ADD CONSTRAINT "wetune_fk_d31386dba4f918d5" FOREIGN KEY ("uploaded_background_id") REFERENCES "uploads" ("id");
ALTER TABLE "ignored_users" ADD CONSTRAINT "wetune_fk_72b8512ec93709a2" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "tag_group_permissions" ADD CONSTRAINT "wetune_fk_bc9718235a6a3e2b" FOREIGN KEY ("group_id") REFERENCES "groups" ("id");
ALTER TABLE "topics" ADD CONSTRAINT "wetune_fk_280d75bea48e131f" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "tags" ADD CONSTRAINT "wetune_fk_944c4647526d3839" FOREIGN KEY ("target_tag_id") REFERENCES "tags" ("id");
ALTER TABLE "group_archived_messages" ADD CONSTRAINT "wetune_fk_e8e63e7ac274282f" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "post_actions" ADD CONSTRAINT "wetune_fk_7566d9b2e12fda66" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "topic_invites" ADD CONSTRAINT "wetune_fk_bef00b7ede72f176" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "topic_tags" ADD CONSTRAINT "wetune_fk_637bd46d55d213f7" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "category_tags" ADD CONSTRAINT "wetune_fk_cff41ff1821b7d7a" FOREIGN KEY ("category_id") REFERENCES "categories" ("id");
ALTER TABLE "categories" ADD CONSTRAINT "wetune_fk_22512f26c1673960" FOREIGN KEY ("uploaded_logo_id") REFERENCES "uploads" ("id");
ALTER TABLE "topic_users" ADD CONSTRAINT "wetune_fk_75a5e4f85cde5689" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "post_uploads" ADD CONSTRAINT "wetune_fk_8f25571843c2ba00" FOREIGN KEY ("upload_id") REFERENCES "uploads" ("id");
ALTER TABLE "topic_embeds" ADD CONSTRAINT "wetune_fk_5d40b0a3745c19ea" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "incoming_links" ADD CONSTRAINT "wetune_fk_7253c1a82c05165b" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "invited_groups" ADD CONSTRAINT "wetune_fk_4f7a9300f6ad3fac" FOREIGN KEY ("group_id") REFERENCES "groups" ("id");
ALTER TABLE "web_hook_event_types_hooks" ADD CONSTRAINT "wetune_fk_611bfb91ee67859c" FOREIGN KEY ("web_hook_id") REFERENCES "web_hooks" ("id");
ALTER TABLE "categories" ADD CONSTRAINT "wetune_fk_ac0cb134e73b64ac" FOREIGN KEY ("parent_category_id") REFERENCES "categories" ("id");
ALTER TABLE "incoming_emails" ADD CONSTRAINT "wetune_fk_362452ea64b322b2" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "child_themes" ADD CONSTRAINT "wetune_fk_e971ced338da6c81" FOREIGN KEY ("parent_theme_id") REFERENCES "themes" ("id");
ALTER TABLE "topic_links" ADD CONSTRAINT "wetune_fk_522ebb4207f51f5c" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "tag_users" ADD CONSTRAINT "wetune_fk_8671eaf8be42d249" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "topic_allowed_users" ADD CONSTRAINT "wetune_fk_95ef0e19df0a52b1" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "anonymous_users" ADD CONSTRAINT "wetune_fk_cc74a0d4b6bd3c30" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "group_mentions" ADD CONSTRAINT "wetune_fk_54915e2f487a7551" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "category_groups" ADD CONSTRAINT "wetune_fk_69085827d8913bd1" FOREIGN KEY ("group_id") REFERENCES "groups" ("id");
ALTER TABLE "web_hook_event_types_hooks" ADD CONSTRAINT "wetune_fk_0a324c37ab29bd1a" FOREIGN KEY ("web_hook_event_type_id") REFERENCES "web_hook_event_types" ("id");
ALTER TABLE "category_tags" ADD CONSTRAINT "wetune_fk_206d2297a2796754" FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");
ALTER TABLE "api_keys" ADD CONSTRAINT "wetune_fk_e1dc132135e7073b" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "categories" ADD CONSTRAINT "wetune_fk_6b80c85d346c6f48" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "tags_web_hooks" ADD CONSTRAINT "wetune_fk_6283b6ebd3d24ff3" FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");
ALTER TABLE "incoming_links" ADD CONSTRAINT "wetune_fk_37de91f1926908b8" FOREIGN KEY ("incoming_referer_id") REFERENCES "incoming_referers" ("id");
ALTER TABLE "directory_items" ADD CONSTRAINT "wetune_fk_65be763f6985652d" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "users" ADD CONSTRAINT "wetune_fk_d38b78278425f0a7" FOREIGN KEY ("primary_group_id") REFERENCES "groups" ("id");
ALTER TABLE "theme_fields" ADD CONSTRAINT "wetune_fk_a519977536e7441c" FOREIGN KEY ("upload_id") REFERENCES "uploads" ("id");
ALTER TABLE "child_themes" ADD CONSTRAINT "wetune_fk_e7f5695cb3e55f50" FOREIGN KEY ("child_theme_id") REFERENCES "themes" ("id");
ALTER TABLE "tag_group_memberships" ADD CONSTRAINT "wetune_fk_256450574710e69e" FOREIGN KEY ("tag_id") REFERENCES "tags" ("id");
ALTER TABLE "posts" ADD CONSTRAINT "wetune_fk_ffd617bc97b0d4ab" FOREIGN KEY ("reply_to_user_id") REFERENCES "users" ("id");
ALTER TABLE "topics" ADD CONSTRAINT "wetune_fk_e2ebec9f5f38ac6c" FOREIGN KEY ("category_id") REFERENCES "categories" ("id");
ALTER TABLE "category_featured_topics" ADD CONSTRAINT "wetune_fk_f04207d6e1302f8e" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "category_users" ADD CONSTRAINT "wetune_fk_2ee5253551d7ada6" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "category_tag_groups" ADD CONSTRAINT "wetune_fk_17be8f43968c2696" FOREIGN KEY ("tag_group_id") REFERENCES "tag_groups" ("id");
ALTER TABLE "post_uploads" ADD CONSTRAINT "wetune_fk_0eb94cd69c26bda6" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "quoted_posts" ADD CONSTRAINT "wetune_fk_34dd3a34b4cf54c6" FOREIGN KEY ("post_id") REFERENCES "posts" ("id");
ALTER TABLE "tag_group_memberships" ADD CONSTRAINT "wetune_fk_7422f20e331f1ac4" FOREIGN KEY ("tag_group_id") REFERENCES "tag_groups" ("id");
ALTER TABLE "notifications" ADD CONSTRAINT "wetune_fk_ee0e1a0c301106fd" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "group_users" ADD CONSTRAINT "wetune_fk_9724bfba110a185d" FOREIGN KEY ("group_id") REFERENCES "groups" ("id");
ALTER TABLE "topic_allowed_users" ADD CONSTRAINT "wetune_fk_a6a68ffada42d802" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "topic_links" ADD CONSTRAINT "wetune_fk_76a49e7ae2f2e2f3" FOREIGN KEY ("topic_id") REFERENCES "topics" ("id");
ALTER TABLE "posts" ADD CONSTRAINT "wetune_fk_4be40a544da4bfdb" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
ALTER TABLE "reviewables" ADD CONSTRAINT "wetune_fk_357d128042c2a2b9" FOREIGN KEY ("target_id") REFERENCES "posts" ("id");
ALTER TABLE "quoted_posts" ADD CONSTRAINT "wetune_fk_1ce6d8ef3edcc9e4" FOREIGN KEY ("quoted_post_id") REFERENCES "posts" ("id");
ALTER TABLE "email_logs" ADD CONSTRAINT "wetune_fk_5038ecaa6b9f863b" FOREIGN KEY ("user_id") REFERENCES "users" ("id");
