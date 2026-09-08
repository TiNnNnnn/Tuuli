WITH MovieDetails AS (
  SELECT
    at.id AS movie_id,
    at.title,
    at.production_year,
    COUNT(DISTINCT ci.person_id) AS cast_count,
    ARRAY_AGG(DISTINCT an.name) AS actor_names
  FROM aka_title AS at
  LEFT JOIN complete_cast AS cc
    ON at.id = cc.movie_id
  LEFT JOIN cast_info AS ci
    ON cc.subject_id = ci.id
  LEFT JOIN aka_name AS an
    ON ci.person_id = an.person_id
  GROUP BY
    at.id,
    at.title,
    at.production_year
), RatedMovies AS (
  SELECT
    md.movie_id,
    md.title,
    md.production_year,
    md.cast_count,
    CASE
      WHEN md.production_year < 2000
      THEN 'Classic'
      WHEN md.production_year BETWEEN 2000 AND 2010
      THEN 'Modern'
      ELSE 'Recent'
    END AS movie_type
  FROM MovieDetails AS md
)
SELECT
  rm.movie_id,
  rm.title,
  rm.production_year,
  rm.cast_count,
  rm.movie_type,
  CASE
    WHEN rm.cast_count IS NULL
    THEN 'No Cast'
    WHEN rm.cast_count > 10
    THEN 'Large Ensemble'
    ELSE 'Standard Cast'
  END AS cast_size_category
FROM RatedMovies AS rm
WHERE
  rm.movie_type IN ('Modern', 'Recent') AND NOT rm.cast_count IS NULL
ORDER BY
  rm.production_year DESC,
  rm.cast_count DESC;
