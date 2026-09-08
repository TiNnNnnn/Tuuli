WITH MovieDetails AS (
  SELECT
    t.id AS movie_id,
    t.title AS movie_title,
    t.production_year,
    STRING_AGG(DISTINCT k.keyword, ', ') AS keywords,
    STRING_AGG(DISTINCT c.name, ', ') AS companies,
    COUNT(DISTINCT ci.person_id) AS cast_count
  FROM aka_title AS t
  JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  JOIN complete_cast AS cc
    ON t.id = cc.movie_id
  JOIN cast_info AS ci
    ON cc.subject_id = ci.id
  JOIN movie_companies AS mc
    ON t.id = mc.movie_id
  JOIN company_name AS c
    ON mc.company_id = c.id
  WHERE
    t.production_year >= 2000
  GROUP BY
    t.id,
    t.title,
    t.production_year
), DetailedActors AS (
  SELECT
    a.person_id,
    a.name,
    COUNT(DISTINCT md.movie_id) AS movies_count,
    SUM(md.production_year) AS total_years_active
  FROM aka_name AS a
  JOIN cast_info AS ci
    ON a.person_id = ci.person_id
  JOIN MovieDetails AS md
    ON ci.movie_id = md.movie_id
  GROUP BY
    a.person_id,
    a.name
)
SELECT
  da.name,
  da.movies_count,
  da.total_years_active,
  md.movie_title,
  md.production_year,
  md.keywords,
  md.companies,
  md.cast_count
FROM DetailedActors AS da
JOIN MovieDetails AS md
  ON da.movies_count > 1
ORDER BY
  da.movies_count DESC,
  md.production_year DESC;
