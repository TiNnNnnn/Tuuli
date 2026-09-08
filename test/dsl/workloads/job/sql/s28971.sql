WITH MovieNames AS (
  SELECT
    title.id AS movie_id,
    title.title AS movie_title,
    title.production_year,
    aka_name.name AS director_name
  FROM title
  JOIN movie_info
    ON movie_info.movie_id = title.id
  JOIN movie_companies
    ON movie_companies.movie_id = title.id
  JOIN company_name
    ON company_name.id = movie_companies.company_id
  JOIN cast_info
    ON cast_info.movie_id = title.id
  JOIN aka_name
    ON aka_name.person_id = cast_info.person_id
  WHERE
    movie_info.info_type_id = (
      SELECT
        id
      FROM info_type
      WHERE
        info = 'Directed by'
    )
    AND title.production_year > 2000
), KeywordCounts AS (
  SELECT
    movie_id,
    COUNT(keyword.id) AS keyword_count
  FROM movie_keyword
  JOIN keyword
    ON keyword.id = movie_keyword.keyword_id
  GROUP BY
    movie_id
), FinalResults AS (
  SELECT
    mn.movie_id,
    mn.movie_title,
    mn.production_year,
    mn.director_name,
    kc.keyword_count
  FROM MovieNames AS mn
  LEFT JOIN KeywordCounts AS kc
    ON mn.movie_id = kc.movie_id
)
SELECT
  movie_id,
  movie_title,
  production_year,
  director_name,
  COALESCE(keyword_count, 0) AS keyword_count
FROM FinalResults
ORDER BY
  production_year DESC,
  keyword_count DESC,
  movie_title ASC
LIMIT 10;
