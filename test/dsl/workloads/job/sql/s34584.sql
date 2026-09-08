WITH RECURSIVE MovieHierarchy AS (
  SELECT
    mt.id AS movie_id,
    mt.title,
    mt.production_year,
    1 AS level
  FROM aka_title AS mt
  WHERE
    mt.kind_id = 1
  UNION ALL
  SELECT
    ml.linked_movie_id AS movie_id,
    at.title,
    at.production_year,
    mh.level + 1
  FROM MovieHierarchy AS mh
  JOIN movie_link AS ml
    ON mh.movie_id = ml.movie_id
  JOIN aka_title AS at
    ON ml.linked_movie_id = at.id
  WHERE
    mh.level < 5
), CastDetails AS (
  SELECT
    ci.movie_id,
    ak.name AS actor_name,
    COUNT(DISTINCT ci.id) AS role_count
  FROM cast_info AS ci
  JOIN aka_name AS ak
    ON ci.person_id = ak.person_id
  GROUP BY
    ci.movie_id,
    ak.name
), MovieKeyword AS (
  SELECT
    mk.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    mk.movie_id
)
SELECT
  mh.title,
  mh.production_year,
  COUNT(DISTINCT cd.actor_name) AS total_actors,
  COALESCE(mk.keywords, 'No Keywords') AS associated_keywords,
  ROW_NUMBER() OVER (ORDER BY mh.production_year DESC) AS rank
FROM MovieHierarchy AS mh
LEFT JOIN CastDetails AS cd
  ON mh.movie_id = cd.movie_id
LEFT JOIN MovieKeyword AS mk
  ON mh.movie_id = mk.movie_id
GROUP BY
  mh.movie_id,
  mh.title,
  mh.production_year,
  mk.keywords
HAVING
  COUNT(DISTINCT cd.actor_name) > 2
ORDER BY
  mh.production_year DESC,
  total_actors DESC;
