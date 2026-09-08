WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    u.DisplayName AS AuthorName,
    p.ViewCount,
    p.Score,
    ROW_NUMBER() OVER (PARTITION BY pt.Name ORDER BY p.ViewCount DESC) AS RankByViews,
    ROW_NUMBER() OVER (PARTITION BY pt.Name ORDER BY p.Score DESC) AS RankByScore
  FROM Posts AS p
  JOIN PostTypes AS pt
    ON p.PostTypeId = pt.Id
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  WHERE
    p.CreationDate > (
      CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
    )
    AND p.ViewCount > 100
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.AuthorName,
  rp.ViewCount,
  rp.Score,
  CASE WHEN rp.RankByViews <= 10 THEN 'Top 10 Viewed' ELSE 'Other' END AS ViewRankCategory,
  CASE WHEN rp.RankByScore <= 10 THEN 'Top 10 Scored' ELSE 'Other' END AS ScoreRankCategory
FROM RankedPosts AS rp
WHERE
  rp.RankByViews <= 10 OR rp.RankByScore <= 10
ORDER BY
  rp.ViewCount DESC,
  rp.Score DESC;
