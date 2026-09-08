WITH movie_details AS (
  SELECT
    t.title AS movie_title,
    t.production_year,
    k.keyword AS movie_keyword,
    c.name AS company_name,
    r.role AS cast_role,
    n.name AS actor_name
  FROM title AS t
  JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  JOIN movie_companies AS mc
    ON t.id = mc.movie_id
  JOIN company_name AS c
    ON mc.company_id = c.id
  JOIN complete_cast AS cc
    ON t.id = cc.movie_id
  JOIN cast_info AS ci
    ON cc.subject_id = ci.id
  JOIN role_type AS r
    ON ci.role_id = r.id
  JOIN aka_name AS n
    ON ci.person_id = n.person_id
  WHERE
    t.production_year BETWEEN 2000 AND 2020
    AND NOT k.keyword IS NULL
    AND c.country_code = 'USA'
), keyword_stats AS (
  SELECT
    movie_keyword,
    COUNT(*) AS keyword_count,
    STRING_AGG(DISTINCT movie_title, ', ') AS related_movies
  FROM movie_details
  GROUP BY
    movie_keyword
)
SELECT
  ks.movie_keyword,
  ks.keyword_count,
  ks.related_movies,
  AVG(EXTRACT(YEAR FROM CURRENT_DATE) - m.production_year) AS average_age
FROM keyword_stats AS ks
JOIN movie_details AS m
  ON ks.movie_keyword = m.movie_keyword
GROUP BY
  ks.movie_keyword,
  ks.keyword_count,
  ks.related_movies
ORDER BY
  ks.keyword_count DESC,
  ks.movie_keyword;
