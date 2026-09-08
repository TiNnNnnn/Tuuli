WITH RECURSIVE MovieHierarchy AS (
  SELECT
    m.id AS movie_id,
    m.title,
    m.production_year,
    1 AS level
  FROM aka_title AS m
  WHERE
    m.production_year > 2000
  UNION ALL
  SELECT
    mk.linked_movie_id AS movie_id,
    m.title,
    m.production_year,
    mh.level + 1 AS level
  FROM movie_link AS mk
  JOIN aka_title AS m
    ON mk.linked_movie_id = m.id
  JOIN MovieHierarchy AS mh
    ON mk.movie_id = mh.movie_id
)
SELECT
  m.id AS movie_id,
  m.title,
  m.production_year,
  (
    SELECT
      COUNT(DISTINCT c.person_id)
    FROM cast_info AS c
    WHERE
      c.movie_id = m.id
  ) AS total_cast,
  cct.kind AS casting_type,
  ROW_NUMBER() OVER (PARTITION BY m.production_year ORDER BY m.title) AS row_num,
  COALESCE(NULLIF(m.note, ''), 'No note available') AS movie_note,
  STRING_AGG(DISTINCT kw.keyword, ', ') AS keywords
FROM aka_title AS m
LEFT JOIN movie_companies AS mc
  ON m.id = mc.movie_id
LEFT JOIN company_name AS cn
  ON mc.company_id = cn.id
LEFT JOIN comp_cast_type AS cct
  ON mc.company_type_id = cct.id
LEFT JOIN movie_keyword AS mk
  ON m.id = mk.movie_id
LEFT JOIN keyword AS kw
  ON mk.keyword_id = kw.id
WHERE
  m.production_year > 2000
  AND m.kind_id IN (
    SELECT
      id
    FROM kind_type
    WHERE
      kind LIKE '%Drama%'
  )
GROUP BY
  m.id,
  m.title,
  m.production_year,
  cct.kind,
  m.note
HAVING
  COUNT(DISTINCT cct.kind) > 1
ORDER BY
  m.production_year DESC,
  m.title;
