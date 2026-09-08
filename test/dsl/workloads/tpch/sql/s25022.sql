WITH SupplierPartDetails AS (
  SELECT
    s.s_name AS supplier_name,
    p.p_name AS part_name,
    ps.ps_availqty AS available_quantity,
    ps.ps_supplycost AS supply_cost
  FROM supplier AS s
  JOIN partsupp AS ps
    ON s.s_suppkey = ps.ps_suppkey
  JOIN part AS p
    ON ps.ps_partkey = p.p_partkey
), AggregatedDetails AS (
  SELECT
    supplier_name,
    COUNT(part_name) AS total_parts,
    SUM(available_quantity) AS total_available_qty,
    SUM(supply_cost * available_quantity) AS total_supply_cost
  FROM SupplierPartDetails
  GROUP BY
    supplier_name
), RankedSuppliers AS (
  SELECT
    supplier_name,
    total_parts,
    total_available_qty,
    total_supply_cost,
    RANK() OVER (ORDER BY total_supply_cost DESC) AS supplier_rank
  FROM AggregatedDetails
)
SELECT
  supplier_name,
  total_parts,
  total_available_qty,
  total_supply_cost,
  CONCAT(
    'Supplier: ',
    supplier_name,
    ', Total Parts: ',
    CAST(total_parts AS VARCHAR),
    ', Total Available Quantity: ',
    CAST(total_available_qty AS VARCHAR),
    ', Total Supply Cost: ',
    CAST(total_supply_cost AS DECIMAL(10, 2))
  ) AS supplier_summary
FROM RankedSuppliers
WHERE
  supplier_rank <= 10
ORDER BY
  supplier_rank;
