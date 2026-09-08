WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    p.Tags,
    ROW_NUMBER() OVER (PARTITION BY pt.Name ORDER BY p.Score DESC) AS RankByScore,
    ROW_NUMBER() OVER (PARTITION BY pt.Name ORDER BY p.ViewCount DESC) AS RankByViews,
    ARRAY_LENGTH(STRING_TO_ARRAY(p.Tags, '>'), 1) AS TagCount
  FROM Posts AS p
  JOIN PostTypes AS pt
    ON p.PostTypeId = pt.Id
  WHERE
    p.CreationDate >= CAST('2024-10-01' AS DATE) - INTERVAL '1 YEAR'
)
SELECT
  pt.Name AS PostType,
  COUNT(rp.PostId) AS TotalPosts,
  AVG(rp.ViewCount) AS AvgViewCount,
  AVG(rp.Score) AS AvgScore,
  SUM(rp.TagCount) AS TotalTags,
  STRING_AGG(rp.Title, '; ') AS Titles,
  MAX(rp.CreationDate) AS MostRecentPostDate
FROM RankedPosts AS rp
JOIN PostTypes AS pt
  ON rp.PostId = pt.Id
WHERE
  rp.RankByScore <= 5 OR rp.RankByViews <= 5
GROUP BY
  pt.Name
ORDER BY
  TotalPosts DESC,
  AvgScore DESC;
