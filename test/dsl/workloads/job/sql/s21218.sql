WITH RankedMovies AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    ROW_NUMBER() OVER (PARTITION BY m.production_year ORDER BY COUNT(c.person_id) DESC) AS rank_by_cast_size
  FROM aka_title AS m
  LEFT JOIN cast_info AS c
    ON m.id = c.movie_id
  GROUP BY
    m.id,
    m.title,
    m.production_year
), FilteredMovies AS (
  SELECT
    rm.movie_id,
    rm.title,
    rm.production_year
  FROM RankedMovies AS rm
  WHERE
    rm.rank_by_cast_size <= 5
), MovieKeywords AS (
  SELECT
    mk.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    mk.movie_id
), MovieDetails AS (
  SELECT
    fm.movie_id,
    fm.title,
    fm.production_year,
    COALESCE(mk.keywords, 'No Keywords') AS keywords
  FROM FilteredMovies AS fm
  LEFT JOIN MovieKeywords AS mk
    ON fm.movie_id = mk.movie_id
), UniqueGenres AS (
  SELECT DISTINCT
    kt.kind AS genre
  FROM kind_type AS kt
  JOIN aka_title AS at
    ON kt.id = at.kind_id
), OutstandingMovies AS (
  SELECT
    md.movie_id,
    md.title,
    md.production_year,
    md.keywords,
    CASE
      WHEN md.production_year < 2000
      THEN 'Classic'
      WHEN md.production_year BETWEEN 2000 AND 2010
      THEN 'Modern'
      ELSE 'New Age'
    END AS era,
    ROW_NUMBER() OVER (ORDER BY md.production_year DESC) AS movie_rank
  FROM MovieDetails AS md
)
SELECT
  om.title,
  om.production_year,
  om.keywords,
  om.era,
  ug.genre
FROM OutstandingMovies AS om
LEFT JOIN UniqueGenres AS ug
  ON 1 = 1
WHERE
  (
    om.production_year IS NULL OR om.production_year > 1990
  )
  AND (
    NOT om.keywords IS NULL AND om.keywords <> 'No Keywords'
  )
ORDER BY
  om.production_year DESC,
  om.title
LIMIT 10
OFFSET 5;
