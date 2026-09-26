CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE trace_zero_input(value int);
INSERT INTO trace_zero_input VALUES (42);
ANALYZE trace_zero_input;
