WITH ranked_movies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    COALESCE(CAST(CAST(m.production_year AS TEXT) AS INT), 0) AS production_year,
    ROW_NUMBER() OVER (PARTITION BY t.kind_id ORDER BY m.production_year DESC NULLS LAST) AS rank_by_year,
    COUNT(ci.id) OVER (PARTITION BY t.id) AS cast_count
  FROM aka_title AS t
  LEFT JOIN title AS m
    ON t.id = m.id
  LEFT JOIN cast_info AS ci
    ON t.movie_id = ci.movie_id
  WHERE
    NOT m.production_year IS NULL OR NOT t.kind_id IS NULL
), movie_keywords AS (
  SELECT
    mk.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    mk.movie_id
), movie_casts AS (
  SELECT
    c.id AS cast_id,
    a.name AS actor_name,
    ri.role AS role_name,
    c.movie_id,
    ROW_NUMBER() OVER (PARTITION BY c.movie_id ORDER BY c.nr_order) AS actor_order
  FROM cast_info AS c
  JOIN aka_name AS a
    ON c.person_id = a.person_id
  LEFT JOIN role_type AS ri
    ON c.role_id = ri.id
  WHERE
    NOT a.name IS NULL
), filtered_movies AS (
  SELECT
    rm.movie_id,
    rm.title,
    rm.production_year,
    rm.rank_by_year,
    rm.cast_count,
    mk.keywords,
    COALESCE(mc.id, 0) AS company_id,
    mc.note AS company_note,
    STRING_AGG(DISTINCT mc.company_id || ': ' || co.name, '; ') AS company_details
  FROM ranked_movies AS rm
  LEFT JOIN movie_companies AS mc
    ON rm.movie_id = mc.movie_id
  LEFT JOIN company_name AS co
    ON mc.company_id = co.id
  LEFT JOIN movie_keywords AS mk
    ON rm.movie_id = mk.movie_id
  WHERE
    rm.rank_by_year <= 5
  GROUP BY
    rm.movie_id,
    rm.title,
    rm.production_year,
    rm.rank_by_year,
    rm.cast_count,
    mk.keywords,
    mc.id,
    mc.note
)
SELECT
  f.title,
  f.production_year,
  f.cast_count,
  f.keywords,
  f.company_details,
  CASE
    WHEN EXISTS(
      SELECT
        1
      FROM movie_casts
      WHERE
        movie_id = f.movie_id AND NOT role_name IS NULL
    )
    THEN 'Has Cast'
    ELSE 'No Cast Found'
  END AS cast_info_status,
  f.rank_by_year
FROM filtered_movies AS f
WHERE
  f.production_year > 1980
ORDER BY
  f.production_year DESC,
  f.title
LIMIT 1000;
