WITH RECURSIVE MovieCTE AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year
  FROM aka_title AS m
  WHERE
    m.kind_id = 1
  UNION ALL
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year
  FROM aka_title AS m
  JOIN MovieCTE AS c
    ON c.movie_id = m.episode_of_id
), CastDetails AS (
  SELECT
    c.movie_id,
    a.name AS actor_name,
    rt.role AS role_name,
    ROW_NUMBER() OVER (PARTITION BY c.movie_id ORDER BY c.nr_order) AS actor_order
  FROM cast_info AS c
  JOIN aka_name AS a
    ON c.person_id = a.person_id
  JOIN role_type AS rt
    ON c.role_id = rt.id
), MovieCompanies AS (
  SELECT
    mc.movie_id,
    STRING_AGG(DISTINCT cn.name, ', ') AS company_names,
    ct.kind AS company_type
  FROM movie_companies AS mc
  JOIN company_name AS cn
    ON mc.company_id = cn.id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
  GROUP BY
    mc.movie_id,
    ct.kind
)
SELECT
  m.movie_id,
  m.title,
  m.production_year,
  cd.actor_name,
  cd.role_name,
  mc.company_names,
  mc.company_type
FROM MovieCTE AS m
LEFT JOIN CastDetails AS cd
  ON m.movie_id = cd.movie_id
LEFT JOIN MovieCompanies AS mc
  ON m.movie_id = mc.movie_id
WHERE
  NOT m.production_year IS NULL
  AND NOT cd.actor_name IS NULL
  AND (
    cd.role_name LIKE '%Director%' OR cd.role_name LIKE '%Producer%'
  )
ORDER BY
  m.production_year DESC,
  cd.actor_order;
