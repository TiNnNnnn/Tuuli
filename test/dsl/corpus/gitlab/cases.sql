SELECT "n".* FROM "notes" AS "n" WHERE "n"."type" = 'D' AND "n"."id" IN (SELECT "m"."id" FROM "notes" AS "m" WHERE "m"."commit_id" = '10232')







































































































































































































































































































































SELECT "licenses".* FROM "licenses"
SELECT "routes".* FROM "routes" WHERE (LOWER("routes"."path") = LOWER('user1')) /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "projects".* FROM "projects" INNER JOIN routes AS rs ON rs.source_id = projects.id AND rs.source_type = 'Project' WHERE (rs.path LIKE 'user1/%') /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "users".* FROM "users" WHERE "users"."incoming_email_token" = $1 LIMIT $2 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */






SELECT "uploads".* FROM "uploads" WHERE "uploads"."model_id" = $1 AND "uploads"."model_type" = $2 AND "uploads"."uploader" IN ($3, $4, $5) AND "uploads"."store" = $6 ORDER BY "uploads"."id" ASC LIMIT $7 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */

SELECT DISTINCT (repository_storage) FROM "projects" INNER JOIN routes AS rs ON rs.source_id = projects.id AND rs.source_type = 'Project' WHERE (rs.path LIKE 'user2/%') /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */

SELECT "redirect_routes".* FROM "redirect_routes" WHERE "redirect_routes"."source_id" = $1 AND "redirect_routes"."source_type" = $2 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "projects".* FROM "projects" WHERE "projects"."namespace_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "chat_teams".* FROM "chat_teams" WHERE "chat_teams"."namespace_id" = $1 LIMIT $2 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "gitlab_subscriptions".* FROM "gitlab_subscriptions" WHERE "gitlab_subscriptions"."namespace_id" = $1 LIMIT $2 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */

SELECT "keys".* FROM "keys" WHERE "keys"."user_id" = $1 AND ("keys"."type" IN ($2, $3) OR "keys"."type" IS NULL) /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */

SELECT "personal_access_tokens".* FROM "personal_access_tokens" WHERE "personal_access_tokens"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "identities".* FROM "identities" WHERE "identities"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "u2f_registrations".* FROM "u2f_registrations" WHERE "u2f_registrations"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "chat_names".* FROM "chat_names" WHERE "chat_names"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "users_star_projects".* FROM "users_star_projects" WHERE "users_star_projects"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */

SELECT "snippets".* FROM "snippets" WHERE "snippets"."author_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "notes".* FROM "notes" WHERE "notes"."author_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "issues".* FROM "issues" WHERE "issues"."author_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "merge_requests".* FROM "merge_requests" WHERE "merge_requests"."author_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */


SELECT "subscriptions".* FROM "subscriptions" WHERE "subscriptions"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "oauth_applications".* FROM "oauth_applications" WHERE "oauth_applications"."owner_id" = $1 AND "oauth_applications"."owner_type" = $2 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "abuse_reports".* FROM "abuse_reports" WHERE "abuse_reports"."user_id" = $1 LIMIT $2 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "abuse_reports".* FROM "abuse_reports" WHERE "abuse_reports"."reporter_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "spam_logs".* FROM "spam_logs" WHERE "spam_logs"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */


SELECT "award_emoji".* FROM "award_emoji" WHERE "award_emoji"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */

SELECT "path_locks".* FROM "path_locks" WHERE "path_locks"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "approvals".* FROM "approvals" WHERE "approvals"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "approvers".* FROM "approvers" WHERE "approvers"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "protected_branch_merge_access_levels".* FROM "protected_branch_merge_access_levels" WHERE "protected_branch_merge_access_levels"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "protected_branch_push_access_levels".* FROM "protected_branch_push_access_levels" WHERE "protected_branch_push_access_levels"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "protected_branch_unprotect_access_levels".* FROM "protected_branch_unprotect_access_levels" WHERE "protected_branch_unprotect_access_levels"."user_id" = $1 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */

SELECT "web_hooks".* FROM "web_hooks" WHERE "web_hooks"."type" = $1 ORDER BY "web_hooks"."id" ASC LIMIT $2 /* application:test,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "users".* FROM "users" WHERE "users"."id" = $1 ORDER BY "users"."id" ASC LIMIT $2 /* application:test,controller:abuse_reports,action:new,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "user_preferences".* FROM "user_preferences" WHERE "user_preferences"."user_id" = $1 LIMIT $2 /* application:test,controller:abuse_reports,action:new,correlation_id:75a0a5d540f63471310aefd1c8d62587 */
SELECT "users".* FROM "users" WHERE "users"."id" = $1 LIMIT $2 /* application:test,controller:abuse_reports,action:new,correlation_id:75a0a5d540f63471310aefd1c8d62587 */





SELECT "ci_pipelines".* FROM "ci_pipelines" WHERE "ci_pipelines"."user_id" = $1 AND "ci_pipelines"."status" IN ($2, $3, $4, $5, $6, $7) ORDER BY "ci_pipelines"."id" ASC LIMIT $8 /* application:test,correlation_id:1741cc87b49b285316122edc85cfc5a2 */


SELECT COUNT(*) FROM "abuse_reports" /* application:test,correlation_id:ad7a72b3838faa0a2e10cd886f576d64 */

















SELECT "pages_domain_acme_orders".* FROM "pages_domain_acme_orders" INNER JOIN "pages_domains" ON "pages_domains"."id" = "pages_domain_acme_orders"."pages_domain_id" WHERE "pages_domains"."domain" = $1 AND "pages_domain_acme_orders"."challenge_token" = $2 LIMIT $3 /* application:test,controller:acme_challenges,action:show,correlation_id:d70867fbb31209836c9f98f9fdc93939 */




SELECT "appearances".* FROM "appearances" ORDER BY "appearances"."id" ASC LIMIT $1 /* application:test,correlation_id:2e362e9a1f9ae42c422ac6ca24898540 */










SELECT "application_settings".* FROM "application_settings" ORDER BY "application_settings"."id" DESC LIMIT $1 /* application:test,correlation_id:15829df51824e682dd7c262cd5b7f10c */
SELECT "licenses".* FROM "licenses" ORDER BY "licenses"."id" DESC LIMIT $1 /* application:test,correlation_id:15829df51824e682dd7c262cd5b7f10c */
SELECT "application_settings".* FROM "application_settings" WHERE "application_settings"."runners_registration_token_encrypted" = $1 LIMIT $2 /* application:test,correlation_id:15829df51824e682dd7c262cd5b7f10c */
SELECT "application_settings".* FROM "application_settings" WHERE "application_settings"."runners_registration_token" = $1 LIMIT $2 /* application:test,correlation_id:15829df51824e682dd7c262cd5b7f10c */
SELECT "application_settings".* FROM "application_settings" WHERE "application_settings"."health_check_access_token" = $1 LIMIT $2 /* application:test,correlation_id:15829df51824e682dd7c262cd5b7f10c */

SELECT "application_setting_terms".* FROM "application_setting_terms" ORDER BY "application_setting_terms"."id" DESC LIMIT $1 /* application:test,correlation_id:15829df51824e682dd7c262cd5b7f10c */




SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_cert_managers" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_cert_managers"."cluster_id" WHERE "clusters_applications_cert_managers"."created_at" BETWEEN $1 AND $2 AND "clusters_applications_cert_managers"."status" IN ($3, $4) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_helm" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_helm"."cluster_id" WHERE "clusters_applications_helm"."created_at" BETWEEN $1 AND $2 AND "clusters_applications_helm"."status" IN ($3, $4) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_ingress" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_ingress"."cluster_id" WHERE "clusters_applications_ingress"."created_at" BETWEEN $1 AND $2 AND "clusters_applications_ingress"."status" IN ($3, $4) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_knative" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_knative"."cluster_id" WHERE "clusters_applications_knative"."created_at" BETWEEN $1 AND $2 AND "clusters_applications_knative"."status" IN ($3, $4) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."created_at" BETWEEN $2 AND $3 AND "clusters"."user_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_gcp" ON "cluster_providers_gcp"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_gcp"."status" IN (3)) AND "clusters"."enabled" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_gcp" ON "cluster_providers_gcp"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_gcp"."status" IN (3)) AND "clusters"."enabled" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_gcp" ON "cluster_providers_gcp"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_gcp"."status" IN (3)) AND "clusters"."enabled" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 AND "clusters"."user_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_aws" ON "cluster_providers_aws"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_aws"."status" IN (3)) AND "clusters"."enabled" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_aws" ON "cluster_providers_aws"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_aws"."status" IN (3)) AND "clusters"."enabled" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_aws" ON "cluster_providers_aws"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_aws"."status" IN (3)) AND "clusters"."enabled" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 AND "clusters"."user_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" WHERE "clusters"."provider_type" = $1 AND "clusters"."enabled" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" WHERE "clusters"."provider_type" = $1 AND "clusters"."enabled" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" WHERE "clusters"."provider_type" = $1 AND "clusters"."enabled" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 AND "clusters"."user_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."cluster_type" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."cluster_type" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."cluster_type" = $2 AND "clusters"."created_at" BETWEEN $3 AND $4 AND "clusters"."user_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "projects"."created_at" BETWEEN $2 AND $3 AND "projects"."creator_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("keys"."user_id") FROM "keys" WHERE "keys"."type" = $1 AND "keys"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("keys"."user_id") FROM "keys" WHERE "keys"."type" = $1 AND "keys"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "keys"."user_id") FROM "keys" WHERE "keys"."type" = $1 AND "keys"."created_at" BETWEEN $2 AND $3 AND "keys"."user_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("keys"."user_id") FROM "keys" WHERE ("keys"."type" IN ($1, $2) OR "keys"."type" IS NULL) AND "keys"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("keys"."user_id") FROM "keys" WHERE ("keys"."type" IN ($1, $2) OR "keys"."type" IS NULL) AND "keys"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "keys"."user_id") FROM "keys" WHERE ("keys"."type" IN ($1, $2) OR "keys"."type" IS NULL) AND "keys"."created_at" BETWEEN $3 AND $4 AND "keys"."user_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("merge_requests"."author_id") FROM "merge_requests" WHERE "merge_requests"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("merge_requests"."author_id") FROM "merge_requests" WHERE "merge_requests"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "merge_requests"."author_id") FROM "merge_requests" WHERE "merge_requests"."created_at" BETWEEN $1 AND $2 AND "merge_requests"."author_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "protected_branches"."code_owner_approval_required" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "protected_branches"."code_owner_approval_required" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "protected_branches"."code_owner_approval_required" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 AND "projects"."creator_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND "approval_merge_request_rules"."approvals_required" = $3 AND "approval_merge_request_rules"."created_at" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND "approval_merge_request_rules"."approvals_required" = $3 AND "approval_merge_request_rules"."created_at" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND "approval_merge_request_rules"."approvals_required" = $3 AND "approval_merge_request_rules"."created_at" BETWEEN $4 AND $5 AND "approval_merge_request_rules"."merge_request_id" BETWEEN $6 AND $7 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND (approvals_required > 0) AND "approval_merge_request_rules"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND (approvals_required > 0) AND "approval_merge_request_rules"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND (approvals_required > 0) AND "approval_merge_request_rules"."created_at" BETWEEN $3 AND $4 AND "approval_merge_request_rules"."merge_request_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" WHERE "projects"."import_type" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" WHERE "projects"."import_type" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" WHERE "projects"."import_type" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 AND "projects"."creator_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "project_features" ON "project_features"."project_id" = "projects"."id" WHERE "project_features"."repository_access_level" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "project_features" ON "project_features"."project_id" = "projects"."id" WHERE "project_features"."repository_access_level" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "project_features" ON "project_features"."project_id" = "projects"."id" WHERE "project_features"."repository_access_level" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 AND "projects"."creator_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "projects"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "projects"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "projects"."created_at" BETWEEN $1 AND $2 AND "projects"."creator_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "remote_mirrors" ON "remote_mirrors"."project_id" = "projects"."id" WHERE "remote_mirrors"."enabled" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "remote_mirrors" ON "remote_mirrors"."project_id" = "projects"."id" WHERE "remote_mirrors"."enabled" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "remote_mirrors" ON "remote_mirrors"."project_id" = "projects"."id" WHERE "remote_mirrors"."enabled" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 AND "projects"."creator_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("snippets"."author_id") FROM "snippets" WHERE "snippets"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("snippets"."author_id") FROM "snippets" WHERE "snippets"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "snippets"."author_id") FROM "snippets" WHERE "snippets"."created_at" BETWEEN $1 AND $2 AND "snippets"."author_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("notes"."author_id") FROM "notes" INNER JOIN "suggestions" ON "suggestions"."note_id" = "notes"."id" WHERE "notes"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("notes"."author_id") FROM "notes" INNER JOIN "suggestions" ON "suggestions"."note_id" = "notes"."id" WHERE "notes"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "notes"."author_id") FROM "notes" INNER JOIN "suggestions" ON "suggestions"."note_id" = "notes"."id" WHERE "notes"."created_at" BETWEEN $1 AND $2 AND "notes"."author_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("events"."author_id") FROM "events" WHERE "events"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("events"."author_id") FROM "events" WHERE "events"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "events"."author_id") FROM "events" WHERE "events"."created_at" BETWEEN $1 AND $2 AND "events"."author_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 AND "members"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 AND "members"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 AND "members"."created_at" BETWEEN $3 AND $4 AND "members"."user_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 AND "members"."ldap" = $3 AND "members"."created_at" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 AND "members"."ldap" = $3 AND "members"."created_at" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 AND "members"."ldap" = $3 AND "members"."created_at" BETWEEN $4 AND $5 AND "members"."user_id" BETWEEN $6 AND $7 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("users"."id") FROM "users" WHERE "users"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("users"."id") FROM "users" WHERE "users"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("users"."id") FROM "users" WHERE "users"."created_at" BETWEEN $1 AND $2 AND "users"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" WHERE "clusters"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" WHERE "clusters"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" WHERE "clusters"."created_at" BETWEEN $1 AND $2 AND "clusters"."user_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_prometheus" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_prometheus"."cluster_id" WHERE "clusters_applications_prometheus"."created_at" BETWEEN $1 AND $2 AND "clusters_applications_prometheus"."status" IN ($3, $4) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."dashboard" = $4 AND "users"."created_at" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."dashboard" = $4 AND "users"."created_at" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."dashboard" = $4 AND "users"."created_at" BETWEEN $5 AND $6 AND "users"."id" BETWEEN $7 AND $8 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("users_ops_dashboard_projects"."user_id") FROM "users_ops_dashboard_projects" INNER JOIN "users" ON "users"."id" = "users_ops_dashboard_projects"."user_id" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users_ops_dashboard_projects"."created_at" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("users_ops_dashboard_projects"."user_id") FROM "users_ops_dashboard_projects" INNER JOIN "users" ON "users"."id" = "users_ops_dashboard_projects"."user_id" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users_ops_dashboard_projects"."created_at" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "users_ops_dashboard_projects"."user_id") FROM "users_ops_dashboard_projects" INNER JOIN "users" ON "users"."id" = "users_ops_dashboard_projects"."user_id" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users_ops_dashboard_projects"."created_at" BETWEEN $4 AND $5 AND "users_ops_dashboard_projects"."user_id" BETWEEN $6 AND $7 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."active" = $3 AND "projects"."created_at" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."active" = $3 AND "projects"."created_at" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."active" = $3 AND "projects"."created_at" BETWEEN $4 AND $5 AND "projects"."creator_id" BETWEEN $6 AND $7 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "project_error_tracking_settings" ON "project_error_tracking_settings"."project_id" = "projects"."id" WHERE "project_error_tracking_settings"."enabled" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "project_error_tracking_settings" ON "project_error_tracking_settings"."project_id" = "projects"."id" WHERE "project_error_tracking_settings"."enabled" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "project_error_tracking_settings" ON "project_error_tracking_settings"."project_id" = "projects"."id" WHERE "project_error_tracking_settings"."enabled" = $1 AND "projects"."created_at" BETWEEN $2 AND $3 AND "projects"."creator_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "project_tracing_settings" ON "project_tracing_settings"."project_id" = "projects"."id" WHERE "projects"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "project_tracing_settings" ON "project_tracing_settings"."project_id" = "projects"."id" WHERE "projects"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "project_tracing_settings" ON "project_tracing_settings"."project_id" = "projects"."id" WHERE "projects"."created_at" BETWEEN $1 AND $2 AND "projects"."creator_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "packages_packages" ON "packages_packages"."project_id" = "projects"."id" WHERE "projects"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "packages_packages" ON "packages_packages"."project_id" = "projects"."id" WHERE "projects"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "packages_packages" ON "packages_packages"."project_id" = "projects"."id" WHERE "projects"."created_at" BETWEEN $1 AND $2 AND "projects"."creator_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("lists"."user_id") FROM "lists" WHERE "lists"."list_type" = $1 AND "lists"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("lists"."user_id") FROM "lists" WHERE "lists"."list_type" = $1 AND "lists"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "lists"."user_id") FROM "lists" WHERE "lists"."list_type" = $1 AND "lists"."created_at" BETWEEN $2 AND $3 AND "lists"."user_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("epics"."author_id") FROM "epics" WHERE "epics"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("epics"."author_id") FROM "epics" WHERE "epics"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "epics"."author_id") FROM "epics" WHERE "epics"."created_at" BETWEEN $1 AND $2 AND "epics"."author_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("issues"."author_id") FROM "issues" WHERE "issues"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("issues"."author_id") FROM "issues" WHERE "issues"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "issues"."author_id") FROM "issues" WHERE "issues"."created_at" BETWEEN $1 AND $2 AND "issues"."author_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("notes"."author_id") FROM "notes" WHERE "notes"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("notes"."author_id") FROM "notes" WHERE "notes"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "notes"."author_id") FROM "notes" WHERE "notes"."created_at" BETWEEN $1 AND $2 AND "notes"."author_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" WHERE "projects"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" WHERE "projects"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" WHERE "projects"."created_at" BETWEEN $1 AND $2 AND "projects"."creator_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND "projects"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND "projects"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND "projects"."created_at" BETWEEN $3 AND $4 AND "projects"."creator_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_cloud_last_sync_at" IS NULL AND "projects"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_cloud_last_sync_at" IS NULL AND "projects"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_cloud_last_sync_at" IS NULL AND "projects"."created_at" BETWEEN $3 AND $4 AND "projects"."creator_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_server_last_sync_at" IS NULL AND "projects"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_server_last_sync_at" IS NULL AND "projects"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_server_last_sync_at" IS NULL AND "projects"."created_at" BETWEEN $3 AND $4 AND "projects"."creator_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."active" = $1 AND "projects"."service_desk_enabled" = $2 AND "projects"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."active" = $1 AND "projects"."service_desk_enabled" = $2 AND "projects"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."active" = $1 AND "projects"."service_desk_enabled" = $2 AND "projects"."created_at" BETWEEN $3 AND $4 AND "projects"."creator_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT "users".* FROM "users" WHERE "users"."user_type" = $1 ORDER BY "users"."id" ASC LIMIT $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */

SELECT "users".* FROM "users" WHERE "users"."email" = $1 LIMIT $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */

SELECT MIN("issues"."id") FROM "issues" WHERE "issues"."author_id" = $1 AND "issues"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("issues"."id") FROM "issues" WHERE "issues"."author_id" = $1 AND "issues"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("issues"."id") FROM "issues" WHERE "issues"."author_id" = $1 AND "issues"."created_at" BETWEEN $2 AND $3 AND "issues"."id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("todos"."author_id") FROM "todos" WHERE "todos"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("todos"."author_id") FROM "todos" WHERE "todos"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "todos"."author_id") FROM "todos" WHERE "todos"."created_at" BETWEEN $1 AND $2 AND "todos"."author_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("deployments"."user_id") FROM "deployments" WHERE "deployments"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("deployments"."user_id") FROM "deployments" WHERE "deployments"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "deployments"."user_id") FROM "deployments" WHERE "deployments"."created_at" BETWEEN $1 AND $2 AND "deployments"."user_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("deployments"."user_id") FROM "deployments" WHERE "deployments"."status" = $1 AND "deployments"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("deployments"."user_id") FROM "deployments" WHERE "deployments"."status" = $1 AND "deployments"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "deployments"."user_id") FROM "deployments" WHERE "deployments"."status" = $1 AND "deployments"."created_at" BETWEEN $2 AND $3 AND "deployments"."user_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("releases"."author_id") FROM "releases" WHERE "releases"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("releases"."author_id") FROM "releases" WHERE "releases"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "releases"."author_id") FROM "releases" WHERE "releases"."created_at" BETWEEN $1 AND $2 AND "releases"."author_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."group_view" = $4 AND "users"."created_at" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."group_view" = $4 AND "users"."created_at" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."group_view" = $4 AND "users"."created_at" BETWEEN $5 AND $6 AND "users"."id" BETWEEN $7 AND $8 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."name" = $2 AND "ci_builds"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."name" = $2 AND "ci_builds"."created_at" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."name" = $2 AND "ci_builds"."created_at" BETWEEN $3 AND $4 AND "ci_builds"."user_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."created_at" BETWEEN $2 AND $3 AND "ci_builds"."user_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."source" = $1 AND "ci_pipelines"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."source" = $1 AND "ci_pipelines"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."source" = $1 AND "ci_pipelines"."created_at" BETWEEN $2 AND $3 AND "ci_pipelines"."user_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE ("ci_pipelines"."source" IN ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) OR "ci_pipelines"."source" IS NULL) AND "ci_pipelines"."created_at" BETWEEN $12 AND $13 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE ("ci_pipelines"."source" IN ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) OR "ci_pipelines"."source" IS NULL) AND "ci_pipelines"."created_at" BETWEEN $12 AND $13 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_pipelines"."user_id") FROM "ci_pipelines" WHERE ("ci_pipelines"."source" IN ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) OR "ci_pipelines"."source" IS NULL) AND "ci_pipelines"."created_at" BETWEEN $12 AND $13 AND "ci_pipelines"."user_id" BETWEEN $14 AND $15 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."config_source" = $1 AND "ci_pipelines"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."config_source" = $1 AND "ci_pipelines"."created_at" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."config_source" = $1 AND "ci_pipelines"."created_at" BETWEEN $2 AND $3 AND "ci_pipelines"."user_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipeline_schedules"."owner_id") FROM "ci_pipeline_schedules" WHERE "ci_pipeline_schedules"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipeline_schedules"."owner_id") FROM "ci_pipeline_schedules" WHERE "ci_pipeline_schedules"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_pipeline_schedules"."owner_id") FROM "ci_pipeline_schedules" WHERE "ci_pipeline_schedules"."created_at" BETWEEN $1 AND $2 AND "ci_pipeline_schedules"."owner_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."created_at" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."created_at" BETWEEN $1 AND $2 AND "ci_pipelines"."user_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_runners" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_runners"."cluster_id" WHERE "clusters_applications_runners"."created_at" BETWEEN $1 AND $2 AND "clusters_applications_runners"."status" IN ($3, $4) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."pipeline_events" = $3 AND "services"."active" = $4 AND "projects"."created_at" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."pipeline_events" = $3 AND "services"."active" = $4 AND "projects"."created_at" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."pipeline_events" = $3 AND "services"."active" = $4 AND "projects"."created_at" BETWEEN $5 AND $6 AND "projects"."creator_id" BETWEEN $7 AND $8 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("historical_data"."active_user_count") FROM "historical_data" WHERE "historical_data"."date" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("lists"."id") FROM "lists" WHERE "lists"."list_type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("lists"."id") FROM "lists" WHERE "lists"."list_type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("lists"."id") FROM "lists" WHERE "lists"."list_type" = $1 AND "lists"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("boards"."id") FROM "boards" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("boards"."id") FROM "boards" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("boards"."id") FROM "boards" WHERE "boards"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_builds"."id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_builds"."id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("ci_builds"."id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipelines"."id") FROM "ci_pipelines" WHERE ("ci_pipelines"."source" IN ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) OR "ci_pipelines"."source" IS NULL) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipelines"."id") FROM "ci_pipelines" WHERE ("ci_pipelines"."source" IN ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) OR "ci_pipelines"."source" IS NULL) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("ci_pipelines"."id") FROM "ci_pipelines" WHERE ("ci_pipelines"."source" IN ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) OR "ci_pipelines"."source" IS NULL) AND "ci_pipelines"."id" BETWEEN $12 AND $13 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipelines"."id") FROM "ci_pipelines" WHERE "ci_pipelines"."source" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipelines"."id") FROM "ci_pipelines" WHERE "ci_pipelines"."source" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("ci_pipelines"."id") FROM "ci_pipelines" WHERE "ci_pipelines"."source" = $1 AND "ci_pipelines"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipelines"."id") FROM "ci_pipelines" WHERE "ci_pipelines"."config_source" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipelines"."id") FROM "ci_pipelines" WHERE "ci_pipelines"."config_source" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("ci_pipelines"."id") FROM "ci_pipelines" WHERE "ci_pipelines"."config_source" = $1 AND "ci_pipelines"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_runners"."id") FROM "ci_runners" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_runners"."id") FROM "ci_runners" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("ci_runners"."id") FROM "ci_runners" WHERE "ci_runners"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipeline_schedules"."id") FROM "ci_pipeline_schedules" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipeline_schedules"."id") FROM "ci_pipeline_schedules" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("ci_pipeline_schedules"."id") FROM "ci_pipeline_schedules" WHERE "ci_pipeline_schedules"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("project_auto_devops"."id") FROM "project_auto_devops" WHERE "project_auto_devops"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("project_auto_devops"."id") FROM "project_auto_devops" WHERE "project_auto_devops"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("project_auto_devops"."id") FROM "project_auto_devops" WHERE "project_auto_devops"."enabled" = $1 AND "project_auto_devops"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("keys"."id") FROM "keys" WHERE "keys"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("keys"."id") FROM "keys" WHERE "keys"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("keys"."id") FROM "keys" WHERE "keys"."type" = $1 AND "keys"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("deployments"."id") FROM "deployments" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("deployments"."id") FROM "deployments" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("deployments"."id") FROM "deployments" WHERE "deployments"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("deployments"."id") FROM "deployments" WHERE "deployments"."status" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("deployments"."id") FROM "deployments" WHERE "deployments"."status" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("deployments"."id") FROM "deployments" WHERE "deployments"."status" = $1 AND "deployments"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("environments"."id") FROM "environments" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("environments"."id") FROM "environments" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("environments"."id") FROM "environments" WHERE "environments"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."id") FROM "clusters" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."id") FROM "clusters" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters"."id") FROM "clusters" WHERE "clusters"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."id") FROM "clusters" WHERE "clusters"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."id") FROM "clusters" WHERE "clusters"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters"."id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."cluster_type" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."cluster_type" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters"."id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."cluster_type" = $2 AND "clusters"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."id") FROM "clusters" INNER JOIN "cluster_providers_aws" ON "cluster_providers_aws"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_aws"."status" IN (3)) AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."id") FROM "clusters" INNER JOIN "cluster_providers_aws" ON "cluster_providers_aws"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_aws"."status" IN (3)) AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters"."id") FROM "clusters" INNER JOIN "cluster_providers_aws" ON "cluster_providers_aws"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_aws"."status" IN (3)) AND "clusters"."enabled" = $2 AND "clusters"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."id") FROM "clusters" INNER JOIN "cluster_providers_gcp" ON "cluster_providers_gcp"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_gcp"."status" IN (3)) AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."id") FROM "clusters" INNER JOIN "cluster_providers_gcp" ON "cluster_providers_gcp"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_gcp"."status" IN (3)) AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters"."id") FROM "clusters" INNER JOIN "cluster_providers_gcp" ON "cluster_providers_gcp"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_gcp"."status" IN (3)) AND "clusters"."enabled" = $2 AND "clusters"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."id") FROM "clusters" WHERE "clusters"."provider_type" = $1 AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."id") FROM "clusters" WHERE "clusters"."provider_type" = $1 AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters"."id") FROM "clusters" WHERE "clusters"."provider_type" = $1 AND "clusters"."enabled" = $2 AND "clusters"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters_applications_helm"."id") FROM "clusters_applications_helm" WHERE "clusters_applications_helm"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters_applications_helm"."id") FROM "clusters_applications_helm" WHERE "clusters_applications_helm"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters_applications_helm"."id") FROM "clusters_applications_helm" WHERE "clusters_applications_helm"."status" IN ($1, $2) AND "clusters_applications_helm"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters_applications_ingress"."id") FROM "clusters_applications_ingress" WHERE "clusters_applications_ingress"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters_applications_ingress"."id") FROM "clusters_applications_ingress" WHERE "clusters_applications_ingress"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters_applications_ingress"."id") FROM "clusters_applications_ingress" WHERE "clusters_applications_ingress"."status" IN ($1, $2) AND "clusters_applications_ingress"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters_applications_cert_managers"."id") FROM "clusters_applications_cert_managers" WHERE "clusters_applications_cert_managers"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters_applications_cert_managers"."id") FROM "clusters_applications_cert_managers" WHERE "clusters_applications_cert_managers"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters_applications_cert_managers"."id") FROM "clusters_applications_cert_managers" WHERE "clusters_applications_cert_managers"."status" IN ($1, $2) AND "clusters_applications_cert_managers"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters_applications_crossplane"."id") FROM "clusters_applications_crossplane" WHERE "clusters_applications_crossplane"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters_applications_crossplane"."id") FROM "clusters_applications_crossplane" WHERE "clusters_applications_crossplane"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters_applications_crossplane"."id") FROM "clusters_applications_crossplane" WHERE "clusters_applications_crossplane"."status" IN ($1, $2) AND "clusters_applications_crossplane"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters_applications_prometheus"."id") FROM "clusters_applications_prometheus" WHERE "clusters_applications_prometheus"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters_applications_prometheus"."id") FROM "clusters_applications_prometheus" WHERE "clusters_applications_prometheus"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters_applications_prometheus"."id") FROM "clusters_applications_prometheus" WHERE "clusters_applications_prometheus"."status" IN ($1, $2) AND "clusters_applications_prometheus"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters_applications_runners"."id") FROM "clusters_applications_runners" WHERE "clusters_applications_runners"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters_applications_runners"."id") FROM "clusters_applications_runners" WHERE "clusters_applications_runners"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters_applications_runners"."id") FROM "clusters_applications_runners" WHERE "clusters_applications_runners"."status" IN ($1, $2) AND "clusters_applications_runners"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters_applications_knative"."id") FROM "clusters_applications_knative" WHERE "clusters_applications_knative"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters_applications_knative"."id") FROM "clusters_applications_knative" WHERE "clusters_applications_knative"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters_applications_knative"."id") FROM "clusters_applications_knative" WHERE "clusters_applications_knative"."status" IN ($1, $2) AND "clusters_applications_knative"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters_applications_elastic_stacks"."id") FROM "clusters_applications_elastic_stacks" WHERE "clusters_applications_elastic_stacks"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters_applications_elastic_stacks"."id") FROM "clusters_applications_elastic_stacks" WHERE "clusters_applications_elastic_stacks"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters_applications_elastic_stacks"."id") FROM "clusters_applications_elastic_stacks" WHERE "clusters_applications_elastic_stacks"."status" IN ($1, $2) AND "clusters_applications_elastic_stacks"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters_applications_jupyter"."id") FROM "clusters_applications_jupyter" WHERE "clusters_applications_jupyter"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters_applications_jupyter"."id") FROM "clusters_applications_jupyter" WHERE "clusters_applications_jupyter"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("clusters_applications_jupyter"."id") FROM "clusters_applications_jupyter" WHERE "clusters_applications_jupyter"."status" IN ($1, $2) AND "clusters_applications_jupyter"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("environments"."id") FROM "environments" WHERE "environments"."environment_type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("environments"."id") FROM "environments" WHERE "environments"."environment_type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("environments"."id") FROM "environments" WHERE "environments"."environment_type" = $1 AND "environments"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("grafana_integrations"."id") FROM "grafana_integrations" WHERE "grafana_integrations"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("grafana_integrations"."id") FROM "grafana_integrations" WHERE "grafana_integrations"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("grafana_integrations"."id") FROM "grafana_integrations" WHERE "grafana_integrations"."enabled" = $1 AND "grafana_integrations"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("namespaces"."id") FROM "namespaces" WHERE "namespaces"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("namespaces"."id") FROM "namespaces" WHERE "namespaces"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("namespaces"."id") FROM "namespaces" WHERE "namespaces"."type" = $1 AND "namespaces"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("issues"."id") FROM "issues" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("issues"."id") FROM "issues" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("issues"."id") FROM "issues" WHERE "issues"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("sentry_issues"."id") FROM "sentry_issues" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("sentry_issues"."id") FROM "sentry_issues" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("sentry_issues"."id") FROM "sentry_issues" WHERE "sentry_issues"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("zoom_meetings"."id") FROM "zoom_meetings" WHERE "zoom_meetings"."issue_status" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("zoom_meetings"."id") FROM "zoom_meetings" WHERE "zoom_meetings"."issue_status" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("zoom_meetings"."id") FROM "zoom_meetings" WHERE "zoom_meetings"."issue_status" = $1 AND "zoom_meetings"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("zoom_meetings"."issue_id") FROM "zoom_meetings" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("zoom_meetings"."issue_id") FROM "zoom_meetings" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "zoom_meetings"."issue_id") FROM "zoom_meetings" WHERE "zoom_meetings"."issue_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */


SELECT MIN("issues"."id") FROM "issues" WHERE "issues"."author_id" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("issues"."id") FROM "issues" WHERE "issues"."author_id" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("issues"."id") FROM "issues" WHERE "issues"."author_id" = $1 AND "issues"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("keys"."id") FROM "keys" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("keys"."id") FROM "keys" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("keys"."id") FROM "keys" WHERE "keys"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("lfs_objects"."id") FROM "lfs_objects" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("lfs_objects"."id") FROM "lfs_objects" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("lfs_objects"."id") FROM "lfs_objects" WHERE "lfs_objects"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("milestones"."id") FROM "milestones" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("milestones"."id") FROM "milestones" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("milestones"."id") FROM "milestones" WHERE "milestones"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("pages_domains"."id") FROM "pages_domains" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("pages_domains"."id") FROM "pages_domains" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("pages_domains"."id") FROM "pages_domains" WHERE "pages_domains"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("pool_repositories"."id") FROM "pool_repositories" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("pool_repositories"."id") FROM "pool_repositories" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("pool_repositories"."id") FROM "pool_repositories" WHERE "pool_repositories"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."id") FROM "projects" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."id") FROM "projects" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("projects"."id") FROM "projects" WHERE "projects"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."id") FROM "projects" WHERE "projects"."import_type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."id") FROM "projects" WHERE "projects"."import_type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("projects"."id") FROM "projects" WHERE "projects"."import_type" = $1 AND "projects"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("project_features"."id") FROM "project_features" WHERE (repository_access_level > 0) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("project_features"."id") FROM "project_features" WHERE (repository_access_level > 0) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("project_features"."id") FROM "project_features" WHERE (repository_access_level > 0) AND "project_features"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("project_error_tracking_settings"."project_id") FROM "project_error_tracking_settings" WHERE "project_error_tracking_settings"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("project_error_tracking_settings"."project_id") FROM "project_error_tracking_settings" WHERE "project_error_tracking_settings"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("project_error_tracking_settings"."project_id") FROM "project_error_tracking_settings" WHERE "project_error_tracking_settings"."enabled" = $1 AND "project_error_tracking_settings"."project_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("services"."id") FROM "services" WHERE "services"."type" = $1 AND "services"."active" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("services"."id") FROM "services" WHERE "services"."type" = $1 AND "services"."active" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("services"."id") FROM "services" WHERE "services"."type" = $1 AND "services"."active" = $2 AND "services"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("protected_branches"."id") FROM "protected_branches" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("protected_branches"."id") FROM "protected_branches" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("protected_branches"."id") FROM "protected_branches" WHERE "protected_branches"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("releases"."id") FROM "releases" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("releases"."id") FROM "releases" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("releases"."id") FROM "releases" WHERE "releases"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("remote_mirrors"."id") FROM "remote_mirrors" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("remote_mirrors"."id") FROM "remote_mirrors" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("remote_mirrors"."id") FROM "remote_mirrors" WHERE "remote_mirrors"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("snippets"."id") FROM "snippets" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("snippets"."id") FROM "snippets" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("snippets"."id") FROM "snippets" WHERE "snippets"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("suggestions"."id") FROM "suggestions" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("suggestions"."id") FROM "suggestions" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("suggestions"."id") FROM "suggestions" WHERE "suggestions"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("todos"."id") FROM "todos" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("todos"."id") FROM "todos" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("todos"."id") FROM "todos" WHERE "todos"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("uploads"."id") FROM "uploads" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("uploads"."id") FROM "uploads" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("uploads"."id") FROM "uploads" WHERE "uploads"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("web_hooks"."id") FROM "web_hooks" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("web_hooks"."id") FROM "web_hooks" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("web_hooks"."id") FROM "web_hooks" WHERE "web_hooks"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("labels"."id") FROM "labels" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("labels"."id") FROM "labels" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("labels"."id") FROM "labels" WHERE "labels"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("merge_requests"."id") FROM "merge_requests" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("merge_requests"."id") FROM "merge_requests" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("merge_requests"."id") FROM "merge_requests" WHERE "merge_requests"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("notes"."id") FROM "notes" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("notes"."id") FROM "notes" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("notes"."id") FROM "notes" WHERE "notes"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("services"."id") FROM "services" WHERE "services"."active" = $1 AND "services"."template" = $2 AND "services"."type" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("services"."id") FROM "services" WHERE "services"."active" = $1 AND "services"."template" = $2 AND "services"."type" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("services"."id") FROM "services" WHERE "services"."active" = $1 AND "services"."template" = $2 AND "services"."type" = $3 AND "services"."id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT "services".* FROM "services" WHERE "services"."active" = $1 AND "services"."type" = $2 ORDER BY "services"."id" ASC LIMIT $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("project_feature_usages"."project_id") FROM "project_feature_usages" WHERE NOT "project_feature_usages"."jira_dvcs_cloud_last_sync_at" IS NULL /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("project_feature_usages"."project_id") FROM "project_feature_usages" WHERE NOT "project_feature_usages"."jira_dvcs_cloud_last_sync_at" IS NULL /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("project_feature_usages"."project_id") FROM "project_feature_usages" WHERE NOT "project_feature_usages"."jira_dvcs_cloud_last_sync_at" IS NULL AND "project_feature_usages"."project_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("project_feature_usages"."project_id") FROM "project_feature_usages" WHERE NOT "project_feature_usages"."jira_dvcs_server_last_sync_at" IS NULL /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("project_feature_usages"."project_id") FROM "project_feature_usages" WHERE NOT "project_feature_usages"."jira_dvcs_server_last_sync_at" IS NULL /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("project_feature_usages"."project_id") FROM "project_feature_usages" WHERE NOT "project_feature_usages"."jira_dvcs_server_last_sync_at" IS NULL AND "project_feature_usages"."project_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND (group_view = 1 OR group_view IS NULL) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND (group_view = 1 OR group_view IS NULL) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND (group_view = 1 OR group_view IS NULL) AND "users"."id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."group_view" = $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."group_view" = $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."group_view" = $4 AND "users"."id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT DISTINCT ON (deployments.environment_id) ci_pipeline_variables.* FROM "ci_pipeline_variables" INNER JOIN "ci_pipelines" ON "ci_pipelines"."id" = "ci_pipeline_variables"."pipeline_id" INNER JOIN "ci_builds" ON "ci_builds"."commit_id" = "ci_pipelines"."id" AND "ci_builds"."type" = $1 INNER JOIN "deployments" ON "deployments"."deployable_id" = "ci_builds"."id" AND "deployments"."deployable_type" = $2 INNER JOIN "environments" ON "environments"."id" = "deployments"."environment_id" INNER JOIN "deployments" AS "last_visible_deployments_environments" ON "last_visible_deployments_environments"."environment_id" = "environments"."id" AND "last_visible_deployments_environments"."status" IN ($3, $4, $5, $6) WHERE ("environments"."state" IN ('available')) AND "deployments"."status" = $7 AND "ci_pipeline_variables"."key" = $8 ORDER BY deployments.environment_id, deployments.id DESC /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT DISTINCT ON (deployments.environment_id) ci_variables.* FROM "ci_variables" INNER JOIN "projects" ON "projects"."id" = "ci_variables"."project_id" INNER JOIN "environments" ON "environments"."project_id" = "projects"."id" INNER JOIN "deployments" ON "deployments"."environment_id" = "environments"."id" AND "deployments"."status" IN ($1, $2, $3, $4) WHERE ("environments"."state" IN ('available')) AND "deployments"."status" = $5 AND "ci_variables"."key" = $6 AND NOT "environments"."id" IN (SELECT DISTINCT ON (deployments.environment_id) deployments.environment_id FROM "ci_pipeline_variables" INNER JOIN "ci_pipelines" ON "ci_pipelines"."id" = "ci_pipeline_variables"."pipeline_id" INNER JOIN "ci_builds" ON "ci_builds"."commit_id" = "ci_pipelines"."id" AND "ci_builds"."type" = $7 INNER JOIN "deployments" ON "deployments"."deployable_id" = "ci_builds"."id" AND "deployments"."deployable_type" = $8 INNER JOIN "environments" ON "environments"."id" = "deployments"."environment_id" INNER JOIN "deployments" AS "last_visible_deployments_environments" ON "last_visible_deployments_environments"."environment_id" = "environments"."id" AND "last_visible_deployments_environments"."status" IN ($9, $10, $11, $12) WHERE ("environments"."state" IN ('available')) AND "deployments"."status" = $13 AND "ci_pipeline_variables"."key" = $14 ORDER BY deployments.environment_id, deployments.id DESC) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("epics"."id") FROM "epics" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("epics"."id") FROM "epics" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("epics"."id") FROM "epics" WHERE "epics"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("operations_feature_flags"."id") FROM "operations_feature_flags" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("operations_feature_flags"."id") FROM "operations_feature_flags" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("operations_feature_flags"."id") FROM "operations_feature_flags" WHERE "operations_feature_flags"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("geo_nodes"."id") FROM "geo_nodes" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("geo_nodes"."id") FROM "geo_nodes" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("geo_nodes"."id") FROM "geo_nodes" WHERE "geo_nodes"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ldap_group_links"."id") FROM "ldap_group_links" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ldap_group_links"."id") FROM "ldap_group_links" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("ldap_group_links"."id") FROM "ldap_group_links" WHERE "ldap_group_links"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN(users.id) FROM "users" INNER JOIN "identities" ON "identities"."user_id" = "users"."id" WHERE (identities.provider LIKE 'ldap%') /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX(users.id) FROM "users" INNER JOIN "identities" ON "identities"."user_id" = "users"."id" WHERE (identities.provider LIKE 'ldap%') /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(users.id) FROM "users" INNER JOIN "identities" ON "identities"."user_id" = "users"."id" WHERE (identities.provider LIKE 'ldap%') AND "users"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "projects"."pending_delete" = $1 AND "projects"."archived" = $2 AND "protected_branches"."code_owner_approval_required" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "projects"."pending_delete" = $1 AND "projects"."archived" = $2 AND "protected_branches"."code_owner_approval_required" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("projects"."id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "projects"."pending_delete" = $1 AND "projects"."archived" = $2 AND "protected_branches"."code_owner_approval_required" = $3 AND "projects"."id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND "approval_merge_request_rules"."approvals_required" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND "approval_merge_request_rules"."approvals_required" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND "approval_merge_request_rules"."approvals_required" = $3 AND "approval_merge_request_rules"."merge_request_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND (approvals_required > 0) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND (approvals_required > 0) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "approval_merge_request_rules"."merge_request_id") FROM "approval_merge_request_rules" WHERE ("approval_merge_request_rules"."code_owner" = $1 OR "approval_merge_request_rules"."rule_type" = $2) AND (approvals_required > 0) AND "approval_merge_request_rules"."merge_request_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("services"."id") FROM "services" WHERE "services"."type" = $1 AND "services"."default" = $2 AND "services"."active" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("services"."id") FROM "services" WHERE "services"."type" = $1 AND "services"."default" = $2 AND "services"."active" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("services"."id") FROM "services" WHERE "services"."type" = $1 AND "services"."default" = $2 AND "services"."active" = $3 AND "services"."id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("packages_packages"."project_id") FROM "packages_packages" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("packages_packages"."project_id") FROM "packages_packages" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "packages_packages"."project_id") FROM "packages_packages" WHERE "packages_packages"."project_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("prometheus_alerts"."project_id") FROM "prometheus_alerts" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("prometheus_alerts"."project_id") FROM "prometheus_alerts" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "prometheus_alerts"."project_id") FROM "prometheus_alerts" WHERE "prometheus_alerts"."project_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("project_tracing_settings"."id") FROM "project_tracing_settings" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("project_tracing_settings"."id") FROM "project_tracing_settings" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("project_tracing_settings"."id") FROM "project_tracing_settings" WHERE "project_tracing_settings"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."id") FROM "projects" WHERE "projects"."namespace_id" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."id") FROM "projects" WHERE "projects"."namespace_id" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("projects"."id") FROM "projects" WHERE "projects"."namespace_id" = $1 AND "projects"."id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."id") FROM "projects" INNER JOIN namespaces ON projects.namespace_id = namespaces.custom_project_templates_group_id /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."id") FROM "projects" INNER JOIN namespaces ON projects.namespace_id = namespaces.custom_project_templates_group_id /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("projects"."id") FROM "projects" INNER JOIN namespaces ON projects.namespace_id = namespaces.custom_project_templates_group_id WHERE "projects"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_builds"."id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."name" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_builds"."id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."name" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("ci_builds"."id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."name" = $2 AND "ci_builds"."id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."dashboard" = $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."dashboard" = $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("users"."id") FROM "users" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users"."dashboard" = $4 AND "users"."id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("users_ops_dashboard_projects"."user_id") FROM "users_ops_dashboard_projects" INNER JOIN "users" ON "users"."id" = "users_ops_dashboard_projects"."user_id" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("users_ops_dashboard_projects"."user_id") FROM "users_ops_dashboard_projects" INNER JOIN "users" ON "users"."id" = "users_ops_dashboard_projects"."user_id" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "users_ops_dashboard_projects"."user_id") FROM "users_ops_dashboard_projects" INNER JOIN "users" ON "users"."id" = "users_ops_dashboard_projects"."user_id" WHERE ("users"."state" IN ('active')) AND (NOT ghost IS TRUE) AND ("users"."user_type" IS NULL OR NOT "users"."user_type" IN ($1, $2, $3)) AND "users_ops_dashboard_projects"."user_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT "features"."key" FROM "features" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT "projects".* FROM "projects" WHERE NOT "projects"."last_activity_at" IS NULL ORDER BY "projects"."last_activity_at" DESC LIMIT $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT "projects".* FROM "projects" WHERE NOT "projects"."last_repository_updated_at" IS NULL ORDER BY "projects"."last_repository_updated_at" DESC LIMIT $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT "projects".* FROM "projects" WHERE "projects"."last_activity_at" IS NULL AND "projects"."last_repository_updated_at" IS NULL LIMIT $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_cert_managers" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_cert_managers"."cluster_id" WHERE "clusters_applications_cert_managers"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_helm" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_helm"."cluster_id" WHERE "clusters_applications_helm"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_ingress" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_ingress"."cluster_id" WHERE "clusters_applications_ingress"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_knative" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_knative"."cluster_id" WHERE "clusters_applications_knative"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."user_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_gcp" ON "cluster_providers_gcp"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_gcp"."status" IN (3)) AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_gcp" ON "cluster_providers_gcp"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_gcp"."status" IN (3)) AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_gcp" ON "cluster_providers_gcp"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_gcp"."status" IN (3)) AND "clusters"."enabled" = $2 AND "clusters"."user_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_aws" ON "cluster_providers_aws"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_aws"."status" IN (3)) AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_aws" ON "cluster_providers_aws"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_aws"."status" IN (3)) AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" INNER JOIN "cluster_providers_aws" ON "cluster_providers_aws"."cluster_id" = "clusters"."id" WHERE "clusters"."provider_type" = $1 AND ("cluster_providers_aws"."status" IN (3)) AND "clusters"."enabled" = $2 AND "clusters"."user_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" WHERE "clusters"."provider_type" = $1 AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" WHERE "clusters"."provider_type" = $1 AND "clusters"."enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" WHERE "clusters"."provider_type" = $1 AND "clusters"."enabled" = $2 AND "clusters"."user_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."cluster_type" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."cluster_type" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" WHERE "clusters"."enabled" = $1 AND "clusters"."cluster_type" = $2 AND "clusters"."user_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "projects"."creator_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("keys"."user_id") FROM "keys" WHERE "keys"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("keys"."user_id") FROM "keys" WHERE "keys"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "keys"."user_id") FROM "keys" WHERE "keys"."type" = $1 AND "keys"."user_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("keys"."user_id") FROM "keys" WHERE ("keys"."type" IN ($1, $2) OR "keys"."type" IS NULL) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("keys"."user_id") FROM "keys" WHERE ("keys"."type" IN ($1, $2) OR "keys"."type" IS NULL) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "keys"."user_id") FROM "keys" WHERE ("keys"."type" IN ($1, $2) OR "keys"."type" IS NULL) AND "keys"."user_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("merge_requests"."author_id") FROM "merge_requests" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("merge_requests"."author_id") FROM "merge_requests" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "merge_requests"."author_id") FROM "merge_requests" WHERE "merge_requests"."author_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "protected_branches"."code_owner_approval_required" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "protected_branches"."code_owner_approval_required" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "protected_branches"."code_owner_approval_required" = $1 AND "projects"."creator_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" WHERE "projects"."import_type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" WHERE "projects"."import_type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" WHERE "projects"."import_type" = $1 AND "projects"."creator_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "project_features" ON "project_features"."project_id" = "projects"."id" WHERE "project_features"."repository_access_level" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "project_features" ON "project_features"."project_id" = "projects"."id" WHERE "project_features"."repository_access_level" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "project_features" ON "project_features"."project_id" = "projects"."id" WHERE "project_features"."repository_access_level" = $1 AND "projects"."creator_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "protected_branches" ON "protected_branches"."project_id" = "projects"."id" WHERE "projects"."creator_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "remote_mirrors" ON "remote_mirrors"."project_id" = "projects"."id" WHERE "remote_mirrors"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "remote_mirrors" ON "remote_mirrors"."project_id" = "projects"."id" WHERE "remote_mirrors"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "remote_mirrors" ON "remote_mirrors"."project_id" = "projects"."id" WHERE "remote_mirrors"."enabled" = $1 AND "projects"."creator_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("snippets"."author_id") FROM "snippets" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("snippets"."author_id") FROM "snippets" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "snippets"."author_id") FROM "snippets" WHERE "snippets"."author_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("notes"."author_id") FROM "notes" INNER JOIN "suggestions" ON "suggestions"."note_id" = "notes"."id" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("notes"."author_id") FROM "notes" INNER JOIN "suggestions" ON "suggestions"."note_id" = "notes"."id" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "notes"."author_id") FROM "notes" INNER JOIN "suggestions" ON "suggestions"."note_id" = "notes"."id" WHERE "notes"."author_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("events"."author_id") FROM "events" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("events"."author_id") FROM "events" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "events"."author_id") FROM "events" WHERE "events"."author_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 AND "members"."user_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 AND "members"."ldap" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 AND "members"."ldap" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "members"."user_id") FROM "members" WHERE "members"."type" = $1 AND "members"."source_type" = $2 AND "members"."ldap" = $3 AND "members"."user_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("users"."id") FROM "users" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("users"."id") FROM "users" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT("users"."id") FROM "users" WHERE "users"."id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("clusters"."user_id") FROM "clusters" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("clusters"."user_id") FROM "clusters" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "clusters"."user_id") FROM "clusters" WHERE "clusters"."user_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_prometheus" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_prometheus"."cluster_id" WHERE "clusters_applications_prometheus"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."active" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."active" = $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."active" = $3 AND "projects"."creator_id" BETWEEN $4 AND $5 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "project_error_tracking_settings" ON "project_error_tracking_settings"."project_id" = "projects"."id" WHERE "project_error_tracking_settings"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "project_error_tracking_settings" ON "project_error_tracking_settings"."project_id" = "projects"."id" WHERE "project_error_tracking_settings"."enabled" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "project_error_tracking_settings" ON "project_error_tracking_settings"."project_id" = "projects"."id" WHERE "project_error_tracking_settings"."enabled" = $1 AND "projects"."creator_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "project_tracing_settings" ON "project_tracing_settings"."project_id" = "projects"."id" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "project_tracing_settings" ON "project_tracing_settings"."project_id" = "projects"."id" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "project_tracing_settings" ON "project_tracing_settings"."project_id" = "projects"."id" WHERE "projects"."creator_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "packages_packages" ON "packages_packages"."project_id" = "projects"."id" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "packages_packages" ON "packages_packages"."project_id" = "projects"."id" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "packages_packages" ON "packages_packages"."project_id" = "projects"."id" WHERE "projects"."creator_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("lists"."user_id") FROM "lists" WHERE "lists"."list_type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("lists"."user_id") FROM "lists" WHERE "lists"."list_type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "lists"."user_id") FROM "lists" WHERE "lists"."list_type" = $1 AND "lists"."user_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("epics"."author_id") FROM "epics" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("epics"."author_id") FROM "epics" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "epics"."author_id") FROM "epics" WHERE "epics"."author_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("issues"."author_id") FROM "issues" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("issues"."author_id") FROM "issues" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "issues"."author_id") FROM "issues" WHERE "issues"."author_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("notes"."author_id") FROM "notes" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("notes"."author_id") FROM "notes" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "notes"."author_id") FROM "notes" WHERE "notes"."author_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" WHERE "projects"."creator_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND "projects"."creator_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_cloud_last_sync_at" IS NULL /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_cloud_last_sync_at" IS NULL /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_cloud_last_sync_at" IS NULL AND "projects"."creator_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_server_last_sync_at" IS NULL /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_server_last_sync_at" IS NULL /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" INNER JOIN "project_feature_usages" ON "project_feature_usages"."project_id" = "projects"."id" WHERE "services"."type" = $1 AND "services"."active" = $2 AND NOT "project_feature_usages"."jira_dvcs_server_last_sync_at" IS NULL AND "projects"."creator_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."active" = $1 AND "projects"."service_desk_enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."active" = $1 AND "projects"."service_desk_enabled" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" WHERE "services"."active" = $1 AND "projects"."service_desk_enabled" = $2 AND "projects"."creator_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("todos"."author_id") FROM "todos" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("todos"."author_id") FROM "todos" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "todos"."author_id") FROM "todos" WHERE "todos"."author_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("deployments"."user_id") FROM "deployments" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("deployments"."user_id") FROM "deployments" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "deployments"."user_id") FROM "deployments" WHERE "deployments"."user_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("deployments"."user_id") FROM "deployments" WHERE "deployments"."status" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("deployments"."user_id") FROM "deployments" WHERE "deployments"."status" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "deployments"."user_id") FROM "deployments" WHERE "deployments"."status" = $1 AND "deployments"."user_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("releases"."author_id") FROM "releases" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("releases"."author_id") FROM "releases" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "releases"."author_id") FROM "releases" WHERE "releases"."author_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."name" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."name" = $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."name" = $2 AND "ci_builds"."user_id" BETWEEN $3 AND $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_builds"."user_id") FROM "ci_builds" WHERE "ci_builds"."type" = $1 AND "ci_builds"."user_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."source" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."source" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."source" = $1 AND "ci_pipelines"."user_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE ("ci_pipelines"."source" IN ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) OR "ci_pipelines"."source" IS NULL) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE ("ci_pipelines"."source" IN ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) OR "ci_pipelines"."source" IS NULL) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_pipelines"."user_id") FROM "ci_pipelines" WHERE ("ci_pipelines"."source" IN ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11) OR "ci_pipelines"."source" IS NULL) AND "ci_pipelines"."user_id" BETWEEN $12 AND $13 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."config_source" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."config_source" = $1 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."config_source" = $1 AND "ci_pipelines"."user_id" BETWEEN $2 AND $3 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipeline_schedules"."owner_id") FROM "ci_pipeline_schedules" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipeline_schedules"."owner_id") FROM "ci_pipeline_schedules" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_pipeline_schedules"."owner_id") FROM "ci_pipeline_schedules" WHERE "ci_pipeline_schedules"."owner_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("ci_pipelines"."user_id") FROM "ci_pipelines" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("ci_pipelines"."user_id") FROM "ci_pipelines" /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "ci_pipelines"."user_id") FROM "ci_pipelines" WHERE "ci_pipelines"."user_id" BETWEEN $1 AND $2 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT user_id) FROM "clusters_applications_runners" INNER JOIN "clusters" ON "clusters"."id" = "clusters_applications_runners"."cluster_id" WHERE "clusters_applications_runners"."status" IN ($1, $2) /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MIN("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."pipeline_events" = $3 AND "services"."active" = $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT MAX("projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."pipeline_events" = $3 AND "services"."active" = $4 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
SELECT COUNT(DISTINCT "projects"."creator_id") FROM "projects" INNER JOIN "services" ON "services"."project_id" = "projects"."id" AND "services"."type" = $1 WHERE "services"."type" = $2 AND "services"."pipeline_events" = $3 AND "services"."active" = $4 AND "projects"."creator_id" BETWEEN $5 AND $6 /* application:test,controller:application_settings,action:usage_data,correlation_id:6a0c0fa1ca2505711e59296537dcdff3 */
































































SELECT "application_settings".* FROM "application_settings" WHERE "application_settings"."id" = $1 LIMIT $2 FOR UPDATE



SELECT "oauth_applications".* FROM "oauth_applications" WHERE "oauth_applications"."owner_id" IS NULL /* application:test,controller:applications,action:index,correlation_id:b9055a1a2d73b0b126217607f275b2ad */
SELECT COUNT(DISTINCT "oauth_access_tokens"."resource_owner_id") AS count_resource_owner_id, "oauth_access_tokens"."application_id" AS oauth_access_tokens_application_id FROM "oauth_access_tokens" WHERE 1 = 0 GROUP BY "oauth_access_tokens"."application_id" /* application:test,controller:applications,action:index,correlation_id:b9055a1a2d73b0b126217607f275b2ad */
SELECT COUNT(*) FROM "projects" INNER JOIN "namespaces" ON "projects"."namespace_id" = "namespaces"."id" WHERE "namespaces"."owner_id" = $1 AND "namespaces"."type" IS NULL /* application:test,controller:applications,action:index,correlation_id:b9055a1a2d73b0b126217607f275b2ad */
SELECT "users".* FROM "users" WHERE "users"."id" = $1 /* application:test,controller:applications,action:index,correlation_id:b9055a1a2d73b0b126217607f275b2ad */
SELECT COUNT(*) FROM "issues" INNER JOIN "projects" ON "projects"."id" = "issues"."project_id" LEFT JOIN project_features ON projects.id = project_features.project_id WHERE (EXISTS(SELECT 1 FROM "project_authorizations" WHERE "project_authorizations"."user_id" = 51 AND (project_authorizations.project_id = projects.id)) OR projects.visibility_level IN (0, 10, 20)) AND ("project_features"."issues_access_level" > 0 OR "project_features"."issues_access_level" IS NULL) AND ("issues"."state_id" IN (1)) AND (EXISTS(SELECT TRUE FROM "issue_assignees" WHERE "issue_assignees"."user_id" IN (51) AND issue_id = issues.id)) AND "projects"."archived" = $1 /* application:test,controller:applications,action:index,correlation_id:b9055a1a2d73b0b126217607f275b2ad */
SELECT COUNT(*) FROM "merge_requests" INNER JOIN "projects" ON "projects"."id" = "merge_requests"."target_project_id" LEFT JOIN project_features ON projects.id = project_features.project_id WHERE (EXISTS(SELECT 1 FROM "project_authorizations" WHERE "project_authorizations"."user_id" = 51 AND (project_authorizations.project_id = projects.id)) OR projects.visibility_level IN (0, 10, 20)) AND ("project_features"."merge_requests_access_level" > 0 OR "project_features"."merge_requests_access_level" IS NULL) AND ("merge_requests"."state_id" IN (1)) AND (EXISTS(SELECT TRUE FROM "merge_request_assignees" WHERE "merge_request_assignees"."user_id" IN (51) AND merge_request_id = merge_requests.id)) AND "projects"."archived" = $1 /* application:test,controller:applications,action:index,correlation_id:b9055a1a2d73b0b126217607f275b2ad */
SELECT COUNT(*) FROM "todos" WHERE "todos"."user_id" = $1 AND ("todos"."state" IN ('pending')) /* application:test,controller:applications,action:index,correlation_id:b9055a1a2d73b0b126217607f275b2ad */
SELECT "user_statuses".* FROM "user_statuses" WHERE "user_statuses"."user_id" = $1 LIMIT $2 /* application:test,controller:applications,action:index,correlation_id:b9055a1a2d73b0b126217607f275b2ad */
SELECT "broadcast_messages".* FROM "broadcast_messages" WHERE (ends_at > '2020-03-26 10:03:16.207506') AND "broadcast_messages"."broadcast_type" = $1 ORDER BY "broadcast_messages"."id" ASC /* application:test,controller:applications,action:index,correlation_id:b9055a1a2d73b0b126217607f275b2ad */





SELECT "oauth_applications".* FROM "oauth_applications" WHERE "oauth_applications"."id" = $1 LIMIT $2 /* application:test,controller:applications,action:edit,correlation_id:53194acc340cdd8b2097f675ca508996 */


SELECT COUNT(*) FROM "oauth_applications" /* application:test,correlation_id:1c10894af5d4e46be790985e298358d4 */

SELECT "oauth_applications".* FROM "oauth_applications" ORDER BY "oauth_applications"."id" DESC LIMIT $1 /* application:test,correlation_id:1c10894af5d4e46be790985e298358d4 */













SELECT COUNT(*) FROM "clusters_applications_helm" /* application:test,correlation_id:92f7cd2156668f0a0ca5d61fbf35733e */












SELECT "clusters".* FROM "clusters" WHERE "clusters"."cluster_type" = $1 AND "clusters"."id" = $2 LIMIT $3 /* application:test,controller:applications,action:create,correlation_id:92f7cd2156668f0a0ca5d61fbf35733e */
SELECT "clusters_applications_helm".* FROM "clusters_applications_helm" WHERE "clusters_applications_helm"."cluster_id" = $1 LIMIT $2 /* application:test,controller:applications,action:create,correlation_id:92f7cd2156668f0a0ca5d61fbf35733e */
SELECT "cluster_platforms_kubernetes".* FROM "cluster_platforms_kubernetes" WHERE "cluster_platforms_kubernetes"."cluster_id" = $1 LIMIT $2 /* application:test,controller:applications,action:create,correlation_id:92f7cd2156668f0a0ca5d61fbf35733e */































SELECT "users".* FROM "users" ORDER BY "users"."id" ASC LIMIT $1
