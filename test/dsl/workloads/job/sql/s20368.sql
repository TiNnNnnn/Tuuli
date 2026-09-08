WITH RECURSIVE movie_cast AS (
  SELECT
    ci.movie_id,
    ak.name AS actor_name,
    ROW_NUMBER() OVER (PARTITION BY ci.movie_id ORDER BY ci.nr_order) AS actor_order,
    COUNT(*) OVER (PARTITION BY ci.movie_id) AS total_actors
  FROM cast_info AS ci
  JOIN aka_name AS ak
    ON ci.person_id = ak.person_id
  WHERE
    NOT ak.name IS NULL
), movie_details AS (
  SELECT
    mt.title,
    mt.production_year,
    mc.actor_name,
    mc.actor_order,
    mc.total_actors,
    CASE
      WHEN mc.actor_order = 1
      THEN 'Lead Actor'
      WHEN mc.actor_order = total_actors
      THEN 'Last Actor'
      ELSE 'Supporting Actor'
    END AS actor_role,
    COUNT(mi.id) AS info_count
  FROM movie_cast AS mc
  JOIN aka_title AS mt
    ON mc.movie_id = mt.movie_id
  LEFT JOIN movie_info AS mi
    ON mc.movie_id = mi.movie_id
  GROUP BY
    mt.title,
    mt.production_year,
    mc.actor_name,
    mc.actor_order,
    mc.total_actors
), filtered_movies AS (
  SELECT
    *,
    CASE WHEN total_actors > 5 THEN 'Ensemble Cast' ELSE 'Small Cast' END AS cast_size
  FROM movie_details
  WHERE
    production_year BETWEEN 2000 AND 2020 AND actor_role <> 'Last Actor'
), ranked_movies AS (
  SELECT
    *,
    RANK() OVER (PARTITION BY cast_size ORDER BY production_year DESC) AS rank_within_size
  FROM filtered_movies
)
SELECT
  title,
  production_year,
  actor_name,
  actor_role,
  cast_size,
  rank_within_size
FROM ranked_movies
WHERE
  rank_within_size <= 5
ORDER BY
  cast_size DESC,
  production_year DESC;
