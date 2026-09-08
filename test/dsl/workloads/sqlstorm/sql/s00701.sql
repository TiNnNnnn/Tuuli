WITH RankedPosts AS (
  SELECT
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    p.OwnerUserId,
    RANK() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC) AS PostRank
  FROM Posts AS p
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
), UserReputation AS (
  SELECT
    u.Id AS UserId,
    u.Reputation,
    b.Name AS BadgeName,
    COALESCE(SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END), 0) AS UpvoteCount,
    COALESCE(SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END), 0) AS DownvoteCount
  FROM Users AS u
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  LEFT JOIN Votes AS v
    ON u.Id = v.UserId
  GROUP BY
    u.Id,
    b.Name
), TopPosts AS (
  SELECT
    rp.Id,
    rp.Title,
    rp.ViewCount,
    ur.UserId,
    ur.Reputation,
    ur.BadgeName
  FROM RankedPosts AS rp
  LEFT JOIN UserReputation AS ur
    ON rp.OwnerUserId = ur.UserId
  WHERE
    rp.PostRank <= 10
), PostHistorySummary AS (
  SELECT
    ph.PostId,
    COUNT(CASE WHEN ph.PostHistoryTypeId IN (10, 11) THEN 1 END) AS CloseOpenCount,
    COUNT(CASE WHEN ph.PostHistoryTypeId = 24 THEN 1 END) AS EditCount,
    MAX(ph.CreationDate) AS LastModified
  FROM PostHistory AS ph
  GROUP BY
    ph.PostId
)
SELECT
  tp.Title,
  tp.ViewCount,
  tp.Reputation,
  COALESCE(th.CloseOpenCount, 0) AS CloseOpenCount,
  COALESCE(th.EditCount, 0) AS EditCount,
  p.CreationDate AS CreatedDate,
  CASE WHEN NOT tp.BadgeName IS NULL THEN 'Has Badge' ELSE 'No Badge' END AS BadgeStatus
FROM TopPosts AS tp
LEFT JOIN PostHistorySummary AS th
  ON tp.Id = th.PostId
JOIN Posts AS p
  ON tp.Id = p.Id
ORDER BY
  tp.ViewCount DESC;
