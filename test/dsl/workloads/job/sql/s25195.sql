WITH ranked_titles AS (
  SELECT
    t.id AS title_id,
    t.title,
    t.production_year,
    COUNT(DISTINCT mc.company_id) AS company_count,
    STRING_AGG(DISTINCT n.name, ', ') AS cast_members,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY COUNT(DISTINCT mc.company_id) DESC) AS year_rank
  FROM title AS t
  LEFT JOIN movie_companies AS mc
    ON t.id = mc.movie_id
  LEFT JOIN complete_cast AS cc
    ON t.id = cc.movie_id
  LEFT JOIN aka_name AS a
    ON cc.subject_id = a.person_id
  LEFT JOIN cast_info AS ci
    ON a.person_id = ci.person_id AND ci.movie_id = t.id
  LEFT JOIN name AS n
    ON a.person_id = n.imdb_id
  WHERE
    t.production_year BETWEEN 2000 AND 2023
  GROUP BY
    t.id,
    t.title,
    t.production_year
)
SELECT
  rt.title_id,
  rt.title,
  rt.production_year,
  rt.company_count,
  rt.cast_members
FROM ranked_titles AS rt
WHERE
  rt.year_rank <= 5
ORDER BY
  rt.production_year DESC,
  rt.company_count DESC;
