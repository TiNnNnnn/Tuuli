WITH RECURSIVE movie_hierarchy AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    m.kind_id,
    CAST(NULL AS INT) AS parent_id
  FROM aka_title AS m
  WHERE
    NOT m.production_year IS NULL
  UNION ALL
  SELECT
    m.id,
    m.title,
    m.production_year,
    m.kind_id,
    mh.movie_id
  FROM aka_title AS m
  JOIN movie_hierarchy AS mh
    ON m.episode_of_id = mh.movie_id
), cast_with_ranks AS (
  SELECT
    ci.movie_id,
    ci.person_id,
    ROW_NUMBER() OVER (PARTITION BY ci.movie_id ORDER BY ci.nr_order) AS actor_rank,
    a.name AS actor_name
  FROM cast_info AS ci
  JOIN aka_name AS a
    ON ci.person_id = a.person_id
), movies_with_keywords AS (
  SELECT
    m.id AS movie_id,
    m.title,
    k.keyword
  FROM aka_title AS m
  LEFT JOIN movie_keyword AS mk
    ON m.id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  WHERE
    NOT k.keyword IS NULL
), company_info AS (
  SELECT
    mc.movie_id,
    STRING_AGG(c.name, ', ') AS companies,
    STRING_AGG(ct.kind, ', ') AS company_types
  FROM movie_companies AS mc
  JOIN company_name AS c
    ON mc.company_id = c.id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
  GROUP BY
    mc.movie_id
)
SELECT
  mh.movie_id,
  mh.title,
  mh.production_year,
  STRING_AGG(DISTINCT kw.keyword, ', ') AS keywords,
  ci.companies,
  ci.company_types,
  MAX(cwr.actor_rank) AS max_actors,
  CASE WHEN MAX(cwr.actor_rank) > 0 THEN 'Active Cast' ELSE 'No Cast' END AS cast_status
FROM movie_hierarchy AS mh
LEFT JOIN movies_with_keywords AS kw
  ON mh.movie_id = kw.movie_id
LEFT JOIN company_info AS ci
  ON mh.movie_id = ci.movie_id
LEFT JOIN cast_with_ranks AS cwr
  ON mh.movie_id = cwr.movie_id
GROUP BY
  mh.movie_id,
  mh.title,
  mh.production_year,
  ci.companies,
  ci.company_types
HAVING
  COUNT(kw.keyword) > 5 OR MAX(cwr.actor_rank) IS NULL
ORDER BY
  mh.production_year DESC,
  mh.title ASC;
