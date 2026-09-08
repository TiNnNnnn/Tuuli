SELECT
  p.Id AS PostId,
  p.Title,
  p.CreationDate,
  p.Score,
  p.ViewCount,
  u.DisplayName AS OwnerDisplayName,
  u.Reputation AS OwnerReputation,
  COUNT(c.Id) AS CommentCount,
  COUNT(v.Id) AS VoteCount,
  SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
  SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes
FROM Posts AS p
LEFT JOIN Users AS u
  ON p.OwnerUserId = u.Id
LEFT JOIN Comments AS c
  ON p.Id = c.PostId
LEFT JOIN Votes AS v
  ON p.Id = v.PostId
WHERE
  p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
GROUP BY
  p.Id,
  p.Title,
  p.CreationDate,
  p.Score,
  p.ViewCount,
  u.DisplayName,
  u.Reputation
ORDER BY
  p.CreationDate DESC
LIMIT 100;
