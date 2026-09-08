WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.CreationDate DESC) AS rn,
    COUNT(*) OVER () AS TotalPosts
  FROM Posts AS p
  WHERE
    p.PostTypeId = 1 AND NOT p.Score IS NULL
), UserBadges AS (
  SELECT
    b.UserId,
    COUNT(b.Id) FILTER(WHERE
      b.Class = 1) AS GoldBadges,
    COUNT(b.Id) FILTER(WHERE
      b.Class = 2) AS SilverBadges,
    COUNT(b.Id) FILTER(WHERE
      b.Class = 3) AS BronzeBadges
  FROM Badges AS b
  GROUP BY
    b.UserId
), RecentVotes AS (
  SELECT
    v.PostId,
    COUNT(v.Id) AS VoteCount
  FROM Votes AS v
  WHERE
    v.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
  GROUP BY
    v.PostId
), ClosedPostCounts AS (
  SELECT
    ph.PostId,
    COUNT(ph.Id) AS CloseCount
  FROM PostHistory AS ph
  WHERE
    ph.PostHistoryTypeId = 10
  GROUP BY
    ph.PostId
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.Score,
  rp.TotalPosts,
  COALESCE(ub.GoldBadges, 0) AS GoldBadges,
  COALESCE(ub.SilverBadges, 0) AS SilverBadges,
  COALESCE(ub.BronzeBadges, 0) AS BronzeBadges,
  COALESCE(rv.VoteCount, 0) AS RecentVoteCount,
  COALESCE(cpc.CloseCount, 0) AS ClosedCount,
  CASE
    WHEN NOT rp.Score IS NULL AND rp.Score > 5
    THEN 'High Score'
    WHEN NOT rp.Score IS NULL AND rp.Score BETWEEN 1 AND 5
    THEN 'Medium Score'
    ELSE 'No Score'
  END AS ScoreCategory
FROM RankedPosts AS rp
LEFT JOIN Users AS u
  ON rp.PostId = u.Id
LEFT JOIN UserBadges AS ub
  ON u.Id = ub.UserId
LEFT JOIN RecentVotes AS rv
  ON rp.PostId = rv.PostId
LEFT JOIN ClosedPostCounts AS cpc
  ON rp.PostId = cpc.PostId
WHERE
  rp.rn = 1
ORDER BY
  rp.CreationDate DESC;
