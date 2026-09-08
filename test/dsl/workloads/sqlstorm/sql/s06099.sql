WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.Score,
    p.CreationDate,
    p.ViewCount,
    COUNT(c.Id) AS CommentCount,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC, p.CreationDate DESC) AS Rank,
    p.OwnerUserId
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
    AND p.Score > 0
  GROUP BY
    p.Id,
    p.Title,
    p.Score,
    p.CreationDate,
    p.ViewCount,
    p.OwnerUserId
), UserEngagement AS (
  SELECT
    u.Id AS UserId,
    COUNT(DISTINCT p.Id) AS PostCount,
    SUM(COALESCE(c.Score, 0)) AS TotalCommentScore,
    SUM(v.BountyAmount) AS TotalBountyAmount
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  GROUP BY
    u.Id
)
SELECT
  up.DisplayName,
  up.Reputation,
  rp.PostId,
  rp.Title,
  rp.Score,
  rp.ViewCount,
  ue.PostCount,
  ue.TotalCommentScore,
  ue.TotalBountyAmount
FROM RankedPosts AS rp
JOIN Users AS up
  ON rp.OwnerUserId = up.Id
JOIN UserEngagement AS ue
  ON up.Id = ue.UserId
WHERE
  rp.Rank <= 10
ORDER BY
  rp.Score DESC,
  ue.TotalBountyAmount DESC;
