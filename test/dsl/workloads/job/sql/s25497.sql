WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    k.keyword,
    COUNT(DISTINCT c.person_id) AS actor_count
  FROM aka_title AS t
  JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  LEFT JOIN cast_info AS c
    ON t.id = c.movie_id
  WHERE
    t.production_year >= 2000
  GROUP BY
    t.id,
    t.title,
    t.production_year,
    k.keyword
), TopMovies AS (
  SELECT
    movie_id,
    title,
    production_year,
    keyword,
    actor_count,
    RANK() OVER (PARTITION BY keyword ORDER BY actor_count DESC) AS rank
  FROM RankedMovies
)
SELECT
  tm.title,
  tm.production_year,
  tm.keyword,
  tm.actor_count
FROM TopMovies AS tm
WHERE
  tm.rank <= 10
ORDER BY
  tm.keyword,
  tm.actor_count DESC;
