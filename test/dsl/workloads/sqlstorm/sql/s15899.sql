SELECT
  p.Title,
  p.CreationDate,
  u.DisplayName AS OwnerName,
  COUNT(c.Id) AS CommentCount,
  SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVoteCount
FROM Posts AS p
JOIN Users AS u
  ON p.OwnerUserId = u.Id
LEFT JOIN Comments AS c
  ON p.Id = c.PostId
LEFT JOIN Votes AS v
  ON p.Id = v.PostId
WHERE
  p.PostTypeId = 1
GROUP BY
  p.Title,
  p.CreationDate,
  u.DisplayName
ORDER BY
  p.CreationDate DESC
LIMIT 10;
