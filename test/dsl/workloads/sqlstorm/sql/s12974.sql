SELECT
  pt.Name AS PostType,
  COUNT(p.Id) AS PostCount,
  AVG(p.Score) AS AverageScore,
  SUM(p.ViewCount) AS TotalViewCount,
  COUNT(DISTINCT p.OwnerUserId) AS UniqueUsers
FROM Posts AS p
JOIN PostTypes AS pt
  ON p.PostTypeId = pt.Id
JOIN Users AS u
  ON p.OwnerUserId = u.Id
GROUP BY
  pt.Name
ORDER BY
  PostCount DESC;
