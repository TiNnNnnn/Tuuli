SELECT
  u.DisplayName AS UserDisplayName,
  p.Title AS PostTitle,
  p.CreationDate AS PostCreationDate,
  p.Score AS PostScore,
  c.Text AS CommentText,
  c.CreationDate AS CommentCreationDate
FROM Posts AS p
JOIN Users AS u
  ON p.OwnerUserId = u.Id
LEFT JOIN Comments AS c
  ON p.Id = c.PostId
WHERE
  p.PostTypeId = 1
ORDER BY
  p.CreationDate DESC
LIMIT 10;
