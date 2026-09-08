WITH MovieActors AS (
  SELECT
    m.id AS movie_id,
    m.title,
    STRING_AGG(DISTINCT a.name, ', ') AS actors,
    COUNT(DISTINCT mc.company_id) AS company_count
  FROM aka_title AS m
  LEFT JOIN cast_info AS ci
    ON ci.movie_id = m.id
  LEFT JOIN aka_name AS a
    ON a.person_id = ci.person_id
  LEFT JOIN movie_companies AS mc
    ON mc.movie_id = m.id
  WHERE
    m.production_year >= 2000
  GROUP BY
    m.id,
    m.title
), ActorRankings AS (
  SELECT
    movie_id,
    RANK() OVER (PARTITION BY movie_id ORDER BY COUNT(DISTINCT a.person_id) DESC) AS actor_rank
  FROM cast_info AS ci
  JOIN aka_name AS a
    ON a.person_id = ci.person_id
  GROUP BY
    movie_id
), MovieInfo AS (
  SELECT
    m.id AS movie_id,
    COALESCE(mi.info, 'No info available') AS movie_info
  FROM aka_title AS m
  LEFT JOIN movie_info AS mi
    ON mi.movie_id = m.id
    AND mi.info_type_id = (
      SELECT
        id
      FROM info_type
      WHERE
        info = 'summary'
    )
)
SELECT
  ma.movie_id,
  ma.title,
  ma.actors,
  ma.company_count,
  COALESCE(ai.movie_info, 'No additional information') AS additional_info,
  ar.actor_rank
FROM MovieActors AS ma
INNER JOIN MovieInfo AS ai
  ON ma.movie_id = ai.movie_id
LEFT JOIN ActorRankings AS ar
  ON ma.movie_id = ar.movie_id
WHERE
  ma.company_count > 1
ORDER BY
  ar.actor_rank,
  ma.title;
