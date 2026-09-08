SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  c.note AS cast_note,
  p.info AS person_info,
  k.keyword AS movie_keyword,
  cn.name AS company_name,
  ct.kind AS company_type,
  m.info AS movie_info
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN aka_title AS t
  ON c.movie_id = t.movie_id
JOIN person_info AS p
  ON a.person_id = p.person_id
JOIN movie_keyword AS mk
  ON t.movie_id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
JOIN movie_companies AS mc
  ON t.movie_id = mc.movie_id
JOIN company_name AS cn
  ON mc.company_id = cn.id
JOIN company_type AS ct
  ON mc.company_type_id = ct.id
JOIN movie_info AS m
  ON t.movie_id = m.movie_id
WHERE
  p.info_type_id = (
    SELECT
      id
    FROM info_type
    WHERE
      info = 'Bio'
  )
  AND m.info_type_id = (
    SELECT
      id
    FROM info_type
    WHERE
      info = 'Summary'
  )
ORDER BY
  t.production_year DESC,
  a.name;
