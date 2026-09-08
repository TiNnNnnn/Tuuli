WITH MovieDetails AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    STRING_AGG(c.character_name, ', ') AS cast_names,
    STRING_AGG(DISTINCT k.keyword, ', ') AS keywords
  FROM title AS m
  JOIN complete_cast AS cc
    ON m.id = cc.movie_id
  JOIN cast_info AS ci
    ON cc.subject_id = ci.person_id
  JOIN aka_name AS a
    ON ci.person_id = a.person_id
  JOIN keyword AS k
    ON m.id = k.id
  LEFT JOIN (
    SELECT
      ci.movie_id,
      STRING_AGG(DISTINCT a.name, ', ') AS character_name
    FROM cast_info AS ci
    JOIN aka_name AS a
      ON ci.person_id = a.person_id
    GROUP BY
      ci.movie_id
  ) AS c
    ON c.movie_id = m.id
  WHERE
    m.production_year > 2000
  GROUP BY
    m.id,
    m.title,
    m.production_year
), CompanyDetails AS (
  SELECT
    mc.movie_id,
    STRING_AGG(DISTINCT cn.name, ', ') AS companies_involved
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
  md.cast_names,
  md.keywords,
  cd.companies_involved
FROM MovieDetails AS md
LEFT JOIN CompanyDetails AS cd
  ON md.movie_id = cd.movie_id
ORDER BY
  md.production_year DESC,
  md.title;
