WITH ranked_movies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    COUNT(DISTINCT ci.person_id) AS num_cast_members,
    STRING_AGG(DISTINCT an.name, ', ') AS all_aka_names
  FROM title AS t
  JOIN complete_cast AS cc
    ON t.id = cc.movie_id
  JOIN cast_info AS ci
    ON cc.subject_id = ci.id
  JOIN aka_name AS an
    ON ci.person_id = an.person_id
  WHERE
    t.production_year BETWEEN 2000 AND 2020
  GROUP BY
    t.id,
    t.title,
    t.production_year
), keyword_stats AS (
  SELECT
    mk.movie_id,
    COUNT(mk.keyword_id) AS keyword_count
  FROM movie_keyword AS mk
  GROUP BY
    mk.movie_id
), movie_details AS (
  SELECT
    rm.movie_id,
    rm.title,
    rm.production_year,
    rm.num_cast_members,
    COALESCE(ks.keyword_count, 0) AS keyword_count,
    rm.all_aka_names
  FROM ranked_movies AS rm
  LEFT JOIN keyword_stats AS ks
    ON rm.movie_id = ks.movie_id
)
SELECT
  md.title,
  md.production_year,
  md.num_cast_members,
  md.keyword_count,
  md.all_aka_names
FROM movie_details AS md
ORDER BY
  md.production_year DESC,
  md.num_cast_members DESC;
