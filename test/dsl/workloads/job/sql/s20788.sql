WITH RankedMovies AS (
  SELECT
    m.title,
    m.production_year,
    ROW_NUMBER() OVER (PARTITION BY m.production_year ORDER BY COUNT(DISTINCT ca.person_id) DESC) AS movie_rank,
    COUNT(DISTINCT ca.person_id) AS cast_count
  FROM aka_title AS m
  LEFT JOIN cast_info AS ca
    ON m.id = ca.movie_id
  WHERE
    m.kind_id IN (
      SELECT
        id
      FROM kind_type
      WHERE
        kind LIKE 'feature%'
    )
  GROUP BY
    m.id,
    m.title,
    m.production_year
), TopMovies AS (
  SELECT
    title,
    production_year,
    cast_count
  FROM RankedMovies
  WHERE
    movie_rank = 1
), MovieKeywords AS (
  SELECT
    km.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS km
  JOIN keyword AS k
    ON km.keyword_id = k.id
  GROUP BY
    km.movie_id
), MovieInfo AS (
  SELECT
    mi.movie_id,
    MAX(CASE WHEN it.info = 'summary' THEN mi.info END) AS summary,
    MAX(CASE WHEN it.info = 'rating' THEN mi.info END) AS rating
  FROM movie_info AS mi
  JOIN info_type AS it
    ON mi.info_type_id = it.id
  GROUP BY
    mi.movie_id
)
SELECT
  tm.title,
  tm.production_year,
  tm.cast_count,
  COALESCE(mk.keywords, 'No keywords') AS keywords,
  COALESCE(mi.summary, 'No summary available') AS summary,
  COALESCE(mi.rating, 'No rating available') AS rating
FROM TopMovies AS tm
LEFT JOIN MovieKeywords AS mk
  ON tm.title = (
    SELECT
      title
    FROM aka_title
    WHERE
      id = mk.movie_id
    LIMIT 1
  )
LEFT JOIN MovieInfo AS mi
  ON tm.title = (
    SELECT
      title
    FROM aka_title
    WHERE
      id = mi.movie_id
    LIMIT 1
  )
ORDER BY
  tm.production_year DESC,
  tm.title;
