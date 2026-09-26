CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE quant_outer(i int, v int);
CREATE TABLE quant_inner(v int, a bool, b bool);
INSERT INTO quant_outer VALUES (1, NULL), (2, 1), (3, 2), (4, 3);
INSERT INTO quant_inner VALUES (1, true, false), (2, false, true),
  (2, true, NULL), (NULL, NULL, true), (3, false, false);
ANALYZE quant_outer;
ANALYZE quant_inner;
CREATE TABLE quant_empty (LIKE quant_inner);
CREATE TABLE quant_error (LIKE quant_inner);
INSERT INTO quant_error VALUES (0, false, false);
ANALYZE quant_empty;
ANALYZE quant_error;
