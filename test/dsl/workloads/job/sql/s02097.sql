WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.title) AS title_rank
  FROM aka_title AS t
  WHERE
    t.kind_id IN (
      SELECT
        id
      FROM kind_type
      WHERE
        kind = 'movie'
    )
    AND NOT t.production_year IS NULL
), MovieActors AS (
  SELECT
    c.movie_id,
    ak.name AS actor_name,
    ROW_NUMBER() OVER (PARTITION BY c.movie_id ORDER BY ak.name) AS actor_rank
  FROM cast_info AS c
  JOIN aka_name AS ak
    ON c.person_id = ak.person_id
  WHERE
    c.role_id IN (
      SELECT
        id
      FROM role_type
      WHERE
        role = 'actor'
    )
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
  ma.actor_name,
  COALESCE(mk.keywords, 'No Keywords') AS keywords,
  COUNT(mk.keywords) OVER (PARTITION BY rm.movie_id) AS keyword_count
FROM RankedMovies AS rm
LEFT JOIN MovieActors AS ma
  ON rm.movie_id = ma.movie_id AND ma.actor_rank <= 3
LEFT JOIN MovieKeywords AS mk
  ON rm.movie_id = mk.movie_id
WHERE
  rm.title_rank <= 5
ORDER BY
  rm.production_year DESC,
  rm.title ASC;
