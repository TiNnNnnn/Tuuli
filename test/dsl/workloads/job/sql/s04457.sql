WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.title) AS rn
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
), ActorRoles AS (
  SELECT
    c.movie_id,
    r.role,
    COUNT(c.id) AS role_count
  FROM cast_info AS c
  JOIN role_type AS r
    ON c.role_id = r.id
  GROUP BY
    c.movie_id,
    r.role
), MovieKeywords AS (
  SELECT
    mk.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    mk.movie_id
)
SELECT
  rm.movie_id,
  rm.title,
  rm.production_year,
  COALESCE(ar.role, 'No roles') AS role,
  COALESCE(mk.keywords, 'No keywords') AS keywords,
  CASE WHEN rm.production_year >= 2000 THEN 'Modern' ELSE 'Classic' END AS movie_type,
  COUNT(DISTINCT c.person_id) AS actor_count
FROM RankedMovies AS rm
LEFT JOIN ActorRoles AS ar
  ON rm.movie_id = ar.movie_id
LEFT JOIN MovieKeywords AS mk
  ON rm.movie_id = mk.movie_id
LEFT JOIN cast_info AS c
  ON rm.movie_id = c.movie_id
GROUP BY
  rm.movie_id,
  rm.title,
  rm.production_year,
  ar.role,
  mk.keywords
ORDER BY
  rm.production_year DESC,
  rm.title;
