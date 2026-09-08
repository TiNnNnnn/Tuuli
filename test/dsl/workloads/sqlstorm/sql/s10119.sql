SELECT
  pt.Name AS PostType,
  COUNT(p.Id) AS TotalPosts,
  AVG(EXTRACT(EPOCH FROM (
    p.LastActivityDate - p.CreationDate
  ))) AS AvgTimeInSeconds
FROM Posts AS p
JOIN PostTypes AS pt
  ON p.PostTypeId = pt.Id
GROUP BY
  pt.Name
ORDER BY
  TotalPosts DESC;
