WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.OwnerUserId,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.Score DESC, p.CreationDate DESC) AS Rank
  FROM Posts AS p
  WHERE
    p.PostTypeId = 1 AND p.Score > 0
), UserMetrics AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    u.Reputation,
    COUNT(DISTINCT p.Id) AS QuestionCount,
    SUM(COALESCE(b.Class, 0)) AS TotalBadges,
    SUM(p.ViewCount) AS TotalViews
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId AND p.PostTypeId = 1
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.DisplayName,
    u.Reputation
), TopUserPosts AS (
  SELECT
    up.PostId,
    up.Title,
    up.Score,
    um.DisplayName,
    um.Reputation,
    um.TotalBadges,
    um.TotalViews
  FROM RankedPosts AS up
  JOIN UserMetrics AS um
    ON up.OwnerUserId = um.UserId
  WHERE
    up.Rank = 1
)
SELECT
  tup.PostId,
  tup.Title,
  tup.Score,
  tup.DisplayName,
  tup.Reputation,
  tup.TotalBadges,
  tup.TotalViews,
  (
    SELECT
      COUNT(*)
    FROM Comments AS c
    WHERE
      c.PostId = tup.PostId
  ) AS CommentCount,
  (
    SELECT
      COUNT(*)
    FROM Votes AS v
    WHERE
      v.PostId = tup.PostId AND v.VoteTypeId = 2
  ) AS UpVoteCount,
  (
    SELECT
      COUNT(*)
    FROM Votes AS v
    WHERE
      v.PostId = tup.PostId AND v.VoteTypeId = 3
  ) AS DownVoteCount
FROM TopUserPosts AS tup
ORDER BY
  tup.Score DESC,
  tup.Reputation DESC;
