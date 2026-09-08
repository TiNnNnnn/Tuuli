WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.production_year DESC) AS rank_year
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
), ActorRoles AS (
  SELECT
    ci.movie_id,
    a.name AS actor_name,
    rt.role,
    ci.nr_order
  FROM cast_info AS ci
  JOIN aka_name AS a
    ON ci.person_id = a.person_id
  LEFT JOIN role_type AS rt
    ON ci.role_id = rt.id
), KeyWords AS (
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
  ar.actor_name,
  ar.role,
  ar.nr_order,
  kw.keywords
FROM RankedMovies AS rm
LEFT JOIN ActorRoles AS ar
  ON rm.movie_id = ar.movie_id
LEFT JOIN KeyWords AS kw
  ON rm.movie_id = kw.movie_id
WHERE
  rm.rank_year <= 10 AND (
    NOT ar.role IS NULL OR NOT kw.keywords IS NULL
  )
ORDER BY
  rm.production_year DESC,
  ar.nr_order ASC,
  ar.actor_name;
