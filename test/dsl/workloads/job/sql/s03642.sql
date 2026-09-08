WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY COUNT(DISTINCT ci.person_id) DESC) AS rank
  FROM aka_title AS t
  LEFT JOIN cast_info AS ci
    ON t.id = ci.movie_id
  GROUP BY
    t.id,
    t.title,
    t.production_year
), CompanyStats AS (
  SELECT
    mc.movie_id,
    cn.name AS company_name,
    ct.kind AS company_type,
    COUNT(mc.id) AS total_companies
  FROM movie_companies AS mc
  JOIN company_name AS cn
    ON mc.company_id = cn.id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
  GROUP BY
    mc.movie_id,
    cn.name,
    ct.kind
)
SELECT
  rm.title AS movie_title,
  rm.production_year,
  cs.company_name,
  cs.company_type,
  COALESCE(cs.total_companies, 0) AS total_companies,
  (
    SELECT
      COUNT(*)
    FROM movie_keyword AS mk
    WHERE
      mk.movie_id = rm.movie_id
  ) AS keyword_count,
  (
    SELECT
      STRING_AGG(k.keyword, ', ')
    FROM movie_keyword AS mk
    JOIN keyword AS k
      ON mk.keyword_id = k.id
    WHERE
      mk.movie_id = rm.movie_id
  ) AS keywords
FROM RankedMovies AS rm
LEFT JOIN CompanyStats AS cs
  ON rm.movie_id = cs.movie_id
WHERE
  rm.rank <= 5
ORDER BY
  rm.production_year DESC,
  total_companies DESC NULLS LAST;
