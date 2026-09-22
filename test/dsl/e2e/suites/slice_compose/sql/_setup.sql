CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE slice_input(v bigint NOT NULL);
-- Equal rows make unordered slice membership independent of scan order.
INSERT INTO slice_input SELECT 7 FROM generate_series(1,10);
ANALYZE slice_input;
