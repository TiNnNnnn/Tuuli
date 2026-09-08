WITH MovieDetails AS (
  SELECT
    t.id AS movie_id,
    t.title AS movie_title,
    t.production_year,
    ak.name AS actor_name,
    ct.kind AS company_type,
    k.keyword AS movie_keyword
  FROM aka_title AS t
  JOIN cast_info AS c
    ON t.id = c.movie_id
  JOIN aka_name AS ak
    ON c.person_id = ak.person_id
  JOIN movie_companies AS mc
    ON t.id = mc.movie_id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
  LEFT JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  WHERE
    t.production_year >= 2000 AND NOT ak.name IS NULL
), AggregatedData AS (
  SELECT
    movie_id,
    movie_title,
    production_year,
    STRING_AGG(DISTINCT actor_name, ', ') AS actors,
    STRING_AGG(DISTINCT company_type, ', ') AS companies,
    STRING_AGG(DISTINCT movie_keyword, ', ') AS keywords
  FROM MovieDetails
  GROUP BY
    movie_id,
    movie_title,
    production_year
)
SELECT
  movie_id,
  movie_title,
  production_year,
  actors,
  companies,
  keywords
FROM AggregatedData
ORDER BY
  production_year DESC,
  movie_title;
