WITH disabled AS MATERIALIZED (
  SELECT set_config('pg_orca.enable_dphyper', 'off', false)
), shadow AS MATERIALIZED (
  SELECT set_config('pg_orca.dphyper_shadow', 'on', false) FROM disabled
)
SELECT set_config('pg_orca.enable_dphyper', 'on', false) FROM shadow;
