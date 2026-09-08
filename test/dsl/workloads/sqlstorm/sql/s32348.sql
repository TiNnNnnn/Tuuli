WITH RECURSIVE RecursivePostHierarchy AS (
  SELECT
    Id AS PostId,
    Title,
    ParentId,
    CreationDate,
    0 AS Level
  FROM Posts
  WHERE
    ParentId IS NULL
  UNION ALL
  SELECT
    p.Id AS PostId,
    p.Title,
    p.ParentId,
    p.CreationDate,
    r.Level + 1
  FROM Posts AS p
  INNER JOIN RecursivePostHierarchy AS r
    ON p.ParentId = r.PostId
), PostStats AS (
  SELECT
    p.Id AS PostId,
    COUNT(c.Id) AS CommentCount,
    COUNT(DISTINCT v.UserId) FILTER(WHERE
      v.VoteTypeId = 2) AS UpvoteCount,
    COUNT(DISTINCT v.UserId) FILTER(WHERE
      v.VoteTypeId = 3) AS DownvoteCount,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 WHEN v.VoteTypeId = 3 THEN -1 ELSE 0 END) AS NetVotes,
    MAX(ph.CreationDate) AS LastHistoryUpdate
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  LEFT JOIN PostHistory AS ph
    ON p.Id = ph.PostId
  GROUP BY
    p.Id
), UserBadges AS (
  SELECT
    u.Id AS UserId,
    COUNT(b.Id) FILTER(WHERE
      b.Class = 1) AS GoldBadges,
    COUNT(b.Id) FILTER(WHERE
      b.Class = 2) AS SilverBadges,
    COUNT(b.Id) FILTER(WHERE
      b.Class = 3) AS BronzeBadges,
    SUM(CASE WHEN b.TagBased THEN 1 ELSE 0 END) AS TagBasedBadges
  FROM Users AS u
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id
)
SELECT
  p.Title AS PostTitle,
  p.CreationDate AS PostCreationDate,
  ps.CommentCount,
  ps.UpvoteCount,
  ps.DownvoteCount,
  ps.NetVotes,
  COALESCE(u.DisplayName, 'Unknown User') AS OwnerDisplayName,
  ub.GoldBadges,
  ub.SilverBadges,
  ub.BronzeBadges,
  ph.Level AS PostLevel,
  ph.ParentId AS ParentPostId,
  CASE
    WHEN NOT ps.LastHistoryUpdate IS NULL
    AND ps.LastHistoryUpdate < CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
    THEN 'Stale Post'
    ELSE 'Active Post'
  END AS PostStatus
FROM Posts AS p
LEFT JOIN PostStats AS ps
  ON p.Id = ps.PostId
LEFT JOIN Users AS u
  ON p.OwnerUserId = u.Id
LEFT JOIN UserBadges AS ub
  ON u.Id = ub.UserId
LEFT JOIN RecursivePostHierarchy AS ph
  ON p.Id = ph.PostId
WHERE
  p.CreationDate > CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
ORDER BY
  ps.NetVotes DESC,
  ps.CommentCount DESC
LIMIT 50;
