WITH MovieDetails AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    COUNT(DISTINCT ci.person_id) AS cast_count,
    STRING_AGG(DISTINCT ak.name, ', ') AS aka_names,
    STRING_AGG(DISTINCT k.keyword, ', ') AS keywords
  FROM title AS t
  JOIN movie_info AS mi
    ON t.id = mi.movie_id
    AND mi.info_type_id = (
      SELECT
        id
      FROM info_type
      WHERE
        info = 'Plot'
    )
  JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  JOIN cast_info AS ci
    ON t.id = ci.movie_id
  LEFT JOIN aka_name AS ak
    ON ak.person_id = ci.person_id
  WHERE
    t.production_year >= 2000
  GROUP BY
    t.id,
    t.title,
    t.production_year
), CompanyDetails AS (
  SELECT
    mc.movie_id,
    COUNT(DISTINCT cn.id) AS company_count,
    STRING_AGG(DISTINCT cn.name, '; ') AS company_names
  FROM movie_companies AS mc
  JOIN company_name AS cn
    ON mc.company_id = cn.id
  GROUP BY
    mc.movie_id
)
SELECT
  md.movie_id,
  md.title,
  md.production_year,
  md.cast_count,
  cd.company_count,
  cd.company_names,
  md.aka_names,
  md.keywords
FROM MovieDetails AS md
LEFT JOIN CompanyDetails AS cd
  ON md.movie_id = cd.movie_id
ORDER BY
  md.production_year DESC,
  md.cast_count DESC;
