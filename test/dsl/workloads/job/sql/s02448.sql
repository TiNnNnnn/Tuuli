WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.title) AS rank
  FROM aka_title AS t
  WHERE
    t.kind_id IN (
      SELECT
        id
      FROM kind_type
      WHERE
        kind = 'movie'
    )
), MoviesWithKeywords AS (
  SELECT
    rm.movie_id,
    rm.title,
    rm.production_year,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM RankedMovies AS rm
  LEFT JOIN movie_keyword AS mk
    ON rm.movie_id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    rm.movie_id,
    rm.title,
    rm.production_year
), ActorInfo AS (
  SELECT
    c.movie_id,
    a.name,
    COUNT(c.person_id) AS actor_count
  FROM cast_info AS c
  JOIN aka_name AS a
    ON c.person_id = a.person_id
  GROUP BY
    c.movie_id,
    a.name
), TopMovies AS (
  SELECT
    mwk.movie_id,
    mwk.title,
    mwk.production_year,
    mwk.keywords,
    COALESCE(ai.actor_count, 0) AS actor_count
  FROM MoviesWithKeywords AS mwk
  LEFT JOIN ActorInfo AS ai
    ON mwk.movie_id = ai.movie_id
  WHERE
    mwk.production_year >= 2000
  ORDER BY
    mwk.production_year DESC,
    mwk.title
)
SELECT DISTINCT
  title,
  production_year,
  keywords,
  actor_count
FROM TopMovies
WHERE
  actor_count > (
    SELECT
      AVG(actor_count)
    FROM ActorInfo
  )
ORDER BY
  production_year DESC,
  title;
