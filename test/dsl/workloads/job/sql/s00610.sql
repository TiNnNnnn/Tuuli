WITH MovieData AS (
  SELECT
    t.title AS movie_title,
    t.production_year,
    COUNT(DISTINCT c.person_id) AS actor_count,
    STRING_AGG(DISTINCT a.name, ', ') AS actors
  FROM aka_title AS t
  LEFT JOIN movie_info AS mi
    ON t.id = mi.movie_id
  LEFT JOIN complete_cast AS cc
    ON t.id = cc.movie_id
  LEFT JOIN cast_info AS c
    ON cc.subject_id = c.id
  LEFT JOIN aka_name AS a
    ON c.person_id = a.person_id
  WHERE
    t.production_year >= 2000
  GROUP BY
    t.id,
    t.title,
    t.production_year
), KeywordData AS (
  SELECT
    t.id AS movie_id,
    k.keyword
  FROM aka_title AS t
  JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  WHERE
    NOT k.keyword IS NULL
), RankedMovies AS (
  SELECT
    md.movie_title,
    md.production_year,
    md.actor_count,
    md.actors,
    ROW_NUMBER() OVER (PARTITION BY md.production_year ORDER BY md.actor_count DESC) AS rank
  FROM MovieData AS md
)
SELECT
  rm.movie_title,
  rm.production_year,
  rm.actor_count,
  rm.actors,
  COALESCE(kd.keyword, 'No Keywords') AS keywords
FROM RankedMovies AS rm
LEFT JOIN KeywordData AS kd
  ON rm.movie_title = (
    SELECT
      title
    FROM aka_title
    WHERE
      id IN (
        SELECT
          movie_id
        FROM movie_info
        WHERE
          info_type_id = 1
      )
    LIMIT 1
  )
WHERE
  rm.rank <= 5
ORDER BY
  rm.production_year DESC,
  rm.actor_count DESC;
