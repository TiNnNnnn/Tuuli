WITH RankedTitles AS (
  SELECT
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.title) AS title_rank
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
), AverageMovieYear AS (
  SELECT
    AVG(COALESCE(t.production_year, 0)) AS avg_year
  FROM aka_title AS t
), ActorMovieCounts AS (
  SELECT
    ci.person_id,
    COUNT(DISTINCT ci.movie_id) AS movie_count
  FROM cast_info AS ci
  JOIN aka_name AS an
    ON ci.person_id = an.person_id
  GROUP BY
    ci.person_id
)
SELECT
  an.name AS actor_name,
  COUNT(DISTINCT ci.movie_id) AS total_movies,
  AVG(COALESCE(amt.avg_year, 0)) AS average_production_year,
  STRING_AGG(DISTINCT rt.title, ', ') AS titles_in_years,
  CASE
    WHEN COUNT(DISTINCT ci.movie_id) > (
      SELECT
        AVG(movie_count)
      FROM ActorMovieCounts
    )
    THEN 'Active Actor'
    ELSE 'Less Active Actor'
  END AS activity_status
FROM aka_name AS an
LEFT JOIN cast_info AS ci
  ON an.person_id = ci.person_id
LEFT JOIN RankedTitles AS rt
  ON ci.movie_id = (
    SELECT
      id
    FROM aka_title
    WHERE
      title = rt.title AND production_year = rt.production_year
    LIMIT 1
  )
LEFT JOIN AverageMovieYear AS amt
  ON TRUE
WHERE
  NOT an.name IS NULL
GROUP BY
  an.name
HAVING
  COUNT(DISTINCT ci.movie_id) > 0
ORDER BY
  total_movies DESC;
