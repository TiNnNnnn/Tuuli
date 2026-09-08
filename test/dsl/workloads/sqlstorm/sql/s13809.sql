WITH UserStats AS (
  SELECT
    u.Id AS UserId,
    u.Reputation,
    COUNT(DISTINCT p.Id) AS PostCount,
    SUM(COALESCE(p.Score, 0)) AS TotalScore,
    SUM(COALESCE(p.ViewCount, 0)) AS TotalViews,
    SUM(CASE WHEN NOT b.Id IS NULL THEN 1 ELSE 0 END) AS BadgeCount
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.Reputation
), PostStats AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    COUNT(c.Id) AS CommentCount,
    COUNT(DISTINCT l.RelatedPostId) AS LinkedPostCount
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN PostLinks AS l
    ON p.Id = l.PostId
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate
)
SELECT
  us.UserId,
  us.Reputation,
  us.PostCount,
  us.TotalScore,
  us.TotalViews,
  us.BadgeCount,
  ps.PostId,
  ps.Title,
  ps.CreationDate,
  ps.CommentCount,
  ps.LinkedPostCount
FROM UserStats AS us
JOIN PostStats AS ps
  ON us.UserId = ps.PostId
ORDER BY
  us.Reputation DESC,
  ps.CreationDate DESC;
