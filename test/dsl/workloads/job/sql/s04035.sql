WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.id) AS rn
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
), MoviesWithKeywords AS (
  SELECT
    rm.movie_id,
    rm.title,
    rm.production_year,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM RankedMovies AS rm
  LEFT JOIN movie_keyword AS mk
    ON rm.movie_id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    rm.movie_id,
    rm.title,
    rm.production_year
), ActorRoles AS (
  SELECT
    c.movie_id,
    COUNT(DISTINCT c.person_id) AS actor_count,
    STRING_AGG(DISTINCT r.role, ', ') AS roles
  FROM cast_info AS c
  JOIN role_type AS r
    ON c.role_id = r.id
  GROUP BY
    c.movie_id
)
SELECT
  mwk.title,
  mwk.production_year,
  mwk.keywords,
  ar.actor_count,
  ar.roles,
  COALESCE(COUNT(m.id), 0) AS linked_movie_count
FROM MoviesWithKeywords AS mwk
LEFT JOIN ActorRoles AS ar
  ON mwk.movie_id = ar.movie_id
LEFT JOIN movie_link AS m
  ON mwk.movie_id = m.movie_id
GROUP BY
  mwk.movie_id,
  mwk.title,
  mwk.production_year,
  mwk.keywords,
  ar.actor_count,
  ar.roles
HAVING
  MWK.production_year >= 2000
  AND (
    ar.actor_count IS NULL OR ar.actor_count > 5
  )
ORDER BY
  mwk.production_year DESC,
  mwk.title;
