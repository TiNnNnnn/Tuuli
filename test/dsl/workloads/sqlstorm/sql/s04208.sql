WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.Score DESC) AS Rank
  FROM Posts AS p
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
    AND p.PostTypeId = 1
), UserStatistics AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    COALESCE(SUM(v.BountyAmount), 0) AS TotalBounties,
    COUNT(DISTINCT p.Id) AS TotalPosts,
    SUM(p.Score) AS TotalScore
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId AND v.VoteTypeId IN (9, 8)
  GROUP BY
    u.Id
), TopContributors AS (
  SELECT
    us.UserId,
    us.DisplayName,
    us.TotalPosts,
    us.TotalScore,
    RANK() OVER (ORDER BY us.TotalScore DESC) AS ScoreRank
  FROM UserStatistics AS us
  WHERE
    us.TotalPosts > 0
)
SELECT
  rp.PostId,
  rp.Title,
  rp.Score AS PostScore,
  rp.ViewCount,
  u.DisplayName AS OwnerDisplayName,
  us.TotalBounties,
  tc.TotalPosts AS OwnerTotalPosts,
  tc.TotalScore AS OwnerTotalScore
FROM RankedPosts AS rp
JOIN Users AS u
  ON rp.PostId = u.Id
LEFT JOIN UserStatistics AS us
  ON u.Id = us.UserId
LEFT JOIN TopContributors AS tc
  ON us.UserId = tc.UserId
WHERE
  rp.Rank <= 5
ORDER BY
  rp.Score DESC;
