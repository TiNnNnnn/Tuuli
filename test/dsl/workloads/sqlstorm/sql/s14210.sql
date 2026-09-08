SELECT
  u.Id AS UserId,
  u.DisplayName,
  u.Reputation,
  COUNT(DISTINCT p.Id) AS TotalPosts,
  COUNT(DISTINCT c.Id) AS TotalComments,
  COUNT(DISTINCT v.Id) AS TotalVotes,
  COUNT(DISTINCT b.Id) AS TotalBadges,
  SUM(p.ViewCount) AS TotalPostViews
FROM Users AS u
LEFT JOIN Posts AS p
  ON u.Id = p.OwnerUserId
LEFT JOIN Comments AS c
  ON p.Id = c.PostId
LEFT JOIN Votes AS v
  ON p.Id = v.PostId
LEFT JOIN Badges AS b
  ON u.Id = b.UserId
GROUP BY
  u.Id,
  u.DisplayName,
  u.Reputation
ORDER BY
  u.Reputation DESC;
