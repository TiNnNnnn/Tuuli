SELECT
  p.Title,
  p.CreationDate,
  p.ViewCount,
  p.Score,
  u.DisplayName AS OwnerDisplayName
FROM Posts AS p
JOIN Users AS u
  ON p.OwnerUserId = u.Id
WHERE
  p.PostTypeId = 1
ORDER BY
  p.CreationDate DESC
LIMIT 10;
