WITH RankedTitles AS (
  SELECT
    a.title,
    a.production_year,
    a.imdb_index,
    COUNT(DISTINCT c.person_id) AS actor_count
  FROM aka_title AS a
  JOIN complete_cast AS cc
    ON a.id = cc.movie_id
  JOIN cast_info AS c
    ON cc.subject_id = c.id
  WHERE
    a.production_year >= 2000
  GROUP BY
    a.title,
    a.production_year,
    a.imdb_index
  ORDER BY
    actor_count DESC
  LIMIT 10
), MovieKeywords AS (
  SELECT
    m.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS m
  JOIN keyword AS k
    ON m.keyword_id = k.id
  GROUP BY
    m.movie_id
), DetailedInfo AS (
  SELECT
    r.title,
    r.production_year,
    r.actor_count,
    mk.keywords
  FROM RankedTitles AS r
  LEFT JOIN MovieKeywords AS mk
    ON r.imdb_index = CAST(mk.movie_id AS VARCHAR)
)
SELECT
  d.title,
  d.production_year,
  d.actor_count,
  d.keywords
FROM DetailedInfo AS d
ORDER BY
  d.actor_count DESC,
  d.production_year DESC;
