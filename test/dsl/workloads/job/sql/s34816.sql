WITH RECURSIVE MovieHierarchy AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    CAST(m.title AS TEXT) AS full_title,
    1 AS depth
  FROM aka_title AS m
  WHERE
    NOT m.production_year IS NULL
  UNION ALL
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    CONCAT(mh.full_title, ' -> ', m.title) AS full_title,
    mh.depth + 1
  FROM MovieHierarchy AS mh
  JOIN aka_title AS m
    ON m.episode_of_id = mh.movie_id
), CollatedCast AS (
  SELECT
    ci.movie_id,
    COUNT(ci.person_id) AS cast_count,
    STRING_AGG(a.name, ', ') AS actors,
    MAX(CASE WHEN NOT a.name IS NULL THEN 1 ELSE 0 END) AS has_actors
  FROM cast_info AS ci
  JOIN aka_name AS a
    ON ci.person_id = a.person_id
  GROUP BY
    ci.movie_id
), MovieInfo AS (
  SELECT
    m.id AS movie_id,
    COALESCE(k.keyword, 'No Keyword') AS keyword,
    COALESCE(mi.info, 'No Info') AS additional_info,
    CASE
      WHEN c.cast_count > 0
      THEN (
        'This movie has ' || c.cast_count || ' total cast members.'
      )
      ELSE 'This movie has no cast members.'
    END AS cast_description,
    mh.title AS movie_title,
    mh.production_year,
    mh.depth
  FROM aka_title AS m
  LEFT JOIN CollatedCast AS c
    ON m.id = c.movie_id
  LEFT JOIN movie_keyword AS mk
    ON m.id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  LEFT JOIN MovieHierarchy AS mh
    ON mh.movie_id = m.id
  LEFT JOIN movie_info AS mi
    ON m.id = mi.movie_id
  WHERE
    m.production_year BETWEEN 2000 AND 2020
)
SELECT
  mi.movie_title,
  mi.production_year,
  mi.keyword,
  mi.additional_info,
  mi.cast_description,
  mh.depth AS hierarchy_level,
  ROW_NUMBER() OVER (PARTITION BY mi.keyword ORDER BY mi.production_year DESC) AS keyword_rank
FROM MovieInfo AS mi
JOIN MovieHierarchy AS mh
  ON mi.movie_id = mh.movie_id
WHERE
  mh.depth < 5
ORDER BY
  mi.production_year DESC,
  mi.keyword;
