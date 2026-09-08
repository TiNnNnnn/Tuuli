WITH RECURSIVE MovieHierarchy AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    COALESCE(a.name, 'Unknown') AS actor_name,
    CAST(COALESCE(c.role_id, 0) AS INT) AS role_id,
    ROW_NUMBER() OVER (PARTITION BY m.id ORDER BY CASE WHEN NOT a.name IS NULL THEN 1 ELSE 0 END DESC) AS role_order
  FROM aka_title AS m
  LEFT JOIN cast_info AS c
    ON m.id = c.movie_id
  LEFT JOIN aka_name AS a
    ON c.person_id = a.person_id
), RankedMovies AS (
  SELECT
    movie_id,
    title,
    production_year,
    actor_name,
    role_id,
    role_order,
    COUNT(*) OVER (PARTITION BY production_year) AS movies_in_year
  FROM MovieHierarchy
), FilteredMovies AS (
  SELECT
    movie_id,
    title,
    production_year,
    actor_name,
    role_id,
    role_order,
    movies_in_year,
    CASE
      WHEN role_order = 1
      THEN 'Lead'
      WHEN role_order > 1 AND role_order <= 3
      THEN 'Supporting'
      ELSE 'Minor'
    END AS role_type
  FROM RankedMovies
  WHERE
    production_year > 2000
    AND (
      NOT actor_name IS NULL AND actor_name <> 'Unknown'
    )
)
SELECT
  f.movie_id,
  f.title,
  f.production_year,
  f.actor_name,
  f.role_type,
  f.movies_in_year,
  COALESCE(SUM(mk.id), 0) AS keyword_count
FROM FilteredMovies AS f
LEFT JOIN movie_keyword AS mk
  ON f.movie_id = mk.movie_id
GROUP BY
  f.movie_id,
  f.title,
  f.production_year,
  f.actor_name,
  f.role_type,
  f.movies_in_year
ORDER BY
  f.production_year DESC,
  f.role_type DESC,
  keyword_count DESC
LIMIT 10;
