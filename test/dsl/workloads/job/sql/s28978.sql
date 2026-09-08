WITH movie_details AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    t.kind_id,
    STRING_AGG(DISTINCT ak.name, ', ') AS aka_names,
    STRING_AGG(DISTINCT k.keyword, ', ') AS keywords,
    STRING_AGG(DISTINCT cn.name, ', ') AS companies
  FROM aka_title AS t
  LEFT JOIN aka_name AS ak
    ON t.id = ak.id
  LEFT JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  LEFT JOIN movie_companies AS mc
    ON t.id = mc.movie_id
  LEFT JOIN company_name AS cn
    ON mc.company_id = cn.id
  GROUP BY
    t.id,
    t.title,
    t.production_year,
    t.kind_id
), cast_details AS (
  SELECT
    m.id AS movie_id,
    STRING_AGG(DISTINCT p.name, ', ') AS cast_names,
    STRING_AGG(DISTINCT rt.role, ', ') AS roles,
    COUNT(ci.person_id) AS cast_count
  FROM complete_cast AS c
  JOIN cast_info AS ci
    ON c.movie_id = ci.movie_id
  JOIN aka_name AS p
    ON ci.person_id = p.person_id
  JOIN role_type AS rt
    ON ci.role_id = rt.id
  JOIN title AS m
    ON c.movie_id = m.id
  GROUP BY
    m.id
)
SELECT
  md.movie_id,
  md.title,
  md.production_year,
  md.kind_id,
  md.aka_names,
  md.keywords,
  cd.cast_names,
  cd.roles,
  cd.cast_count
FROM movie_details AS md
LEFT JOIN cast_details AS cd
  ON md.movie_id = cd.movie_id
WHERE
  md.production_year >= 2000
ORDER BY
  md.production_year DESC,
  md.title ASC;
