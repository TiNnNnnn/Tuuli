WITH RECURSIVE MovieHierarchy AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    1 AS depth
  FROM aka_title AS t
  WHERE
    t.kind_id = (
      SELECT
        id
      FROM kind_type
      WHERE
        kind = 'movie'
    )
  UNION ALL
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    mh.depth + 1 AS depth
  FROM movie_link AS ml
  JOIN aka_title AS t
    ON ml.linked_movie_id = t.id
  JOIN MovieHierarchy AS mh
    ON ml.movie_id = mh.movie_id
)
SELECT
  a.name AS actor_name,
  m.title AS movie_title,
  m.production_year,
  STRING_AGG(DISTINCT k.keyword, ', ') AS keywords,
  COUNT(DISTINCT c.id) AS total_cast,
  AVG(CASE WHEN NOT pi.info IS NULL THEN 1 ELSE 0 END) AS presence_ratio,
  ROW_NUMBER() OVER (PARTITION BY a.id ORDER BY m.production_year DESC) AS movie_rank
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN aka_title AS m
  ON c.movie_id = m.id
LEFT JOIN movie_keyword AS mk
  ON m.id = mk.movie_id
LEFT JOIN keyword AS k
  ON mk.keyword_id = k.id
LEFT JOIN person_info AS pi
  ON a.person_id = pi.person_id
  AND pi.info_type_id = (
    SELECT
      id
    FROM info_type
    WHERE
      info = 'Biography'
  )
WHERE
  m.production_year >= 2000
  AND (
    a.name_pcode_cf IS NULL OR NOT a.name_pcode_nf IS NULL
  )
GROUP BY
  a.id,
  a.name,
  m.id,
  m.title,
  m.production_year
HAVING
  COUNT(DISTINCT c.id) > 5
ORDER BY
  presence_ratio DESC,
  movie_rank ASC;
