WITH RECURSIVE movie_hierarchy AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    COALESCE(cast_info.person_id, -1) AS actor_id,
    COALESCE(a.name, 'Unknown Actor') AS actor_name,
    1 AS level
  FROM aka_title AS m
  LEFT JOIN cast_info
    ON m.id = cast_info.movie_id
  LEFT JOIN aka_name AS a
    ON cast_info.person_id = a.person_id
  WHERE
    NOT m.production_year IS NULL
  UNION ALL
  SELECT
    mh.movie_id,
    mh.title,
    mh.production_year,
    -1,
    'Nested Actor' || mh.level,
    mh.level + 1
  FROM movie_hierarchy AS mh
  WHERE
    mh.level < 5
), aggregated_movies AS (
  SELECT
    movie_id,
    title,
    production_year,
    COUNT(actor_id) AS num_actors,
    STRING_AGG(DISTINCT actor_name, ', ') AS actor_names
  FROM movie_hierarchy
  GROUP BY
    movie_id,
    title,
    production_year
), high_actor_movies AS (
  SELECT
    m.movie_id,
    m.title,
    m.production_year,
    m.num_actors,
    m.actor_names
  FROM aggregated_movies AS m
  WHERE
    m.num_actors > 5
), movies_with_keywords AS (
  SELECT
    m.movie_id,
    m.title,
    m.production_year,
    m.num_actors,
    m.actor_names,
    k.keyword
  FROM high_actor_movies AS m
  LEFT JOIN movie_keyword AS mk
    ON m.movie_id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
), final_output AS (
  SELECT
    mwk.movie_id,
    mwk.title,
    mwk.production_year,
    mwk.num_actors,
    mwk.actor_names,
    mwk.keyword,
    RANK() OVER (PARTITION BY mwk.production_year ORDER BY mwk.num_actors DESC) AS actor_rank
  FROM movies_with_keywords AS mwk
)
SELECT
  f.movie_id,
  f.title,
  f.production_year,
  f.num_actors,
  f.actor_names,
  COALESCE(f.keyword, 'No Keywords') AS keyword,
  f.actor_rank
FROM final_output AS f
WHERE
  f.actor_rank <= 10
ORDER BY
  f.production_year DESC,
  f.actor_rank ASC;
