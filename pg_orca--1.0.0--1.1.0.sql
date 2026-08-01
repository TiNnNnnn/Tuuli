/* Install or refresh the xform controls when upgrading either 1.0.0 variant. */
\echo Use "ALTER EXTENSION pg_orca UPDATE TO '1.1.0'" to load this file. \quit

CREATE OR REPLACE FUNCTION disable_xform(text)
	RETURNS text
	AS 'MODULE_PATHNAME', 'DisableXform'
	LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION enable_xform(text)
	RETURNS text
	AS 'MODULE_PATHNAME', 'EnableXform'
	LANGUAGE C STRICT;
