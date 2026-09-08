WITH RankedTitles AS (
  SELECT
    t.id AS title_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.production_year DESC) AS year_rank
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
), ActorRoles AS (
  SELECT
    ci.movie_id,
    a.name,
    ci.nr_order,
    r.role,
    COALESCE(ci.note, 'Unknown Role') AS role_note,
    COUNT(*) OVER (PARTITION BY ci.movie_id) AS role_count
  FROM cast_info AS ci
  JOIN aka_name AS a
    ON ci.person_id = a.person_id
  JOIN role_type AS r
    ON ci.role_id = r.id
  WHERE
    NOT a.name IS NULL
  ORDER BY
    ci.nr_order
), MoviesWithKeywords AS (
  SELECT
    mt.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  JOIN aka_title AS mt
    ON mk.movie_id = mt.movie_id
  GROUP BY
    mt.movie_id
), CompanyDetails AS (
  SELECT
    mc.movie_id,
    c.name AS company_name,
    ct.kind AS company_type,
    COALESCE(mc.note, 'No Note Provided') AS company_note
  FROM movie_companies AS mc
  JOIN company_name AS c
    ON mc.company_id = c.id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
)
SELECT
  rt.title,
  rt.production_year,
  ar.name AS actor_name,
  ar.role,
  ar.role_note,
  mwk.keywords,
  cd.company_name,
  cd.company_type,
  cd.company_note
FROM RankedTitles AS rt
LEFT JOIN ActorRoles AS ar
  ON rt.title_id = ar.movie_id
LEFT JOIN MoviesWithKeywords AS mwk
  ON rt.title_id = mwk.movie_id
LEFT JOIN CompanyDetails AS cd
  ON rt.title_id = cd.movie_id
WHERE
  (
    rt.year_rank = 1 OR ar.role_count > 3
  )
  AND (
    NOT cd.company_type IS NULL OR cd.company_note LIKE '%special%'
  )
ORDER BY
  rt.production_year DESC,
  rt.title,
  ar.nr_order
LIMIT 100;
