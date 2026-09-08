SELECT
  p.Title,
  p.CreationDate,
  u.DisplayName AS Owner,
  t.TagName
FROM Posts AS p
JOIN Users AS u
  ON p.OwnerUserId = u.Id
JOIN Tags AS t
  ON t.ExcerptPostId = p.Id
WHERE
  p.PostTypeId = 1
ORDER BY
  p.CreationDate DESC
LIMIT 10;
