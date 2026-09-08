WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    COUNT(DISTINCT ci.person_id) AS cast_count,
    STRING_AGG(DISTINCT a.name, ', ') AS actors,
    COALESCE(STRING_AGG(DISTINCT k.keyword, ', '), 'No Keywords') AS keywords
  FROM title AS t
  LEFT JOIN cast_info AS ci
    ON t.id = ci.movie_id
  LEFT JOIN aka_name AS a
    ON ci.person_id = a.person_id
  LEFT JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  WHERE
    t.production_year >= 2000
  GROUP BY
    t.id,
    t.title,
    t.production_year
), RankedActors AS (
  SELECT
    a.person_id,
    a.name,
    COUNT(DISTINCT ci.movie_id) AS movie_count
  FROM aka_name AS a
  LEFT JOIN cast_info AS ci
    ON a.person_id = ci.person_id
  GROUP BY
    a.person_id,
    a.name
  HAVING
    COUNT(DISTINCT ci.movie_id) > 5
), FinalResults AS (
  SELECT
    rm.movie_id,
    rm.title,
    rm.production_year,
    rm.cast_count,
    rm.actors,
    ra.movie_count AS actor_movie_count
  FROM RankedMovies AS rm
  JOIN RankedActors AS ra
    ON rm.actors LIKE '%' || ra.name || '%'
  ORDER BY
    rm.production_year DESC,
    rm.cast_count DESC
)
SELECT
  fr.movie_id,
  fr.title,
  fr.production_year,
  fr.cast_count,
  fr.actors,
  fr.actor_movie_count
FROM FinalResults AS fr
WHERE
  fr.actor_movie_count >= 2
LIMIT 50;
