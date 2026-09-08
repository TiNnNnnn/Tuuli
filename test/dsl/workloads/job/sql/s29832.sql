WITH Actor_Movie_List AS (
  SELECT
    a.id AS actor_id,
    a.name AS actor_name,
    t.title AS movie_title,
    t.production_year,
    STRING_AGG(DISTINCT k.keyword, ', ') AS keywords
  FROM cast_info AS ci
  JOIN aka_name AS a
    ON ci.person_id = a.person_id
  JOIN aka_title AS t
    ON ci.movie_id = t.movie_id
  LEFT JOIN movie_keyword AS mk
    ON t.id = mk.movie_id
  LEFT JOIN keyword AS k
    ON mk.keyword_id = k.id
  WHERE
    NOT a.name IS NULL AND t.production_year BETWEEN 2000 AND 2023
  GROUP BY
    a.id,
    a.name,
    t.title,
    t.production_year
), Actor_Details AS (
  SELECT
    am.actor_id,
    am.actor_name,
    COUNT(DISTINCT am.movie_title) AS movie_count,
    STRING_AGG(DISTINCT am.keywords, '; ') AS all_keywords,
    (
      SELECT
        COUNT(*)
      FROM person_info AS pi
      WHERE
        pi.person_id = am.actor_id
    ) AS info_count
  FROM Actor_Movie_List AS am
  GROUP BY
    am.actor_id,
    am.actor_name
)
SELECT
  ad.actor_id,
  ad.actor_name,
  ad.movie_count,
  ad.all_keywords,
  ad.info_count
FROM Actor_Details AS ad
WHERE
  ad.movie_count > 5
ORDER BY
  ad.movie_count DESC,
  ad.actor_name ASC;
