WITH RankedMovies AS (
  SELECT
    a.title,
    a.production_year,
    COUNT(DISTINCT c.person_id) AS cast_count,
    ROW_NUMBER() OVER (PARTITION BY a.production_year ORDER BY COUNT(DISTINCT c.person_id) DESC) AS rank
  FROM aka_title AS a
  LEFT JOIN cast_info AS c
    ON a.id = c.movie_id
  WHERE
    a.production_year >= 2000
  GROUP BY
    a.title,
    a.production_year
)
SELECT
  rm.title,
  rm.production_year,
  rm.cast_count,
  (
    SELECT
      STRING_AGG(j.name, ', ')
    FROM aka_name AS j
    JOIN cast_info AS ci
      ON j.person_id = ci.person_id
    WHERE
      ci.movie_id = (
        SELECT
          id
        FROM aka_title
        WHERE
          title = rm.title AND production_year = rm.production_year
        LIMIT 1
      )
  ) AS cast_names,
  (
    CASE
      WHEN rm.cast_count IS NULL
      THEN 'No cast info'
      WHEN rm.cast_count > 10
      THEN 'Large cast'
      ELSE 'Small cast'
    END
  ) AS cast_size_category
FROM RankedMovies AS rm
WHERE
  rm.rank <= 3
ORDER BY
  rm.production_year DESC,
  rm.cast_count DESC;
