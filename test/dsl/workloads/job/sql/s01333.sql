WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.title) AS title_rank
  FROM aka_title AS t
  WHERE
    t.kind_id = (
      SELECT
        id
      FROM kind_type
      WHERE
        kind = 'movie'
    )
), FilteredCast AS (
  SELECT
    c.movie_id,
    COUNT(c.person_id) AS cast_count,
    STRING_AGG(a.name, ', ') AS cast_names
  FROM cast_info AS c
  JOIN aka_name AS a
    ON c.person_id = a.person_id
  WHERE
    c.nr_order < 10
  GROUP BY
    c.movie_id
), MovieCompanies AS (
  SELECT
    mc.movie_id,
    COUNT(DISTINCT mc.company_id) AS company_count
  FROM movie_companies AS mc
  JOIN company_name AS cn
    ON mc.company_id = cn.id
  WHERE
    NOT cn.country_code IS NULL
  GROUP BY
    mc.movie_id
)
SELECT
  rm.movie_id,
  rm.title,
  rm.production_year,
  COALESCE(fc.cast_count, 0) AS total_cast,
  COALESCE(fc.cast_names, 'No cast information available') AS cast_members,
  COALESCE(mc.company_count, 0) AS total_companies
FROM RankedMovies AS rm
LEFT JOIN FilteredCast AS fc
  ON rm.movie_id = fc.movie_id
LEFT JOIN MovieCompanies AS mc
  ON rm.movie_id = mc.movie_id
WHERE
  rm.production_year BETWEEN 2000 AND 2020
ORDER BY
  rm.production_year DESC,
  rm.title_rank;
