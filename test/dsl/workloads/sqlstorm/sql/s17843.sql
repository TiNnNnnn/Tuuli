SELECT
  p.Id AS PostId,
  p.Title,
  p.Score,
  p.CreationDate,
  u.DisplayName AS OwnerDisplayName,
  COUNT(c.Id) AS CommentCount
FROM Posts AS p
JOIN Users AS u
  ON p.OwnerUserId = u.Id
LEFT JOIN Comments AS c
  ON p.Id = c.PostId
WHERE
  p.PostTypeId = 1
GROUP BY
  p.Id,
  p.Title,
  p.Score,
  p.CreationDate,
  u.DisplayName
ORDER BY
  p.Score DESC
LIMIT 10;
