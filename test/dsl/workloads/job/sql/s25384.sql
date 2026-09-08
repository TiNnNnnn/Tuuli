WITH movie_aggregates AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    COUNT(DISTINCT c.person_id) AS cast_count,
    STRING_AGG(DISTINCT a.name, ', ') AS cast_names,
    STRING_AGG(DISTINCT k.keyword, ', ') AS keywords
  FROM aka_title AS t
  LEFT JOIN cast_info AS c
    ON t.id = c.movie_id
  LEFT JOIN aka_name AS a
    ON c.person_id = a.person_id
  LEFT JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    t.id,
    t.title,
    t.production_year
), company_aggregates AS (
  SELECT
    m.movie_id,
    STRING_AGG(DISTINCT co.name, ', ') AS companies,
    STRING_AGG(DISTINCT ct.kind, ', ') AS company_types
  FROM movie_companies AS m
  JOIN company_name AS co
    ON m.company_id = co.id
  JOIN company_type AS ct
    ON m.company_type_id = ct.id
  GROUP BY
    m.movie_id
), final_result AS (
  SELECT
    ma.movie_id,
    ma.title,
    ma.production_year,
    ma.cast_count,
    ma.cast_names,
    co.companies,
    co.company_types,
    ma.keywords
  FROM movie_aggregates AS ma
  LEFT JOIN company_aggregates AS co
    ON ma.movie_id = co.movie_id
)
SELECT
  movie_id,
  title,
  production_year,
  cast_count,
  cast_names,
  companies,
  company_types,
  CASE WHEN cast_count > 0 THEN 'Has Cast' ELSE 'No Cast' END AS cast_status,
  COALESCE(keywords, 'No Keywords') AS keywords_present
FROM final_result
ORDER BY
  production_year DESC,
  cast_count DESC;
