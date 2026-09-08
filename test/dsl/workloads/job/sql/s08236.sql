WITH RankedTitles AS (
  SELECT
    t.id AS title_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.id) AS rn
  FROM title AS t
  WHERE
    t.production_year BETWEEN 2000 AND 2020
), ActorMovies AS (
  SELECT
    ci.movie_id,
    a.name AS actor_name,
    COUNT(ci.person_id) AS actor_count
  FROM cast_info AS ci
  JOIN aka_name AS a
    ON a.person_id = ci.person_id
  WHERE
    ci.nr_order = 1
  GROUP BY
    ci.movie_id,
    a.name
), CompanyMovies AS (
  SELECT
    mc.movie_id,
    c.name AS company_name,
    ct.kind AS company_type
  FROM movie_companies AS mc
  JOIN company_name AS c
    ON c.id = mc.company_id
  JOIN company_type AS ct
    ON ct.id = mc.company_type_id
  WHERE
    c.country_code = 'USA'
), MoviesWithKeywords AS (
  SELECT
    mk.movie_id,
    k.keyword
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON k.id = mk.keyword_id
  WHERE
    NOT k.phonetic_code IS NULL
)
SELECT
  rt.title,
  rt.production_year,
  am.actor_name,
  cm.company_name,
  cm.company_type,
  mk.keyword
FROM RankedTitles AS rt
LEFT JOIN ActorMovies AS am
  ON am.movie_id = rt.title_id
LEFT JOIN CompanyMovies AS cm
  ON cm.movie_id = rt.title_id
LEFT JOIN MoviesWithKeywords AS mk
  ON mk.movie_id = rt.title_id
WHERE
  rt.rn <= 5
ORDER BY
  rt.production_year DESC,
  rt.title;
