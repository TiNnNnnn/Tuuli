WITH RECURSIVE MovieHierarchy AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    1 AS level,
    CAST(m.title AS VARCHAR(255)) AS path
  FROM aka_title AS m
  WHERE
    m.episode_of_id IS NULL
  UNION ALL
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    mh.level + 1 AS level,
    CAST(mh.path || ' -> ' || m.title AS VARCHAR(255)) AS path
  FROM aka_title AS m
  JOIN MovieHierarchy AS mh
    ON m.episode_of_id = mh.movie_id
), KeywordCounts AS (
  SELECT
    mt.movie_id,
    COUNT(mk.keyword_id) AS keyword_count
  FROM movie_keyword AS mk
  JOIN aka_title AS mt
    ON mk.movie_id = mt.id
  WHERE
    mt.production_year >= 2000
  GROUP BY
    mt.movie_id
), CastSummaries AS (
  SELECT
    ci.movie_id,
    COUNT(DISTINCT ci.person_id) AS actor_count,
    STRING_AGG(DISTINCT ak.name, ', ') AS actors
  FROM cast_info AS ci
  LEFT JOIN aka_name AS ak
    ON ci.person_id = ak.person_id
  GROUP BY
    ci.movie_id
)
SELECT
  mh.movie_id,
  mh.title,
  mh.production_year,
  mh.level,
  mh.path,
  COALESCE(kc.keyword_count, 0) AS keyword_count,
  COALESCE(cs.actor_count, 0) AS actor_count,
  COALESCE(cs.actors, 'No actors') AS actors
FROM MovieHierarchy AS mh
LEFT JOIN KeywordCounts AS kc
  ON mh.movie_id = kc.movie_id
LEFT JOIN CastSummaries AS cs
  ON mh.movie_id = cs.movie_id
ORDER BY
  mh.production_year DESC,
  mh.level,
  mh.title;
