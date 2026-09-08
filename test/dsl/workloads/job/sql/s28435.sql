WITH MovieTitles AS (
  SELECT
    t.id AS title_id,
    t.title,
    t.production_year
  FROM aka_title AS t
  WHERE
    t.production_year >= 2000
  ORDER BY
    t.production_year DESC
  LIMIT 100
), CastInfo AS (
  SELECT
    ci.movie_id,
    c.name AS actor_name,
    ci.role_id
  FROM cast_info AS ci
  JOIN aka_name AS c
    ON ci.person_id = c.person_id
), MovieCompanies AS (
  SELECT
    mc.movie_id,
    co.name AS company_name,
    ct.kind AS company_type
  FROM movie_companies AS mc
  JOIN company_name AS co
    ON mc.company_id = co.id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
), MovieKeywords AS (
  SELECT
    mk.movie_id,
    k.keyword
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
), CombinedInfo AS (
  SELECT
    mt.title,
    mt.production_year,
    ci.actor_name,
    mc.company_name,
    mc.company_type,
    mk.keyword
  FROM MovieTitles AS mt
  LEFT JOIN CastInfo AS ci
    ON mt.title_id = ci.movie_id
  LEFT JOIN MovieCompanies AS mc
    ON mt.title_id = mc.movie_id
  LEFT JOIN MovieKeywords AS mk
    ON mt.title_id = mk.movie_id
)
SELECT
  title,
  production_year,
  STRING_AGG(DISTINCT actor_name, ', ') AS actors,
  STRING_AGG(DISTINCT company_name || ' (' || company_type || ')', ', ') AS companies,
  STRING_AGG(DISTINCT keyword, ', ') AS keywords
FROM CombinedInfo
GROUP BY
  title,
  production_year
ORDER BY
  production_year DESC;
