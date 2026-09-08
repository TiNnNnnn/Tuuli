WITH ranked_movies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    k.keyword,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.title) AS rank
  FROM aka_title AS t
  JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  WHERE
    t.production_year BETWEEN 2000 AND 2020
), cast_summary AS (
  SELECT
    c.movie_id,
    COUNT(DISTINCT c.person_id) AS cast_count,
    STRING_AGG(DISTINCT a.name, ', ') AS cast_names
  FROM cast_info AS c
  JOIN aka_name AS a
    ON c.person_id = a.person_id
  GROUP BY
    c.movie_id
), movie_details AS (
  SELECT
    r.movie_id,
    r.title,
    r.production_year,
    cs.cast_count,
    cs.cast_names,
    ROW_NUMBER() OVER (ORDER BY r.production_year DESC, r.movie_id) AS movie_rank
  FROM ranked_movies AS r
  LEFT JOIN cast_summary AS cs
    ON r.movie_id = cs.movie_id
)
SELECT
  md.title,
  md.production_year,
  md.cast_count,
  md.cast_names,
  CASE
    WHEN md.cast_count > 10
    THEN 'Large Cast'
    WHEN md.cast_count BETWEEN 5 AND 10
    THEN 'Medium Cast'
    ELSE 'Small Cast'
  END AS cast_size_category
FROM movie_details AS md
WHERE
  md.movie_rank <= 50
ORDER BY
  md.production_year DESC,
  md.title;
