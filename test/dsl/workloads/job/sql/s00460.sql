WITH ranked_movies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.production_year DESC) AS rank_per_year
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
), actor_count AS (
  SELECT
    ci.movie_id,
    COUNT(DISTINCT ci.person_id) AS total_actors
  FROM cast_info AS ci
  GROUP BY
    ci.movie_id
), company_info AS (
  SELECT
    mc.movie_id,
    cn.name AS company_name,
    ct.kind AS company_type
  FROM movie_companies AS mc
  JOIN company_name AS cn
    ON mc.company_id = cn.id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
), movies_with_info AS (
  SELECT
    rm.movie_id,
    rm.title,
    rm.production_year,
    ac.total_actors,
    ci.company_name,
    ci.company_type
  FROM ranked_movies AS rm
  LEFT JOIN actor_count AS ac
    ON rm.movie_id = ac.movie_id
  LEFT JOIN company_info AS ci
    ON rm.movie_id = ci.movie_id
)
SELECT
  mw.title,
  mw.production_year,
  COALESCE(mw.total_actors, 0) AS actor_count,
  STRING_AGG(DISTINCT mw.company_name, ', ') AS companies,
  COUNT(*) FILTER(WHERE
    mw.production_year = 2023) OVER () AS movies_from_2023,
  CASE
    WHEN mw.production_year IS NULL
    THEN 'Unknown Year'
    ELSE CAST(mw.production_year AS VARCHAR)
  END AS formatted_year
FROM movies_with_info AS mw
WHERE
  NOT mw.company_name IS NULL
GROUP BY
  mw.title,
  mw.production_year,
  mw.total_actors
ORDER BY
  mw.production_year DESC,
  actor_count DESC;
