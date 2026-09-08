WITH RankedTitles AS (
  SELECT
    t.id AS title_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY EXTRACT(YEAR FROM CAST('2024-10-01 12:34:56' AS TIMESTAMP)) - t.production_year ORDER BY t.production_year DESC) AS recent_rank
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
), ActorsInfo AS (
  SELECT
    a.person_id,
    COALESCE(a.name, 'Unknown') AS actor_name,
    COUNT(DISTINCT c.movie_id) AS movie_count
  FROM aka_name AS a
  LEFT JOIN cast_info AS c
    ON a.person_id = c.person_id
  GROUP BY
    a.person_id,
    a.name
), MovieCompanies AS (
  SELECT
    m.movie_id,
    STRING_AGG(c.name, ', ') AS companies
  FROM movie_companies AS m
  JOIN company_name AS c
    ON m.company_id = c.id
  GROUP BY
    m.movie_id
), FilteredMovies AS (
  SELECT
    rt.title_id,
    rt.title,
    rt.production_year,
    COALESCE(mc.companies, 'Unknown') AS companies
  FROM RankedTitles AS rt
  LEFT JOIN MovieCompanies AS mc
    ON rt.title_id = mc.movie_id
  WHERE
    rt.recent_rank <= 10
), FinalOutput AS (
  SELECT
    f.title,
    f.production_year,
    f.companies,
    a.actor_name,
    a.movie_count,
    CASE
      WHEN a.movie_count > 5
      THEN 'Veteran Actor'
      WHEN a.movie_count BETWEEN 3 AND 5
      THEN 'Rising Star'
      ELSE 'Newbie Actor'
    END AS actor_status
  FROM FilteredMovies AS f
  LEFT JOIN ActorsInfo AS a
    ON f.title_id IN (
      SELECT
        movie_id
      FROM cast_info
      WHERE
        person_id = a.person_id
    )
)
SELECT
  title,
  production_year,
  companies,
  actor_name,
  movie_count,
  actor_status
FROM FinalOutput
WHERE
  production_year BETWEEN 2000 AND 2023
ORDER BY
  production_year DESC,
  movie_count DESC
LIMIT 50;
