WITH RankedTitles AS (
  SELECT
    t.id AS title_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.id) AS title_rank
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
), ActorMovies AS (
  SELECT
    c.movie_id,
    COUNT(*) AS actor_count
  FROM cast_info AS c
  GROUP BY
    c.movie_id
), CompanyMovies AS (
  SELECT
    mc.movie_id,
    COUNT(DISTINCT cn.id) AS company_count
  FROM movie_companies AS mc
  JOIN company_name AS cn
    ON mc.company_id = cn.id
  WHERE
    NOT cn.country_code IS NULL
  GROUP BY
    mc.movie_id
)
SELECT
  rt.title,
  rt.production_year,
  am.actor_count,
  cm.company_count,
  (
    CASE
      WHEN am.actor_count IS NULL
      THEN 'No Actors'
      ELSE CAST(am.actor_count AS VARCHAR) || ' Actors'
    END
  ) AS actor_info,
  (
    CASE
      WHEN cm.company_count IS NULL
      THEN 'No Companies'
      ELSE CAST(cm.company_count AS VARCHAR) || ' Companies'
    END
  ) AS company_info
FROM RankedTitles AS rt
LEFT JOIN ActorMovies AS am
  ON rt.title_id = am.movie_id
LEFT JOIN CompanyMovies AS cm
  ON rt.title_id = cm.movie_id
WHERE
  rt.title_rank <= 5
ORDER BY
  rt.production_year DESC,
  rt.title;
