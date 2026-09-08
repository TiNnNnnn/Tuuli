SELECT
  p.Id AS PostID,
  p.Title,
  u.DisplayName AS OwnerDisplayName,
  p.CreationDate,
  p.Score,
  p.ViewCount,
  p.AnswerCount,
  p.CommentCount
FROM Posts AS p
JOIN Users AS u
  ON p.OwnerUserId = u.Id
WHERE
  p.PostTypeId = 1
ORDER BY
  p.CreationDate DESC
LIMIT 10;
