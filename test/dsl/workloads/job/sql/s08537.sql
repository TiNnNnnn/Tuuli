WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    c.name AS company_name,
    k.keyword,
    ROW_NUMBER() OVER (PARTITION BY t.id ORDER BY t.production_year DESC) AS rank_order
  FROM title AS t
  JOIN movie_companies AS mc
    ON t.id = mc.movie_id
  JOIN company_name AS c
    ON mc.company_id = c.id
  JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  WHERE
    t.production_year >= 2000
), TopRankedMovies AS (
  SELECT
    movie_id,
    title,
    company_name,
    keyword
  FROM RankedMovies
  WHERE
    rank_order = 1
)
SELECT
  tr.movie_id,
  tr.title,
  tr.company_name,
  STRING_AGG(tr.keyword, ', ') AS keywords
FROM TopRankedMovies AS tr
GROUP BY
  tr.movie_id,
  tr.title,
  tr.company_name
ORDER BY
  tr.title ASC;
