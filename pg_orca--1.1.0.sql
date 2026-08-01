/* pg_orca extension SQL.
 *
 * Loads the shared library so _PG_init runs and registers the
 * planner_hook + pg_orca.* GUCs in the current session.
 *
 * To make every new connection to a database auto-load pg_orca
 * (no per-session LOAD, no server restart), run AFTER CREATE EXTENSION
 * as a top-level command:
 *
 *     ALTER DATABASE mydb SET session_preload_libraries = 'pg_orca';
 *
 * Alternative scopes:
 *     ALTER SYSTEM SET session_preload_libraries = 'pg_orca';   -- cluster-wide
 *     SELECT pg_reload_conf();
 *
 *     ALTER ROLE bench SET session_preload_libraries = 'pg_orca'; -- single role
 *
 * Roll back:
 *     ALTER DATABASE mydb RESET session_preload_libraries;
 *     DROP EXTENSION pg_orca;
 */
LOAD 'MODULE_PATHNAME';

/* Per-xform enable/disable, by xform name (e.g. 'CXformCollapseProject').
 *
 * These flip optimizer_xforms[] via COptTasks::SetXform, which the shared
 * PxfsCandidates-intersect-enabled gate honours for BOTH native xforms and
 * the DSL rule shells (CXformDSLRule_*). Disabling a native logical xform and
 * keeping the corresponding DSL shell enabled is exactly the "replace ORCA's
 * logical rewrite with a MONSOON-proven DSL rule" path.
 *
 *     SELECT disable_xform('CXformCollapseProject');
 *     SELECT enable_xform('CXformCollapseProject');
 *
 * NOTE: pre-memo CExpressionPreprocessor steps are outside SetXform's reach;
 * use the pg_orca.enable_dsl_rule GUC to skip the operator-collapsing
 * preprocess steps so the DSL matcher sees the un-collapsed logical tree.
 */
CREATE FUNCTION disable_xform(text)
	RETURNS text
	AS 'MODULE_PATHNAME', 'DisableXform'
	LANGUAGE C STRICT;

CREATE FUNCTION enable_xform(text)
	RETURNS text
	AS 'MODULE_PATHNAME', 'EnableXform'
	LANGUAGE C STRICT;
