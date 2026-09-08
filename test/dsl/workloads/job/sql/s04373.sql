WITH movie_years AS (
  SELECT
    production_year,
    COUNT(*) AS movie_count
  FROM aka_title
  GROUP BY
    production_year
), cast_summary AS (
  SELECT
    c.movie_id,
    COUNT(c.person_id) AS actor_count,
    STRING_AGG(DISTINCT a.name, ', ') AS actor_names
  FROM cast_info AS c
  JOIN aka_name AS a
    ON c.person_id = a.person_id
  GROUP BY
    c.movie_id
), keyword_summary AS (
  SELECT
    mk.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    mk.movie_id
), title_info AS (
  SELECT
    t.id AS title_id,
    t.title,
    COALESCE(years.movie_count, 0) AS total_movies,
    COALESCE(summary.actor_count, 0) AS total_actors,
    COALESCE(keywords.keywords, 'None') AS keyword_list
  FROM title AS t
  LEFT JOIN movie_years AS years
    ON t.production_year = years.production_year
  LEFT JOIN cast_summary AS summary
    ON t.id = summary.movie_id
  LEFT JOIN keyword_summary AS keywords
    ON t.id = keywords.movie_id
)
SELECT
  ti.title_id,
  ti.title,
  COALESCE(ti.total_movies, 0) AS movies_in_year,
  ti.total_actors AS actors_in_movie,
  ti.keyword_list,
  ROW_NUMBER() OVER (ORDER BY ti.total_actors DESC) AS rank_actors,
  ROW_NUMBER() OVER (ORDER BY ti.total_movies DESC) AS rank_movies
FROM title_info AS ti
WHERE
  ti.total_actors > 0 OR ti.total_movies > 0
ORDER BY
  ti.total_actors DESC,
  ti.total_movies DESC
FETCH FIRST 10 ROWS ONLY;
