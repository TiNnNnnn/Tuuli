WITH RecursiveMovieInfo AS (
  SELECT
    mt.id AS movie_id,
    mt.title AS movie_title,
    COALESCE(mt.production_year, 0) AS production_year,
    COALESCE(k.keyword, 'No Keywords') AS keyword,
    COALESCE(a.name, 'Unknown Actor') AS actor_name,
    ROW_NUMBER() OVER (PARTITION BY mt.id ORDER BY COALESCE(k.keyword, 'No Keywords')) AS keyword_rank
  FROM aka_title AS mt
  LEFT JOIN movie_keyword AS mk
    ON mt.id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  LEFT JOIN cast_info AS ci
    ON mt.id = ci.movie_id
  LEFT JOIN aka_name AS a
    ON ci.person_id = a.person_id
), AggregatedProductionYears AS (
  SELECT
    mm.movie_title,
    MAX(mm.production_year) AS max_production_year,
    COUNT(DISTINCT mm.actor_name) AS actor_count
  FROM RecursiveMovieInfo AS mm
  GROUP BY
    mm.movie_title
), TitleWithKeywordCounts AS (
  SELECT
    m.movie_title,
    COUNT(DISTINCT k.keyword) AS unique_keyword_count
  FROM RecursiveMovieInfo AS m
  JOIN keyword AS k
    ON m.keyword = k.keyword
  GROUP BY
    m.movie_title
)
SELECT
  ap.movie_title,
  ap.max_production_year,
  ap.actor_count,
  COALESCE(t.unique_keyword_count, 0) AS unique_keyword_count,
  CASE
    WHEN ap.max_production_year < 2000
    THEN 'Classic Era'
    WHEN ap.max_production_year BETWEEN 2000 AND 2010
    THEN 'Early 21st Century'
    ELSE 'Recent Releases'
  END AS era_category
FROM AggregatedProductionYears AS ap
LEFT JOIN TitleWithKeywordCounts AS t
  ON ap.movie_title = t.movie_title
WHERE
  ap.actor_count > 1
ORDER BY
  ap.max_production_year DESC,
  ap.movie_title ASC
FETCH FIRST 50 ROWS ONLY;
