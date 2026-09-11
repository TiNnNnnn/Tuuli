-- Nonconstant outer Boolean retains PostgreSQL's OpExpr comparison and uses
-- the existing translator. This is a distinct declared SQL input, not a claim
-- of equivalence to TRUE IN for arbitrary data (p_size may be zero or NULL).
SELECT p.p_partkey
FROM part AS p
WHERE (p.p_size > 0) IN (
  SELECT ps.ps_availqty > :minimum_quantity
  FROM partsupp AS ps
  WHERE ps.ps_partkey = p.p_partkey
)
ORDER BY p.p_partkey;
