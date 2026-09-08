WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.id) AS rn
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
), ActorRoles AS (
  SELECT
    c.movie_id,
    r.role,
    COUNT(*) AS actor_count
  FROM cast_info AS c
  JOIN role_type AS r
    ON c.role_id = r.id
  GROUP BY
    c.movie_id,
    r.role
), MovieCompanyDetails AS (
  SELECT
    mc.movie_id,
    cn.name AS company_name,
    ct.kind AS company_type,
    COUNT(*) OVER (PARTITION BY mc.movie_id) AS num_companies
  FROM movie_companies AS mc
  JOIN company_name AS cn
    ON mc.company_id = cn.id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
), TitlesWithKeyword AS (
  SELECT
    m.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS m
  JOIN keyword AS k
    ON m.keyword_id = k.id
  GROUP BY
    m.movie_id
)
SELECT
  r.movie_id,
  r.title,
  r.production_year,
  COALESCE(ar.role, 'Unknown Role') AS actor_role,
  COALESCE(ar.actor_count, 0) AS number_of_actors,
  COALESCE(mcd.company_name, 'No Company') AS company_name,
  COALESCE(mcd.company_type, 'Unknown Type') AS company_type,
  COALESCE(mcd.num_companies, 0) AS total_companies,
  COALESCE(t.keywords, 'No Keywords') AS keywords
FROM RankedMovies AS r
LEFT JOIN ActorRoles AS ar
  ON r.movie_id = ar.movie_id
LEFT JOIN MovieCompanyDetails AS mcd
  ON r.movie_id = mcd.movie_id
LEFT JOIN TitlesWithKeyword AS t
  ON r.movie_id = t.movie_id
WHERE
  r.rn <= 10
ORDER BY
  r.production_year DESC,
  r.movie_id;
