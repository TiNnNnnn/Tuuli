SELECT
  pt.Name AS PostType,
  COUNT(p.Id) AS TotalPosts,
  AVG(p.Score) AS AverageScore
FROM Posts AS p
JOIN PostTypes AS pt
  ON p.PostTypeId = pt.Id
GROUP BY
  pt.Name
ORDER BY
  TotalPosts DESC;
