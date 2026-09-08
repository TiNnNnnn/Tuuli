WITH MovieDetails AS (
  SELECT
    title.id AS movie_id,
    title.title AS movie_title,
    aka_title.title AS aka_title,
    title.production_year,
    STRING_AGG(DISTINCT keyword.keyword, ', ') AS keywords,
    company_name.name AS production_company
  FROM title
  LEFT JOIN aka_title
    ON title.id = aka_title.movie_id
  LEFT JOIN movie_companies
    ON title.id = movie_companies.movie_id
  LEFT JOIN company_name
    ON movie_companies.company_id = company_name.id
  LEFT JOIN movie_keyword
    ON title.id = movie_keyword.movie_id
  LEFT JOIN keyword
    ON movie_keyword.keyword_id = keyword.id
  GROUP BY
    title.id,
    title.title,
    aka_title.title,
    title.production_year,
    company_name.name
), PersonDetails AS (
  SELECT
    aka_name.person_id,
    aka_name.name AS person_name,
    role_type.role AS person_role,
    COUNT(DISTINCT cast_info.movie_id) AS movie_count
  FROM aka_name
  JOIN cast_info
    ON aka_name.person_id = cast_info.person_id
  JOIN role_type
    ON cast_info.role_id = role_type.id
  GROUP BY
    aka_name.person_id,
    aka_name.name,
    role_type.role
)
SELECT
  movie.movie_title,
  movie.production_year,
  movie.keywords,
  person.person_name,
  person.person_role,
  person.movie_count,
  movie.production_company
FROM MovieDetails AS movie
JOIN complete_cast AS cc
  ON movie.movie_id = cc.movie_id
JOIN PersonDetails AS person
  ON cc.subject_id = person.person_id
ORDER BY
  movie.production_year DESC,
  person.movie_count DESC
LIMIT 100;
