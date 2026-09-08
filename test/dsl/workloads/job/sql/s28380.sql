WITH ActorMovies AS (
  SELECT
    a.id AS actor_id,
    a.name AS actor_name,
    m.title AS movie_title,
    m.production_year,
    r.role AS actor_role
  FROM aka_name AS a
  JOIN cast_info AS ci
    ON a.person_id = ci.person_id
  JOIN title AS m
    ON ci.movie_id = m.id
  JOIN role_type AS r
    ON ci.role_id = r.id
), MovieKeywords AS (
  SELECT
    m.id AS movie_id,
    k.keyword AS movie_keyword
  FROM title AS m
  JOIN movie_keyword AS mk
    ON m.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
), ActorKeywords AS (
  SELECT
    am.actor_id,
    am.actor_name,
    am.movie_title,
    am.production_year,
    STRING_AGG(mk.movie_keyword, ', ') AS keywords
  FROM ActorMovies AS am
  LEFT JOIN MovieKeywords AS mk
    ON am.movie_title = mk.movie_keyword
  GROUP BY
    am.actor_id,
    am.actor_name,
    am.movie_title,
    am.production_year
)
SELECT
  ak.actor_id,
  ak.actor_name,
  ak.movie_title,
  ak.production_year,
  ak.keywords
FROM ActorKeywords AS ak
WHERE
  NOT ak.keywords IS NULL
ORDER BY
  ak.production_year DESC,
  ak.actor_name;
