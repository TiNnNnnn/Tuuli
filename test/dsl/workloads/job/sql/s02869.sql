WITH RankedMovies AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    ROW_NUMBER() OVER (PARTITION BY t.production_year ORDER BY t.production_year DESC) AS year_rank
  FROM aka_title AS t
  WHERE
    NOT t.production_year IS NULL
    AND t.kind_id IN (
      SELECT
        id
      FROM kind_type
      WHERE
        kind = 'movie'
    )
), DirectorInfo AS (
  SELECT
    c.movie_id,
    a.name AS director_name,
    c.nr_order
  FROM cast_info AS c
  INNER JOIN aka_name AS a
    ON a.person_id = c.person_id
  WHERE
    c.role_id = (
      SELECT
        id
      FROM role_type
      WHERE
        role = 'director'
    )
), TitleKeywords AS (
  SELECT
    mk.movie_id,
    STRING_AGG(k.keyword, ', ') AS keywords
  FROM movie_keyword AS mk
  INNER JOIN keyword AS k
    ON mk.keyword_id = k.id
  GROUP BY
    mk.movie_id
), MoviesWithKeywords AS (
  SELECT
    rm.movie_id,
    rm.title,
    rm.production_year,
    COALESCE(tk.keywords, 'No keywords') AS keywords
  FROM RankedMovies AS rm
  LEFT JOIN TitleKeywords AS tk
    ON rm.movie_id = tk.movie_id
)
SELECT
  mwk.movie_id,
  mwk.title,
  mwk.production_year,
  di.director_name,
  mwk.keywords,
  CASE
    WHEN mwk.production_year >= 2000
    THEN 'Modern Era'
    WHEN mwk.production_year IS NULL
    THEN 'Unknown Year'
    ELSE 'Classic Era'
  END AS era_category
FROM MoviesWithKeywords AS mwk
LEFT JOIN DirectorInfo AS di
  ON mwk.movie_id = di.movie_id
WHERE
  NOT mwk.keywords IS NULL AND mwk.production_year BETWEEN 1990 AND 2023
ORDER BY
  mwk.production_year DESC,
  mwk.title;
