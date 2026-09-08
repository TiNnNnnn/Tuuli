WITH MovieInfo AS (
  SELECT
    t.id AS movie_id,
    t.title,
    t.production_year,
    STRING_AGG(DISTINCT ak.name, ', ') AS aka_names,
    STRING_AGG(DISTINCT c.name, ', ') AS company_names,
    STRING_AGG(DISTINCT k.keyword, ', ') AS keywords,
    COUNT(DISTINCT ca.person_id) AS cast_count
  FROM aka_title AS t
  LEFT JOIN aka_name AS ak
    ON ak.person_id = t.id
  LEFT JOIN movie_companies AS mc
    ON mc.movie_id = t.id
  LEFT JOIN company_name AS c
    ON c.id = mc.company_id
  LEFT JOIN movie_keyword AS mk
    ON mk.movie_id = t.id
  LEFT JOIN keyword AS k
    ON k.id = mk.keyword_id
  LEFT JOIN cast_info AS ca
    ON ca.movie_id = t.id
  GROUP BY
    t.id,
    t.title,
    t.production_year
), RankedMovies AS (
  SELECT
    movie_id,
    title,
    production_year,
    aka_names,
    company_names,
    keywords,
    cast_count,
    ROW_NUMBER() OVER (ORDER BY production_year DESC, cast_count DESC) AS rank
  FROM MovieInfo
)
SELECT
  rm.rank,
  rm.title,
  rm.production_year,
  rm.aka_names,
  rm.company_names,
  rm.keywords,
  rm.cast_count
FROM RankedMovies AS rm
WHERE
  rm.rank <= 10
ORDER BY
  rm.rank;
