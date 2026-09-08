WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.Tags,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.AnswerCount,
    ROW_NUMBER() OVER (PARTITION BY p.Tags ORDER BY p.CreationDate DESC) AS Rank
  FROM Posts AS p
  WHERE
    p.PostTypeId = 1
    AND p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
), AggregatedData AS (
  SELECT
    r.PostId,
    r.Title,
    r.Tags,
    r.CreationDate,
    r.Score,
    r.ViewCount,
    r.AnswerCount,
    COUNT(c.Id) AS CommentCount,
    AVG(u.Reputation) AS AverageReputation,
    SUM(CASE WHEN b.Class = 1 THEN 1 ELSE 0 END) AS GoldBadges,
    SUM(CASE WHEN b.Class = 2 THEN 1 ELSE 0 END) AS SilverBadges,
    SUM(CASE WHEN b.Class = 3 THEN 1 ELSE 0 END) AS BronzeBadges
  FROM RankedPosts AS r
  LEFT JOIN Comments AS c
    ON c.PostId = r.PostId
  LEFT JOIN Users AS u
    ON u.Id = r.PostId
  LEFT JOIN Badges AS b
    ON b.UserId = u.Id
  WHERE
    r.Rank = 1
  GROUP BY
    r.PostId,
    r.Title,
    r.Tags,
    r.CreationDate,
    r.Score,
    r.ViewCount,
    r.AnswerCount
)
SELECT
  a.PostId,
  a.Title,
  a.Tags,
  a.CreationDate,
  a.Score,
  a.ViewCount,
  a.AnswerCount,
  a.CommentCount,
  a.AverageReputation,
  a.GoldBadges,
  a.SilverBadges,
  a.BronzeBadges
FROM AggregatedData AS a
ORDER BY
  a.Score DESC,
  a.ViewCount DESC
LIMIT 50;
