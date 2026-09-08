WITH ranked_movies AS (
  SELECT
    a.id AS movie_id,
    a.title AS movie_title,
    a.production_year,
    k.keyword AS movie_keyword,
    ROW_NUMBER() OVER (PARTITION BY a.id ORDER BY a.production_year DESC) AS rank_year
  FROM aka_title AS a
  JOIN movie_keyword AS mk
    ON a.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  WHERE
    a.production_year >= 2000
), movie_cast AS (
  SELECT
    c.movie_id,
    c.person_id,
    p.name AS actor_name,
    r.role AS actor_role
  FROM cast_info AS c
  JOIN aka_name AS p
    ON c.person_id = p.person_id
  JOIN role_type AS r
    ON c.role_id = r.id
), company_details AS (
  SELECT
    mc.movie_id,
    co.name AS company_name,
    ct.kind AS company_type
  FROM movie_companies AS mc
  JOIN company_name AS co
    ON mc.company_id = co.id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
), movie_info_details AS (
  SELECT
    mi.movie_id,
    ARRAY_AGG(CONCAT(it.info, ': ', mi.info)) AS additional_info
  FROM movie_info AS mi
  JOIN info_type AS it
    ON mi.info_type_id = it.id
  GROUP BY
    mi.movie_id
)
SELECT
  rm.movie_title,
  rm.production_year,
  rm.movie_keyword,
  STRING_AGG(DISTINCT mc.actor_name || ' (' || mc.actor_role || ')', ', ') AS cast,
  STRING_AGG(DISTINCT cd.company_name || ' [' || cd.company_type || ']', ', ') AS production_companies,
  md.additional_info
FROM ranked_movies AS rm
LEFT JOIN movie_cast AS mc
  ON rm.movie_id = mc.movie_id
LEFT JOIN company_details AS cd
  ON rm.movie_id = cd.movie_id
LEFT JOIN movie_info_details AS md
  ON rm.movie_id = md.movie_id
WHERE
  rm.rank_year = 1
GROUP BY
  rm.movie_id,
  rm.movie_title,
  rm.production_year,
  rm.movie_keyword,
  md.additional_info
ORDER BY
  rm.production_year DESC;
