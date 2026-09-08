WITH RankedMovies AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    COUNT(DISTINCT c.person_id) AS cast_count,
    STRING_AGG(DISTINCT a.name, ', ') AS actors
  FROM aka_title AS m
  JOIN cast_info AS c
    ON m.id = c.movie_id
  JOIN aka_name AS a
    ON c.person_id = a.person_id
  GROUP BY
    m.id,
    m.title,
    m.production_year
), MovieKeywords AS (
  SELECT
    mk.movie_id,
    STRING_AGG(DISTINCT k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    mk.movie_id
), MovieCompanies AS (
  SELECT
    mc.movie_id,
    STRING_AGG(DISTINCT cn.name, ', ') AS companies
  FROM movie_companies AS mc
  JOIN company_name AS cn
    ON mc.company_id = cn.id
  GROUP BY
    mc.movie_id
)
SELECT
  rm.title,
  rm.production_year,
  rm.cast_count,
  rm.actors,
  mk.keywords,
  mc.companies
FROM RankedMovies AS rm
LEFT JOIN MovieKeywords AS mk
  ON rm.movie_id = mk.movie_id
LEFT JOIN MovieCompanies AS mc
  ON rm.movie_id = mc.movie_id
WHERE
  rm.production_year >= 2000
ORDER BY
  rm.production_year DESC,
  rm.cast_count DESC;
