SELECT
  p.Id AS PostId,
  p.Title,
  p.CreationDate,
  u.DisplayName AS UserDisplayName,
  pt.Name AS PostType,
  COUNT(c.Id) AS CommentCount
FROM Posts AS p
JOIN Users AS u
  ON p.OwnerUserId = u.Id
JOIN PostTypes AS pt
  ON p.PostTypeId = pt.Id
LEFT JOIN Comments AS c
  ON p.Id = c.PostId
GROUP BY
  p.Id,
  p.Title,
  p.CreationDate,
  u.DisplayName,
  pt.Name
ORDER BY
  p.CreationDate DESC
LIMIT 10;
