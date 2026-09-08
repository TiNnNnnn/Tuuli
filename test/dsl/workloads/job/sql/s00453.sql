WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.production_year DESC) AS rank,
    ct.kind AS company_type,
    COUNT(DISTINCT kc.keyword) AS keyword_count
  FROM aka_title AS t
  LEFT JOIN movie_companies AS mc
    ON t.id = mc.movie_id
  LEFT JOIN company_type AS ct
    ON mc.company_type_id = ct.id
  LEFT JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  LEFT JOIN keyword AS kc
    ON mk.keyword_id = kc.id
  WHERE
    NOT t.production_year IS NULL
  GROUP BY
    t.id,
    t.title,
    t.production_year,
    ct.kind
), SubqueryMovies AS (
  SELECT
    rm.movie_id,
    rm.title,
    rm.production_year,
    rm.rank,
    COALESCE(SUM(CASE WHEN NOT cc.role_id IS NULL THEN 1 ELSE 0 END), 0) AS cast_count,
    STRING_AGG(DISTINCT ak.name, ', ') AS actor_names
  FROM RankedMovies AS rm
  LEFT JOIN cast_info AS cc
    ON rm.movie_id = cc.movie_id
  LEFT JOIN aka_name AS ak
    ON cc.person_id = ak.person_id
  GROUP BY
    rm.movie_id,
    rm.title,
    rm.production_year,
    rm.rank
)
SELECT
  sm.movie_id,
  sm.title,
  sm.production_year,
  sm.rank,
  sm.cast_count,
  sm.actor_names,
  CASE
    WHEN sm.rank <= 5
    THEN 'Top 5 Movies'
    WHEN sm.rank > 5 AND sm.rank <= 10
    THEN 'Next 5 Movies'
    ELSE 'Beyond Top 10'
  END AS movie_category
FROM SubqueryMovies AS sm
WHERE
  sm.cast_count > 0
ORDER BY
  sm.production_year DESC,
  sm.rank;
