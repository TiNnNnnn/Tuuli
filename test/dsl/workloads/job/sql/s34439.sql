WITH RECURSIVE movie_hierarchy AS (
  SELECT
    mt.id AS movie_id,
    mt.title,
    mt.production_year,
    1 AS level
  FROM aka_title AS mt
  WHERE
    mt.production_year >= 2000
  UNION ALL
  SELECT
    ml.linked_movie_id AS movie_id,
    at.title,
    at.production_year,
    mh.level + 1
  FROM movie_link AS ml
  INNER JOIN aka_title AS at
    ON ml.linked_movie_id = at.id
  INNER JOIN movie_hierarchy AS mh
    ON ml.movie_id = mh.movie_id
  WHERE
    mh.level < 3
), actor_rankings AS (
  SELECT
    ci.movie_id,
    ak.name AS actor_name,
    ROW_NUMBER() OVER (PARTITION BY ci.movie_id ORDER BY COUNT(*) DESC) AS role_rank
  FROM cast_info AS ci
  JOIN aka_name AS ak
    ON ci.person_id = ak.person_id
  GROUP BY
    ci.movie_id,
    ak.name
), company_movie_info AS (
  SELECT
    mc.movie_id,
    STRING_AGG(CASE WHEN NOT ct.kind IS NULL THEN cn.name ELSE 'Unknown Company' END, ', ') AS companies_involved
  FROM movie_companies AS mc
  LEFT JOIN company_name AS cn
    ON mc.company_id = cn.id
  LEFT JOIN company_type AS ct
    ON mc.company_type_id = ct.id
  GROUP BY
    mc.movie_id
)
SELECT
  mh.movie_id,
  mh.title,
  mh.production_year,
  COALESCE(ar.actor_name, 'No Cast') AS lead_actor,
  cm.companies_involved,
  COUNT(DISTINCT ki.keyword) AS keyword_count,
  (
    SELECT
      COUNT(*)
    FROM complete_cast AS cc
    WHERE
      cc.movie_id = mh.movie_id
  ) AS total_cast_count
FROM movie_hierarchy AS mh
LEFT JOIN actor_rankings AS ar
  ON mh.movie_id = ar.movie_id AND ar.role_rank = 1
LEFT JOIN company_movie_info AS cm
  ON mh.movie_id = cm.movie_id
LEFT JOIN movie_keyword AS mk
  ON mh.movie_id = mk.movie_id
LEFT JOIN keyword AS ki
  ON mk.keyword_id = ki.id
GROUP BY
  mh.movie_id,
  mh.title,
  mh.production_year,
  ar.actor_name,
  cm.companies_involved
HAVING
  COUNT(DISTINCT ki.keyword) > 3
ORDER BY
  mh.production_year DESC,
  mh.title ASC
LIMIT 100;
