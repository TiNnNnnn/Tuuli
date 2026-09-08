WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.id) AS rn
  FROM aka_title AS t
  JOIN movie_info AS mi
    ON t.id = mi.movie_id
  WHERE
    mi.info_type_id = (
      SELECT
        id
      FROM info_type
      WHERE
        info = 'duration'
    )
    AND NOT mi.info IS NULL
), ActorRoles AS (
  SELECT
    ci.movie_id,
    ak.name AS actor_name,
    rt.role AS role
  FROM cast_info AS ci
  JOIN aka_name AS ak
    ON ci.person_id = ak.person_id
  JOIN role_type AS rt
    ON ci.role_id = rt.id
), MoviesWithKeywords AS (
  SELECT
    mk.movie_id,
    k.keyword
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
)
SELECT
  rm.movie_id,
  rm.title,
  rm.production_year,
  ar.actor_name,
  ar.role,
  STRING_AGG(mk.keyword, ', ') AS keywords
FROM RankedMovies AS rm
LEFT JOIN ActorRoles AS ar
  ON rm.movie_id = ar.movie_id
LEFT JOIN MoviesWithKeywords AS mk
  ON rm.movie_id = mk.movie_id
WHERE
  rm.rn <= 10
GROUP BY
  rm.movie_id,
  rm.title,
  rm.production_year,
  ar.actor_name,
  ar.role
ORDER BY
  rm.production_year DESC,
  rm.title;
