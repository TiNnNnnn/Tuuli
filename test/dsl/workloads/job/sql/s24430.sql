WITH RecursiveRatings AS (
  SELECT
    ti.id AS title_id,
    ti.title,
    ti.production_year,
    COUNT(DISTINCT ci.person_id) AS cast_count,
    RANK() OVER (PARTITION BY ti.production_year ORDER BY COUNT(DISTINCT ci.person_id) DESC) AS yearly_rank
  FROM aka_title AS ti
  LEFT JOIN cast_info AS ci
    ON ci.movie_id = ti.movie_id
  WHERE
    ti.kind_id IN (
      SELECT
        id
      FROM kind_type
      WHERE
        kind LIKE 'movie%'
    )
  GROUP BY
    ti.id,
    ti.title,
    ti.production_year
), CompanyContributions AS (
  SELECT
    mc.movie_id,
    COALESCE(SUM(CASE WHEN ct.kind LIKE 'Production%' THEN 1 ELSE 0 END), 0) AS production_company_count,
    COUNT(DISTINCT mc.company_id) AS total_companies
  FROM movie_companies AS mc
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
  WHERE
    mc.movie_id IN (
      SELECT
        movie_id
      FROM complete_cast
    )
  GROUP BY
    mc.movie_id
), RankedMovies AS (
  SELECT
    rr.title_id,
    rr.title,
    rr.production_year,
    rr.cast_count,
    cc.production_company_count,
    cc.total_companies,
    rr.yearly_rank
  FROM RecursiveRatings AS rr
  LEFT JOIN CompanyContributions AS cc
    ON rr.title_id = cc.movie_id
)
SELECT
  rm.title,
  rm.production_year,
  rm.cast_count,
  rm.production_company_count,
  rm.total_companies,
  CASE
    WHEN rm.yearly_rank IS NULL
    THEN 'Unranked'
    WHEN rm.yearly_rank > 10
    THEN 'Not Notable'
    ELSE 'Noteworthy'
  END AS rank_description
FROM RankedMovies AS rm
WHERE
  NOT rm.production_year IS NULL
  AND rm.cast_count > (
    SELECT
      AVG(cast_count)
    FROM RecursiveRatings
    WHERE
      production_year = rm.production_year
  )
ORDER BY
  rm.production_year DESC,
  rm.cast_count DESC;
