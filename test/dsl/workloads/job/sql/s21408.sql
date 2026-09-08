WITH RecursiveTopMovies AS (
  SELECT
    a.id AS movie_id,
    a.title,
    a.production_year,
    COUNT(DISTINCT ca.person_id) AS cast_count,
    RANK() OVER (PARTITION BY a.production_year ORDER BY COUNT(DISTINCT ca.person_id) DESC) AS rank_per_year
  FROM aka_title AS a
  LEFT JOIN cast_info AS ca
    ON a.movie_id = ca.movie_id
  GROUP BY
    a.id,
    a.title,
    a.production_year
), FilteredTopMovies AS (
  SELECT
    movie_id,
    title,
    production_year,
    cast_count
  FROM RecursiveTopMovies
  WHERE
    rank_per_year <= 5
), RelatedMovieKeywords AS (
  SELECT
    mt.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  JOIN aka_title AS mt
    ON mk.movie_id = mt.movie_id
  GROUP BY
    mt.movie_id
), TitleWithKeywordInfo AS (
  SELECT
    ft.movie_id,
    ft.title,
    ft.production_year,
    ft.cast_count,
    COALESCE(rmk.keywords, 'No Keywords') AS keywords
  FROM FilteredTopMovies AS ft
  LEFT JOIN RelatedMovieKeywords AS rmk
    ON ft.movie_id = rmk.movie_id
)
SELECT
  twi.title,
  twi.production_year,
  twi.cast_count,
  twi.keywords,
  CASE
    WHEN twi.cast_count >= 10
    THEN 'High Cast'
    WHEN twi.cast_count BETWEEN 5 AND 9
    THEN 'Medium Cast'
    ELSE 'Low Cast'
  END AS cast_category
FROM TitleWithKeywordInfo AS twi
WHERE
  twi.production_year >= 2000
ORDER BY
  twi.production_year DESC,
  twi.cast_count DESC;
