-- The projected Boolean is consumed by IN, so it cannot be discarded as
-- an unused EXISTS output. The inner filter references the outer part.
SELECT p.p_partkey
FROM part AS p
WHERE TRUE IN (
  SELECT ps.ps_availqty > :minimum_quantity
  FROM partsupp AS ps
  WHERE ps.ps_partkey = p.p_partkey
)
ORDER BY p.p_partkey;
