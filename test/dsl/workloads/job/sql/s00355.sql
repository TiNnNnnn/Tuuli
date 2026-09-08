WITH movie_years AS (
  SELECT
    production_year,
    COUNT(*) AS movie_count,
    AVG(CAST(EXTRACT(YEAR FROM CAST('2024-10-01' AS DATE)) AS INT) - production_year) AS avg_age
  FROM aka_title
  GROUP BY
    production_year
), actor_movie_counts AS (
  SELECT
    a.person_id,
    COUNT(DISTINCT c.movie_id) AS total_movies,
    AVG(m.production_year) AS avg_movie_year
  FROM cast_info AS c
  JOIN aka_name AS a
    ON a.person_id = c.person_id
  JOIN aka_title AS m
    ON m.id = c.movie_id
  GROUP BY
    a.person_id
), interesting_movies AS (
  SELECT
    m.title,
    m.production_year,
    m.kind_id,
    COALESCE(k.keyword, 'No Keywords') AS keyword,
    ROW_NUMBER() OVER (PARTITION BY m.production_year ORDER BY m.production_year DESC) AS rn
  FROM aka_title AS m
  LEFT JOIN movie_keyword AS mk
    ON mk.movie_id = m.id
  LEFT JOIN keyword AS k
    ON k.id = mk.keyword_id
  WHERE
    m.production_year >= (
      SELECT
        MIN(production_year)
      FROM movie_years
      WHERE
        movie_count > 5
    )
), top_actors AS (
  SELECT
    an.name,
    ac.total_movies,
    my.avg_age
  FROM actor_movie_counts AS ac
  JOIN aka_name AS an
    ON an.person_id = ac.person_id
  JOIN movie_years AS my
    ON ac.avg_movie_year < my.production_year
  WHERE
    ac.total_movies > 10
)
SELECT
  t.title,
  t.production_year,
  t.keyword,
  ta.name AS top_actor,
  ta.total_movies,
  ROW_NUMBER() OVER (ORDER BY t.production_year DESC) AS movie_rank
FROM interesting_movies AS t
JOIN top_actors AS ta
  ON ta.total_movies = (
    SELECT
      MAX(total_movies)
    FROM top_actors
  )
WHERE
  t.rn <= 5
ORDER BY
  t.production_year DESC;
