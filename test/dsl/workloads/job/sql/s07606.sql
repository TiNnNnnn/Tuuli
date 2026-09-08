WITH RankedTitles AS (
  SELECT
    t.id AS title_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.title) AS title_rank
  FROM title AS t
  WHERE
    NOT t.production_year IS NULL
), CastDetails AS (
  SELECT
    ci.movie_id,
    a.name AS actor_name,
    ci.note AS role_note,
    r.role AS role_type
  FROM cast_info AS ci
  JOIN aka_name AS a
    ON ci.person_id = a.person_id
  JOIN role_type AS r
    ON ci.role_id = r.id
), MovieKeywords AS (
  SELECT
    mk.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    mk.movie_id
)
SELECT
  rt.title,
  rt.production_year,
  cd.actor_name,
  cd.role_note,
  cd.role_type,
  mk.keywords
FROM RankedTitles AS rt
JOIN complete_cast AS cc
  ON rt.title_id = cc.movie_id
JOIN CastDetails AS cd
  ON cc.movie_id = cd.movie_id
LEFT JOIN MovieKeywords AS mk
  ON cc.movie_id = mk.movie_id
WHERE
  rt.title_rank <= 5
ORDER BY
  rt.production_year DESC,
  rt.title;
