WITH RankedTitles AS (
  SELECT
    t.id AS title_id,
    t.title,
    t.production_year,
    k.keyword,
    ROW_NUMBER() OVER (PARTITION BY t.id ORDER BY k.keyword) AS keyword_rank
  FROM title AS t
  JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  JOIN keyword AS k
    ON mk.keyword_id = k.id
), TitleCounts AS (
  SELECT
    rt.title_id,
    COUNT(rt.keyword) AS keyword_count
  FROM RankedTitles AS rt
  GROUP BY
    rt.title_id
), MoviesWithHighKeywords AS (
  SELECT
    tc.title_id,
    t.title,
    t.production_year,
    tc.keyword_count
  FROM TitleCounts AS tc
  JOIN title AS t
    ON tc.title_id = t.id
  WHERE
    tc.keyword_count > 5
), MovieDetails AS (
  SELECT
    m.title_id,
    c.name AS company_name,
    m.production_year,
    STRING_AGG(p.name, ', ') AS cast_names,
    m.keyword_count
  FROM MoviesWithHighKeywords AS m
  LEFT JOIN movie_companies AS mc
    ON mc.movie_id = m.title_id
  LEFT JOIN company_name AS c
    ON mc.company_id = c.id
  LEFT JOIN cast_info AS ci
    ON ci.movie_id = m.title_id
  LEFT JOIN aka_name AS p
    ON ci.person_id = p.person_id
  GROUP BY
    m.title_id,
    c.name,
    m.production_year,
    m.keyword_count
)
SELECT
  md.title_id,
  md.company_name,
  md.production_year,
  md.cast_names,
  md.keyword_count
FROM MovieDetails AS md
ORDER BY
  md.production_year DESC,
  md.keyword_count DESC;
