WITH SupplierParts AS (
  SELECT
    s.s_name AS supplier_name,
    p.p_name AS part_name,
    p.p_type AS part_type,
    ps.ps_availqty AS available_quantity,
    ps.ps_supplycost AS supply_cost,
    CONCAT(s.s_name, ' supplies ', p.p_name, ' of type ', p.p_type) AS supplier_part_description
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
), HighValueSuppliers AS (
  SELECT
    supplier_name,
    COUNT(part_name) AS part_count,
    SUM(supply_cost * available_quantity) AS total_value,
    STRING_AGG(supplier_part_description, '; ') AS descriptions
  FROM SupplierParts
  GROUP BY
    supplier_name
  HAVING
    SUM(supply_cost * available_quantity) > 1000000
)
SELECT
  ROW_NUMBER() OVER (ORDER BY total_value DESC) AS rank,
  supplier_name,
  part_count,
  total_value,
  descriptions
FROM HighValueSuppliers
ORDER BY
  total_value DESC;
