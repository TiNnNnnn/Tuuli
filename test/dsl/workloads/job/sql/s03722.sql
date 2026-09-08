WITH RankedMovies AS (
  SELECT
    a.title,
    a.production_year,
    COALESCE(c.name, 'Unknown') AS company_name,
    ROW_NUMBER() OVER (PARTITION BY a.production_year ORDER BY COUNT(DISTINCT ci.person_id) DESC) AS rank_by_cast_size,
    SUM(CASE WHEN NOT k.keyword IS NULL THEN 1 ELSE 0 END) AS keyword_count
  FROM aka_title AS a
  LEFT JOIN movie_companies AS mc
    ON a.id = mc.movie_id
  LEFT JOIN company_name AS c
    ON mc.company_id = c.id
  LEFT JOIN complete_cast AS cc
    ON a.id = cc.movie_id
  LEFT JOIN cast_info AS ci
    ON cc.subject_id = ci.person_id
  LEFT JOIN movie_keyword AS mk
    ON a.id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  WHERE
    a.production_year > 2000
  GROUP BY
    a.title,
    a.production_year,
    c.name
), FilteredMovies AS (
  SELECT
    title,
    production_year,
    company_name,
    keyword_count
  FROM RankedMovies
  WHERE
    rank_by_cast_size <= 5
)
SELECT
  f.title,
  f.production_year,
  f.company_name,
  f.keyword_count,
  CASE
    WHEN f.keyword_count > 10
    THEN 'Highly Tagged'
    WHEN f.keyword_count BETWEEN 5 AND 10
    THEN 'Moderately Tagged'
    ELSE 'Low Tagged'
  END AS tagging_category
FROM FilteredMovies AS f
LEFT JOIN cast_info AS ci
  ON f.title = (
    SELECT
      a.title
    FROM aka_title AS a
    WHERE
      a.id = ci.movie_id
  )
WHERE
  NOT ci.nr_order IS NULL OR ci.person_role_id IS NULL
ORDER BY
  f.production_year DESC,
  f.keyword_count DESC;
