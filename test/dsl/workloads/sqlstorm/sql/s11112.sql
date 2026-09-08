WITH PostStats AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    COUNT(DISTINCT c.Id) AS CommentCount,
    COUNT(DISTINCT v.Id) AS VoteCount,
    COUNT(DISTINCT b.Id) AS BadgeCount,
    MAX(p.LastActivityDate) AS LastActivityDate
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  LEFT JOIN Badges AS b
    ON p.OwnerUserId = b.UserId
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate
), UserStats AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    SUM(p.ViewCount) AS TotalViews,
    SUM(v.BountyAmount) AS TotalBounties,
    SUM(p.Score) AS TotalScore,
    COUNT(DISTINCT p.Id) AS PostCount
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  GROUP BY
    u.Id,
    u.DisplayName
)
SELECT
  u.UserId,
  u.DisplayName,
  u.TotalViews,
  u.TotalBounties,
  u.TotalScore,
  u.PostCount,
  ps.PostId,
  ps.Title AS PostTitle,
  ps.CreationDate AS PostCreationDate,
  ps.CommentCount,
  ps.VoteCount,
  ps.BadgeCount,
  ps.LastActivityDate
FROM UserStats AS u
JOIN PostStats AS ps
  ON u.UserId = ps.PostId
ORDER BY
  u.TotalScore DESC,
  ps.LastActivityDate DESC
LIMIT 100;
