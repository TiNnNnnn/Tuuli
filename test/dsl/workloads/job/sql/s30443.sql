WITH RECURSIVE MovieHierarchy AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    ARRAY[m.id] AS path_ids
  FROM aka_title AS m
  WHERE
    m.production_year >= 2000
  UNION ALL
  SELECT
    linked.linked_movie_id,
    linked_movie.title,
    linked_movie.production_year,
    path_ids || linked.linked_movie_id
  FROM movie_link AS linked
  JOIN aka_title AS linked_movie
    ON linked.linked_movie_id = linked_movie.id
  JOIN MovieHierarchy AS mh
    ON mh.movie_id = linked.movie_id
  WHERE
    linked_movie.production_year >= 2000
    AND NOT linked.linked_movie_id = ANY(
      mh.path_ids
    )
), ActorRole AS (
  SELECT
    ci.movie_id,
    ak.name AS actor_name,
    rt.role
  FROM cast_info AS ci
  JOIN aka_name AS ak
    ON ci.person_id = ak.person_id
  JOIN role_type AS rt
    ON ci.role_id = rt.id
), FullMovieInfo AS (
  SELECT
    mh.movie_id,
    mh.title,
    mh.production_year,
    ARRAY_AGG(DISTINCT ar.actor_name) AS actors,
    COUNT(DISTINCT ar.actor_name) AS actor_count
  FROM MovieHierarchy AS mh
  LEFT JOIN ActorRole AS ar
    ON mh.movie_id = ar.movie_id
  GROUP BY
    mh.movie_id,
    mh.title,
    mh.production_year
)
SELECT
  fmi.title,
  fmi.production_year,
  fmi.actor_count,
  COALESCE(fmi.actors[1], 'No Actors') AS first_actor,
  CASE
    WHEN fmi.actor_count > 5
    THEN 'Ensemble Cast'
    WHEN fmi.actor_count > 0
    THEN 'Small Cast'
    ELSE 'No Cast'
  END AS cast_size
FROM FullMovieInfo AS fmi
ORDER BY
  fmi.production_year DESC,
  fmi.actor_count DESC;
