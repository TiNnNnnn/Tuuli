SELECT
  pt.Name AS PostType,
  COUNT(p.Id) AS TotalPosts,
  AVG(u.Reputation) AS AverageUserReputation,
  SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS TotalUpVotes,
  COUNT(DISTINCT c.Id) AS TotalComments,
  COUNT(DISTINCT b.Id) AS TotalBadges
FROM Posts AS p
JOIN PostTypes AS pt
  ON p.PostTypeId = pt.Id
LEFT JOIN Users AS u
  ON p.OwnerUserId = u.Id
LEFT JOIN Votes AS v
  ON p.Id = v.PostId
LEFT JOIN Comments AS c
  ON p.Id = c.PostId
LEFT JOIN Badges AS b
  ON u.Id = b.UserId
WHERE
  p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
GROUP BY
  pt.Name,
  u.Reputation
ORDER BY
  TotalPosts DESC,
  AverageUserReputation DESC;
