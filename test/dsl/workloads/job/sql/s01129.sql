WITH RankedMovies AS (
  SELECT
    a.id AS movie_id,
    a.title,
    a.production_year,
    ROW_NUMBER() OVER (PARTITION BY a.production_year ORDER BY a.title) AS title_rank,
    COUNT(DISTINCT c.person_id) AS actor_count
  FROM aka_title AS a
  LEFT JOIN cast_info AS c
    ON a.id = c.movie_id
  GROUP BY
    a.id,
    a.title,
    a.production_year
), ActorRankings AS (
  SELECT
    p.person_id,
    p.name,
    RANK() OVER (ORDER BY COUNT(DISTINCT ci.movie_id) DESC) AS rank,
    CASE WHEN COUNT(DISTINCT ci.movie_id) = 0 THEN 'No movies' ELSE 'Active' END AS activity_status
  FROM aka_name AS p
  LEFT JOIN cast_info AS ci
    ON p.person_id = ci.person_id
  GROUP BY
    p.person_id,
    p.name
), MovieKeywordCounts AS (
  SELECT
    mk.movie_id,
    COUNT(DISTINCT k.keyword) AS keyword_count
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    mk.movie_id
), Summary AS (
  SELECT
    rm.title,
    rm.production_year,
    ak.name AS top_actor,
    ak.rank AS actor_rank,
    COALESCE(mkc.keyword_count, 0) AS num_keywords,
    rm.actor_count
  FROM RankedMovies AS rm
  JOIN ActorRankings AS ak
    ON rm.actor_count = ak.rank
  LEFT JOIN MovieKeywordCounts AS mkc
    ON rm.movie_id = mkc.movie_id
)
SELECT
  title,
  production_year,
  top_actor,
  actor_rank,
  num_keywords,
  actor_count
FROM Summary
WHERE
  production_year > 2000
ORDER BY
  actor_rank,
  production_year DESC
LIMIT 10;
