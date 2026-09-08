SELECT
  akn.name AS aka_name,
  tit.title AS movie_title,
  cnt.name AS company_name,
  rt.role AS person_role,
  pi.info AS person_info
FROM aka_name AS akn
JOIN cast_info AS ci
  ON akn.person_id = ci.person_id
JOIN title AS tit
  ON ci.movie_id = tit.id
JOIN movie_companies AS mc
  ON tit.id = mc.movie_id
JOIN company_name AS cnt
  ON mc.company_id = cnt.id
JOIN role_type AS rt
  ON ci.role_id = rt.id
JOIN person_info AS pi
  ON akn.person_id = pi.person_id
WHERE
  tit.production_year >= 2000
  AND cnt.country_code = 'USA'
  AND pi.info_type_id IN (
    SELECT
      id
    FROM info_type
    WHERE
      info = 'Biography'
  )
ORDER BY
  tit.production_year DESC,
  akn.name;
