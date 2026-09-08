WITH RankedMovies AS (
  SELECT
    at.id AS title_id,
    at.title,
    at.production_year,
    COALESCE(SUM(ci.nr_order), 0) AS total_cast,
    ROW_NUMBER() OVER (PARTITION BY at.production_year ORDER BY COALESCE(SUM(ci.nr_order), 0) DESC) AS year_rank
  FROM aka_title AS at
  LEFT JOIN cast_info AS ci
    ON at.movie_id = ci.movie_id
  GROUP BY
    at.id,
    at.title,
    at.production_year
), TopMovies AS (
  SELECT
    title_id,
    title,
    production_year,
    total_cast
  FROM RankedMovies
  WHERE
    year_rank <= 5
), CompanyDetails AS (
  SELECT
    mc.movie_id,
    c.name AS company_name,
    ct.kind AS company_type,
    mc.note AS company_note
  FROM movie_companies AS mc
  JOIN company_name AS c
    ON mc.company_id = c.id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
), KeywordCounts AS (
  SELECT
    mk.movie_id,
    COUNT(DISTINCT k.keyword) AS keyword_count
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    mk.movie_id
)
SELECT
  tm.title,
  tm.production_year,
  tm.total_cast,
  COALESCE(cd.company_name, 'Unknown Company') AS company_name,
  COALESCE(cd.company_type, 'Independent') AS company_type,
  COALESCE(kc.keyword_count, 0) AS unique_keywords,
  (
    CASE
      WHEN tm.total_cast > 10
      THEN 'Large Cast'
      WHEN tm.total_cast BETWEEN 5 AND 10
      THEN 'Medium Cast'
      ELSE 'Small Cast'
    END
  ) AS cast_size
FROM TopMovies AS tm
LEFT JOIN CompanyDetails AS cd
  ON tm.title_id = cd.movie_id
LEFT JOIN KeywordCounts AS kc
  ON tm.title_id = kc.movie_id
WHERE
  (
    tm.production_year >= 2000 AND tm.production_year < 2023
  )
  AND (
    cd.company_note IS NULL OR NOT cd.company_note LIKE '%cameo%'
  )
ORDER BY
  tm.production_year DESC,
  tm.total_cast DESC;
