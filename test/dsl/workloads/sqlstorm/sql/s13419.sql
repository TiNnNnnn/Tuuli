SELECT
  p.Title AS PostTitle,
  p.CreationDate AS PostCreationDate,
  u.DisplayName AS AuthorDisplayName,
  COUNT(c.Id) AS CommentCount,
  SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
  SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes,
  p.ViewCount,
  p.Score,
  pt.Name AS PostTypeName
FROM Posts AS p
JOIN Users AS u
  ON p.OwnerUserId = u.Id
LEFT JOIN Comments AS c
  ON p.Id = c.PostId
LEFT JOIN Votes AS v
  ON p.Id = v.PostId
JOIN PostTypes AS pt
  ON p.PostTypeId = pt.Id
WHERE
  p.CreationDate >= '2022-01-01'
GROUP BY
  p.Title,
  p.CreationDate,
  u.DisplayName,
  p.ViewCount,
  p.Score,
  pt.Name
ORDER BY
  p.CreationDate DESC
LIMIT 100;
