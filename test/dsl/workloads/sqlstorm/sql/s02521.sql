WITH RecentPosts AS (
  SELECT
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.OwnerUserId,
    RANK() OVER (PARTITION BY p.OwnerUserId ORDER BY p.CreationDate DESC) AS PostRank
  FROM Posts AS p
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
), UserReputation AS (
  SELECT
    u.Id AS UserId,
    u.Reputation,
    COUNT(DISTINCT r.Id) AS RecentPostCount
  FROM Users AS u
  LEFT JOIN RecentPosts AS r
    ON u.Id = r.OwnerUserId
  GROUP BY
    u.Id,
    u.Reputation
), UpvotedPosts AS (
  SELECT
    v.PostId,
    COUNT(v.Id) AS UpvoteCount
  FROM Votes AS v
  WHERE
    v.VoteTypeId = 2
  GROUP BY
    v.PostId
), PostDetails AS (
  SELECT
    rp.Id,
    rp.Title,
    rp.CreationDate,
    rp.ViewCount,
    ur.Reputation,
    ur.RecentPostCount,
    COALESCE(up.UpvoteCount, 0) AS UpvoteCount
  FROM RecentPosts AS rp
  JOIN UserReputation AS ur
    ON rp.OwnerUserId = ur.UserId
  LEFT JOIN UpvotedPosts AS up
    ON rp.Id = up.PostId
)
SELECT
  pd.Title,
  pd.CreationDate,
  pd.ViewCount,
  pd.Reputation,
  pd.RecentPostCount,
  pd.UpvoteCount,
  CASE WHEN pd.RecentPostCount >= 5 THEN 'Active User' ELSE 'New User' END AS UserStatus
FROM PostDetails AS pd
WHERE
  pd.UpvoteCount > 0
ORDER BY
  pd.Reputation DESC,
  pd.UpvoteCount DESC;
