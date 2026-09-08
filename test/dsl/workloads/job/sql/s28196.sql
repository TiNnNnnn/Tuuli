WITH MovieDetails AS (
  SELECT
    a.title AS movie_title,
    a.production_year,
    c.name AS company_name,
    rt.role AS actor_role,
    ak.name AS actor_name
  FROM aka_title AS a
  JOIN complete_cast AS cc
    ON a.id = cc.movie_id
  JOIN cast_info AS ci
    ON cc.subject_id = ci.id
  JOIN aka_name AS ak
    ON ci.person_id = ak.person_id
  JOIN movie_companies AS mc
    ON a.id = mc.movie_id
  JOIN company_name AS c
    ON mc.company_id = c.id
  JOIN role_type AS rt
    ON ci.role_id = rt.id
  WHERE
    a.production_year BETWEEN 2000 AND 2020
    AND NOT ak.name IS NULL
    AND ci.note IS NULL
), ActorCounts AS (
  SELECT
    actor_name,
    COUNT(movie_title) AS movie_count
  FROM MovieDetails
  GROUP BY
    actor_name
), RankedActors AS (
  SELECT
    actor_name,
    movie_count,
    RANK() OVER (ORDER BY movie_count DESC) AS rank
  FROM ActorCounts
)
SELECT
  ra.actor_name,
  ra.movie_count,
  md.movie_title,
  md.production_year,
  md.company_name,
  md.actor_role
FROM RankedActors AS ra
JOIN MovieDetails AS md
  ON ra.actor_name = md.actor_name
WHERE
  ra.rank <= 10
ORDER BY
  ra.rank,
  md.production_year DESC;
