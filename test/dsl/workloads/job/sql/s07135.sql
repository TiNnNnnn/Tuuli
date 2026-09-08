SELECT
  a.name AS aka_name,
  t.title AS movie_title,
  c.note AS role_note,
  p.info AS person_info,
  k.keyword AS movie_keyword,
  co.name AS company_name,
  ct.kind AS company_type
FROM aka_name AS a
JOIN cast_info AS c
  ON a.person_id = c.person_id
JOIN title AS t
  ON c.movie_id = t.id
JOIN movie_info AS m
  ON t.id = m.movie_id
JOIN person_info AS p
  ON a.person_id = p.person_id
JOIN movie_keyword AS mk
  ON t.id = mk.movie_id
JOIN keyword AS k
  ON mk.keyword_id = k.id
JOIN movie_companies AS mc
  ON t.id = mc.movie_id
JOIN company_name AS co
  ON mc.company_id = co.id
JOIN company_type AS ct
  ON mc.company_type_id = ct.id
WHERE
  t.production_year >= 2000
  AND p.info_type_id = (
    SELECT
      id
    FROM info_type
    WHERE
      info = 'Birthdate'
  )
  AND k.keyword LIKE '%Action%'
ORDER BY
  t.production_year DESC,
  a.name ASC;
