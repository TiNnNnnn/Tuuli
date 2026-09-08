WITH RankedMovies AS (
  SELECT
    a.id AS movie_id,
    a.title,
    a.production_year,
    a.kind_id,
    ROW_NUMBER() OVER (PARTITION BY a.production_year ORDER BY COUNT(DISTINCT ci.person_id) DESC) AS movie_rank
  FROM aka_title AS a
  JOIN cast_info AS ci
    ON a.id = ci.movie_id
  GROUP BY
    a.id,
    a.title,
    a.production_year,
    a.kind_id
), TopMovies AS (
  SELECT
    *
  FROM RankedMovies
  WHERE
    movie_rank <= 5
), MovieDetails AS (
  SELECT
    tm.movie_id,
    tm.title,
    tm.production_year,
    COUNT(DISTINCT mc.company_id) AS company_count,
    STRING_AGG(DISTINCT c.kind, ', ') AS company_types,
    STRING_AGG(DISTINCT kw.keyword, ', ') AS keywords
  FROM TopMovies AS tm
  LEFT JOIN movie_companies AS mc
    ON tm.movie_id = mc.movie_id
  LEFT JOIN company_type AS c
    ON mc.company_type_id = c.id
  LEFT JOIN movie_keyword AS mw
    ON tm.movie_id = mw.movie_id
  LEFT JOIN keyword AS kw
    ON mw.keyword_id = kw.id
  GROUP BY
    tm.movie_id,
    tm.title,
    tm.production_year
)
SELECT
  md.title,
  md.production_year,
  md.company_count,
  md.company_types,
  md.keywords,
  ak.name AS main_actor
FROM MovieDetails AS md
JOIN cast_info AS ci
  ON md.movie_id = ci.movie_id
JOIN aka_name AS ak
  ON ci.person_id = ak.person_id
WHERE
  ci.nr_order = 1
ORDER BY
  md.production_year DESC,
  md.title;
