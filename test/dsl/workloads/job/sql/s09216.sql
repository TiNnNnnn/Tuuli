WITH MovieData AS (
  SELECT
    t.title AS movie_title,
    t.production_year,
    a.name AS actor_name,
    c.kind AS cast_type,
    t.id AS movie_id
  FROM aka_title AS t
  JOIN complete_cast AS cc
    ON t.id = cc.movie_id
  JOIN cast_info AS ci
    ON cc.subject_id = ci.id
  JOIN aka_name AS a
    ON ci.person_id = a.person_id
  JOIN comp_cast_type AS c
    ON ci.person_role_id = c.id
  WHERE
    t.production_year BETWEEN 2000 AND 2020
), KeywordData AS (
  SELECT
    mk.movie_id,
    k.keyword
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
), CompanyData AS (
  SELECT
    mc.movie_id,
    co.name AS company_name,
    ct.kind AS company_type
  FROM movie_companies AS mc
  JOIN company_name AS co
    ON mc.company_id = co.id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
)
SELECT
  md.movie_title,
  md.production_year,
  md.actor_name,
  md.cast_type,
  kd.keyword,
  cd.company_name,
  cd.company_type
FROM MovieData AS md
LEFT JOIN KeywordData AS kd
  ON md.movie_id = kd.movie_id
LEFT JOIN CompanyData AS cd
  ON md.movie_id = cd.movie_id
ORDER BY
  md.production_year DESC,
  md.movie_title;
