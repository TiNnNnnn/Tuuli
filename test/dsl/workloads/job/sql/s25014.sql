WITH ActorMovies AS (
  SELECT
    a.id AS actor_id,
    a.name AS actor_name,
    STRING_AGG(DISTINCT t.title, ', ') AS movies,
    COUNT(t.id) AS movie_count
  FROM aka_name AS a
  JOIN cast_info AS c
    ON a.person_id = c.person_id
  JOIN aka_title AS t
    ON c.movie_id = t.movie_id
  WHERE
    NOT a.name IS NULL
  GROUP BY
    a.id,
    a.name
), MovieGenres AS (
  SELECT
    t.id AS movie_id,
    t.title AS movie_title,
    k.keyword AS genre
  FROM aka_title AS t
  JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
), ActorGenreCount AS (
  SELECT
    am.actor_id,
    am.actor_name,
    COUNT(DISTINCT mg.genre) AS distinct_genre_count
  FROM ActorMovies AS am
  JOIN cast_info AS c
    ON am.actor_id = c.person_id
  JOIN MovieGenres AS mg
    ON c.movie_id = mg.movie_id
  GROUP BY
    am.actor_id,
    am.actor_name
)
SELECT
  agc.actor_id,
  agc.actor_name,
  am.movie_count,
  agc.distinct_genre_count,
  (
    SELECT
      COUNT(*)
    FROM aka_title
  ) AS total_movies_in_db
FROM ActorGenreCount AS agc
JOIN ActorMovies AS am
  ON agc.actor_id = am.actor_id
ORDER BY
  agc.distinct_genre_count DESC,
  am.movie_count DESC
LIMIT 10;
