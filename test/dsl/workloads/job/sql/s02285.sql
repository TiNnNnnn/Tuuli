WITH MovieDetails AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    COUNT(DISTINCT c.person_id) AS cast_count,
    STRING_AGG(DISTINCT k.keyword, ', ') AS keywords
  FROM aka_title AS t
  LEFT JOIN complete_cast AS cc
    ON t.id = cc.movie_id
  LEFT JOIN cast_info AS c
    ON cc.id = c.movie_id
  LEFT JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    t.id,
    t.title,
    t.production_year
), TopMovies AS (
  SELECT
    *,
    ROW_NUMBER() OVER (ORDER BY cast_count DESC) AS rn
  FROM MovieDetails
  WHERE
    production_year >= 2000
)
SELECT
  m.movie_id,
  m.title,
  m.production_year,
  COALESCE(m.cast_count, 0) AS cast_count,
  COALESCE(m.keywords, 'No keywords') AS keywords,
  (
    SELECT
      COUNT(DISTINCT mc.company_id)
    FROM movie_companies AS mc
    WHERE
      mc.movie_id = m.movie_id AND NOT mc.company_type_id IS NULL
  ) AS company_count,
  (
    SELECT
      STRING_AGG(DISTINCT c.name, ', ')
    FROM company_name AS c
    JOIN movie_companies AS mc
      ON c.id = mc.company_id
    WHERE
      mc.movie_id = m.movie_id
  ) AS company_names
FROM TopMovies AS m
WHERE
  m.rn <= 10
ORDER BY
  m.cast_count DESC;
