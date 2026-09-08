WITH ranked_movies AS (
  SELECT
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.title) AS rn,
    COUNT(*) OVER (PARTITION BY t.production_year) AS total_movies
  FROM aka_title AS t
  WHERE
    t.kind_id = (
      SELECT
        id
      FROM kind_type
      WHERE
        kind = 'movie'
    )
    AND NOT t.production_year IS NULL
), movie_cast AS (
  SELECT
    m.title,
    c.person_id,
    a.name AS actor_name,
    r.role AS role_name,
    m.production_year
  FROM ranked_movies AS m
  JOIN complete_cast AS cc
    ON cc.movie_id = (
      SELECT
        id
      FROM aka_title
      WHERE
        title = m.title
      LIMIT 1
    )
  JOIN cast_info AS c
    ON c.movie_id = cc.movie_id
  LEFT JOIN aka_name AS a
    ON a.person_id = c.person_id
  LEFT JOIN role_type AS r
    ON c.role_id = r.id
)
SELECT
  mc.title,
  mc.production_year,
  COUNT(DISTINCT mc.actor_name) AS actor_count,
  STRING_AGG(DISTINCT mc.actor_name, ', ') AS actors_list,
  (
    SELECT
      COUNT(*)
    FROM movie_companies AS mc2
    WHERE
      mc2.movie_id = (
        SELECT
          id
        FROM aka_title
        WHERE
          title = mc.title
        LIMIT 1
      )
  ) AS company_count,
  MAX(CASE WHEN mc.production_year < 2000 THEN 'Classic' ELSE 'Modern' END) AS movie_era
FROM movie_cast AS mc
GROUP BY
  mc.title,
  mc.production_year
HAVING
  COUNT(DISTINCT mc.actor_name) > 3
ORDER BY
  mc.production_year DESC,
  mc.title ASC
LIMIT 10;
