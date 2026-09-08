SELECT
  pt.Name AS PostType,
  COUNT(p.Id) AS PostCount,
  AVG(p.Score) AS AverageScore,
  SUM(p.ViewCount) AS TotalViews
FROM Posts AS p
JOIN PostTypes AS pt
  ON p.PostTypeId = pt.Id
GROUP BY
  pt.Name
ORDER BY
  PostCount DESC;
