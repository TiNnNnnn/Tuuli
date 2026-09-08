WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title AS movie_title,
    t.production_year,
    COUNT(ci.person_id) AS cast_count,
    STRING_AGG(DISTINCT an.name, ', ') AS actor_names
  FROM aka_title AS t
  JOIN complete_cast AS cc
    ON t.id = cc.movie_id
  JOIN cast_info AS ci
    ON cc.subject_id = ci.id
  JOIN aka_name AS an
    ON ci.person_id = an.person_id
  WHERE
    t.production_year BETWEEN 2000 AND 2020
  GROUP BY
    t.id,
    t.title,
    t.production_year
  ORDER BY
    cast_count DESC
  LIMIT 10
), MovieKeywords AS (
  SELECT
    mk.movie_id,
    STRING_AGG(DISTINCT k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    mk.movie_id
)
SELECT
  rm.movie_id,
  rm.movie_title,
  rm.production_year,
  rm.cast_count,
  rm.actor_names,
  mk.keywords
FROM RankedMovies AS rm
LEFT JOIN MovieKeywords AS mk
  ON rm.movie_id = mk.movie_id
ORDER BY
  rm.production_year DESC;
