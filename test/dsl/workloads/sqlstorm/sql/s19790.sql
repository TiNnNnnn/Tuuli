SELECT
  p.Id AS PostId,
  p.Title,
  p.CreationDate,
  u.DisplayName AS OwnerDisplayName,
  p.Score,
  p.ViewCount,
  p.AnswerCount
FROM Posts AS p
JOIN Users AS u
  ON p.OwnerUserId = u.Id
WHERE
  p.PostTypeId = 1
ORDER BY
  p.CreationDate DESC
LIMIT 10;
