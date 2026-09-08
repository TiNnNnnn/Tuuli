WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    ROW_NUMBER() OVER (PARTITION BY CASE
      WHEN p.PostTypeId = 1
      THEN 'Questions'
      WHEN p.PostTypeId = 2
      THEN 'Answers'
      ELSE 'Others'
    END ORDER BY p.Score DESC) AS Rank
  FROM Posts AS p
  WHERE
    p.CreationDate >= CAST('2024-10-01' AS DATE) - INTERVAL '1 YEAR'
), UserStats AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS Upvotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS Downvotes,
    COUNT(DISTINCT b.Id) AS BadgeCount
  FROM Users AS u
  LEFT JOIN Votes AS v
    ON u.Id = v.UserId
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.DisplayName
), ClosedPosts AS (
  SELECT
    ph.PostId,
    MIN(ph.CreationDate) AS FirstClosedDate,
    STRING_AGG(DISTINCT c.Name, ', ') AS CloseReasons
  FROM PostHistory AS ph
  JOIN CloseReasonTypes AS c
    ON CAST(ph.Comment AS INT) = c.Id
  WHERE
    ph.PostHistoryTypeId = 10
  GROUP BY
    ph.PostId
)
SELECT
  rp.PostId,
  rp.Title,
  rp.ViewCount,
  rp.Score,
  us.DisplayName AS TopUser,
  us.Upvotes,
  us.Downvotes,
  cp.FirstClosedDate,
  cp.CloseReasons,
  COALESCE(us.BadgeCount, 0) AS BadgeCount
FROM RankedPosts AS rp
LEFT JOIN UserStats AS us
  ON rp.Score = us.Upvotes - us.Downvotes
LEFT JOIN ClosedPosts AS cp
  ON rp.PostId = cp.PostId
WHERE
  rp.Rank <= 10
  AND rp.Score > 0
  AND (
    cp.FirstClosedDate IS NULL
    OR cp.FirstClosedDate >= CAST('2024-10-01' AS DATE) - INTERVAL '3 MONTHS'
  )
ORDER BY
  rp.Score DESC,
  rp.ViewCount DESC;
