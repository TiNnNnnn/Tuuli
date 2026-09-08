WITH RankedMovies AS (
  SELECT
    m.id AS movie_id,
    m.title,
    COALESCE(m.production_year, 0) AS production_year,
    ROW_NUMBER() OVER (PARTITION BY COALESCE(m.production_year, 0) ORDER BY m.title) AS rank
  FROM aka_title AS m
  WHERE
    m.kind_id IN (
      SELECT
        id
      FROM kind_type
      WHERE
        kind IN ('movie', 'feature')
    )
), ActorInformation AS (
  SELECT
    a.id AS actor_id,
    a.name,
    COUNT(DISTINCT ci.movie_id) AS movie_count
  FROM aka_name AS a
  JOIN cast_info AS ci
    ON a.person_id = ci.person_id
  GROUP BY
    a.id,
    a.name
), MoviesWithKeywords AS (
  SELECT
    m.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  JOIN RankedMovies AS m
    ON mk.movie_id = m.movie_id
  GROUP BY
    m.movie_id
)
SELECT
  rm.movie_id,
  rm.title,
  rm.production_year,
  ai.name AS actor_name,
  ai.movie_count,
  mwk.keywords
FROM RankedMovies AS rm
LEFT JOIN cast_info AS ci
  ON rm.movie_id = ci.movie_id
LEFT JOIN ActorInformation AS ai
  ON ci.person_id = ai.actor_id
LEFT JOIN MoviesWithKeywords AS mwk
  ON rm.movie_id = mwk.movie_id
WHERE
  (
    rm.production_year > 2000 OR rm.production_year IS NULL
  )
  AND (
    ai.movie_count > 5 OR ai.movie_count IS NULL
  )
ORDER BY
  rm.production_year DESC,
  rm.title;
