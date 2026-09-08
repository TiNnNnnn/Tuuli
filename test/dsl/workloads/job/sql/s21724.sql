WITH RECURSIVE RelatedMovies AS (
  SELECT
    m.movie_id,
    m.title,
    m.production_year,
    1 AS recursion_level
  FROM aka_title AS m
  WHERE
    m.kind_id = (
      SELECT
        id
      FROM kind_type
      WHERE
        kind = 'movie'
    )
  UNION ALL
  SELECT
    mm.movie_id,
    mm.title,
    mm.production_year,
    r.recursion_level + 1
  FROM movie_link AS ml
  JOIN aka_title AS mm
    ON ml.linked_movie_id = mm.id
  JOIN RelatedMovies AS r
    ON ml.movie_id = r.movie_id
  WHERE
    r.recursion_level < 5
), PersonMovies AS (
  SELECT
    ca.person_id,
    ca.movie_id,
    at.title,
    at.production_year,
    ROW_NUMBER() OVER (PARTITION BY ca.person_id ORDER BY at.production_year DESC) AS role_rank
  FROM cast_info AS ca
  JOIN aka_title AS at
    ON ca.movie_id = at.id
  WHERE
    NOT at.production_year IS NULL
), TopActors AS (
  SELECT
    person_id,
    COUNT(DISTINCT movie_id) AS movie_count
  FROM PersonMovies
  WHERE
    role_rank <= 5
  GROUP BY
    person_id
  HAVING
    COUNT(DISTINCT movie_id) > 2
), ActorInfo AS (
  SELECT
    ak.name,
    ta.movie_count
  FROM aka_name AS ak
  JOIN TopActors AS ta
    ON ak.person_id = ta.person_id
  WHERE
    NOT ak.name IS NULL
), MovieDetails AS (
  SELECT
    rt.kind,
    COUNT(DISTINCT m.id) AS movie_count
  FROM aka_title AS m
  JOIN kind_type AS rt
    ON m.kind_id = rt.id
  GROUP BY
    rt.kind
), FinalBenchmark AS (
  SELECT
    ai.name AS actor_name,
    ai.movie_count,
    md.kind AS movie_kind,
    COALESCE(md.movie_count, 0) AS total_movies,
    (
      CAST(ai.movie_count AS DECIMAL) / NULLIF(md.movie_count, 0)
    ) * 100 AS performance_percentage
  FROM ActorInfo AS ai
  LEFT JOIN MovieDetails AS md
    ON 1 = 1
)
SELECT
  *
FROM FinalBenchmark
ORDER BY
  performance_percentage DESC,
  actor_name
LIMIT 10;
