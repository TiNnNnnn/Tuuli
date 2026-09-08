WITH RankedMovies AS (
  SELECT
    a.title,
    a.production_year,
    COALESCE(c.kind, 'Unknown') AS kind,
    COUNT(DISTINCT ci.person_id) AS cast_count,
    ROW_NUMBER() OVER (PARTITION BY a.production_year ORDER BY COUNT(DISTINCT ci.person_id) DESC) AS rank
  FROM aka_title AS a
  LEFT JOIN movie_companies AS mc
    ON a.id = mc.movie_id
  LEFT JOIN company_name AS cn
    ON mc.company_id = cn.id
  LEFT JOIN cast_info AS ci
    ON a.id = ci.movie_id
  LEFT JOIN role_type AS rt
    ON ci.role_id = rt.id
  LEFT JOIN kind_type AS c
    ON a.kind_id = c.id
  GROUP BY
    a.title,
    a.production_year,
    c.kind
), TopMovies AS (
  SELECT
    title,
    production_year,
    kind,
    cast_count
  FROM RankedMovies
  WHERE
    rank <= 5
), CastDetails AS (
  SELECT
    p.name,
    ci.movie_id,
    rt.role
  FROM cast_info AS ci
  JOIN aka_name AS p
    ON ci.person_id = p.person_id
  JOIN role_type AS rt
    ON ci.role_id = rt.id
)
SELECT
  tm.title,
  tm.production_year,
  tm.kind,
  tm.cast_count,
  STRING_AGG(
    CASE
      WHEN NOT cd.role IS NULL
      THEN cd.name || ' (' || cd.role || ')'
      ELSE 'Unknown'
    END,
    ', '
  ) AS cast_details
FROM TopMovies AS tm
LEFT JOIN CastDetails AS cd
  ON tm.title = (
    SELECT
      title
    FROM aka_title
    WHERE
      id = cd.movie_id
  )
GROUP BY
  tm.title,
  tm.production_year,
  tm.kind,
  tm.cast_count
ORDER BY
  tm.production_year DESC,
  tm.cast_count DESC;
