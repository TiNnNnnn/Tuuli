WITH ranked_titles AS (
  SELECT
    t.id AS title_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.title) AS title_rank
  FROM aka_title AS t
  WHERE
    t.production_year > 2000
), cast_info_enhanced AS (
  SELECT
    ci.movie_id,
    ci.person_id,
    ci.role_id,
    COALESCE(r.role, 'Unknown Role') AS role_name,
    COUNT(*) OVER (PARTITION BY ci.movie_id) AS total_cast_for_movie
  FROM cast_info AS ci
  LEFT JOIN role_type AS r
    ON ci.role_id = r.id
), company_details AS (
  SELECT
    mc.movie_id,
    c.name AS company_name,
    ct.kind AS company_type,
    COUNT(*) OVER (PARTITION BY mc.movie_id) AS total_companies
  FROM movie_companies AS mc
  JOIN company_name AS c
    ON mc.company_id = c.id
  JOIN company_type AS ct
    ON mc.company_type_id = ct.id
), movie_info_summary AS (
  SELECT
    m.movie_id,
    STRING_AGG(DISTINCT mi.info, '; ') AS all_info,
    COUNT(DISTINCT mk.keyword_id) AS total_keywords
  FROM movie_info AS m
  LEFT JOIN movie_keyword AS mk
    ON m.movie_id = mk.movie_id
  LEFT JOIN movie_info_idx AS mi
    ON m.movie_id = mi.movie_id
  GROUP BY
    m.movie_id
)
SELECT
  rt.title_id,
  rt.title,
  rt.production_year,
  ci.person_id,
  ci.role_name,
  cd.company_name,
  cd.company_type,
  mis.all_info,
  mis.total_keywords
FROM ranked_titles AS rt
JOIN cast_info_enhanced AS ci
  ON rt.title_id = ci.movie_id
LEFT JOIN company_details AS cd
  ON ci.movie_id = cd.movie_id
LEFT JOIN movie_info_summary AS mis
  ON ci.movie_id = mis.movie_id
WHERE
  (
    cd.total_companies IS NULL OR cd.total_companies > 2
  )
  AND (
    mis.total_keywords IS NULL OR mis.total_keywords > 1
  )
  AND rt.title_rank <= 5
ORDER BY
  rt.production_year DESC,
  rt.title;
