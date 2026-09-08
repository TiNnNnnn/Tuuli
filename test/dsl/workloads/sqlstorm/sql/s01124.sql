WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC) AS PostRank,
    u.Reputation AS UserReputation
  FROM Posts AS p
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  WHERE
    NOT p.Score IS NULL
), CommentStatistics AS (
  SELECT
    PostId,
    COUNT(*) AS TotalComments,
    AVG(Score) AS AverageCommentScore
  FROM Comments
  GROUP BY
    PostId
), BadgeCounts AS (
  SELECT
    UserId,
    COUNT(*) AS TotalBadges,
    SUM(CASE WHEN Class = 1 THEN 1 ELSE 0 END) AS GoldBadges,
    SUM(CASE WHEN Class = 2 THEN 1 ELSE 0 END) AS SilverBadges,
    SUM(CASE WHEN Class = 3 THEN 1 ELSE 0 END) AS BronzeBadges
  FROM Badges
  GROUP BY
    UserId
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.Score,
  cs.TotalComments,
  cs.AverageCommentScore,
  bc.TotalBadges,
  bc.GoldBadges,
  bc.SilverBadges,
  bc.BronzeBadges,
  CASE WHEN NOT bc.UserId IS NULL THEN 'Has Badges' ELSE 'No Badges' END AS BadgeStatus
FROM RankedPosts AS rp
LEFT JOIN CommentStatistics AS cs
  ON rp.PostId = cs.PostId
LEFT JOIN BadgeCounts AS bc
  ON rp.UserReputation = bc.UserId
WHERE
  rp.PostRank <= 5
  AND (
    cs.TotalComments IS NULL OR cs.TotalComments > 2
  )
  AND (
    rp.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
  )
ORDER BY
  rp.Score DESC,
  cs.TotalComments DESC;
