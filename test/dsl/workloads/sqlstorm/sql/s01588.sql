WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.CreationDate DESC) AS rn,
    COUNT(c.Id) AS CommentCount
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.OwnerUserId
), TopContributors AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    SUM(v.BountyAmount) AS TotalBounties,
    COUNT(DISTINCT p.Id) AS PostCount
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId AND v.VoteTypeId IN (8, 9)
  GROUP BY
    u.Id,
    u.DisplayName
  HAVING
    COUNT(DISTINCT p.Id) > 10
), RecentPostHistory AS (
  SELECT
    ph.PostId,
    ph.CreationDate,
    ph.UserId,
    ph.Comment,
    RANK() OVER (PARTITION BY ph.PostId ORDER BY ph.CreationDate DESC) AS rank
  FROM PostHistory AS ph
  WHERE
    ph.PostHistoryTypeId IN (10, 12)
), TopPostDetails AS (
  SELECT
    rp.PostId,
    rp.Title,
    rp.CreationDate,
    rp.CommentCount,
    COALESCE(rph.UserId, -1) AS LastActionUserId,
    COALESCE(rph.Comment, 'No remarks.') AS LastActionComment
  FROM RankedPosts AS rp
  LEFT JOIN RecentPostHistory AS rph
    ON rp.PostId = rph.PostId AND rph.rank = 1
)
SELECT
  t.UserId,
  t.DisplayName,
  tp.PostId,
  tp.Title,
  tp.CreationDate,
  tp.CommentCount,
  tp.LastActionUserId,
  tp.LastActionComment,
  CASE
    WHEN tp.CommentCount > 5
    THEN 'Highly Discussed'
    WHEN tp.CommentCount BETWEEN 1 AND 5
    THEN 'Moderately Discussed'
    ELSE 'Rarely Discussed'
  END AS DiscussionLevel
FROM TopContributors AS t
JOIN TopPostDetails AS tp
  ON t.UserId = tp.LastActionUserId
ORDER BY
  t.TotalBounties DESC,
  tp.CommentCount DESC
LIMIT 50;
