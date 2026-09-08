WITH RankedMovies AS (
  SELECT
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY COUNT(c.person_id) DESC) AS most_casted,
    t.id AS title_id
  FROM aka_title AS t
  JOIN cast_info AS c
    ON t.id = c.movie_id
  GROUP BY
    t.id,
    t.title,
    t.production_year
), CteWithNotes AS (
  SELECT
    t.id AS title_id,
    t.title,
    STRING_AGG(DISTINCT c.note, ', ') AS notes
  FROM aka_title AS t
  LEFT JOIN cast_info AS c
    ON t.id = c.movie_id
  GROUP BY
    t.id,
    t.title
), MovieKeywords AS (
  SELECT
    mk.movie_id,
    k.keyword
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  WHERE
    k.keyword LIKE '%Action%'
), PersonRoleCount AS (
  SELECT
    ci.person_id,
    COUNT(DISTINCT ci.movie_id) AS movie_count,
    SUM(CASE WHEN NOT r.role IS NULL THEN 1 ELSE 0 END) AS roles_count
  FROM cast_info AS ci
  JOIN role_type AS r
    ON ci.role_id = r.id
  GROUP BY
    ci.person_id
)
SELECT
  rm.title,
  rm.production_year,
  COALESCE(cn.notes, 'No Notes Available') AS notes,
  COUNT(mk.keyword) AS keyword_count,
  pr.movie_count,
  pr.roles_count
FROM RankedMovies AS rm
LEFT JOIN CteWithNotes AS cn
  ON rm.title_id = cn.title_id
LEFT JOIN MovieKeywords AS mk
  ON rm.title_id = mk.movie_id
LEFT JOIN PersonRoleCount AS pr
  ON pr.person_id IN (
    SELECT
      person_id
    FROM cast_info
    WHERE
      movie_id = rm.title_id
  )
WHERE
  rm.most_casted = 1 AND NOT mk.keyword IS NULL AND pr.movie_count > 5
GROUP BY
  rm.title,
  rm.production_year,
  cn.notes,
  pr.movie_count,
  pr.roles_count
HAVING
  COUNT(mk.keyword) > 0
ORDER BY
  rm.production_year DESC,
  rm.title
LIMIT 10;
