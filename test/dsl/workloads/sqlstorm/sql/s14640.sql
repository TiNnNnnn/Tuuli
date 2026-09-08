SELECT
  p.Id AS PostId,
  p.Title,
  p.CreationDate,
  u.DisplayName AS OwnerDisplayName,
  COUNT(c.Id) AS CommentCount,
  SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVoteCount,
  SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVoteCount,
  COUNT(DISTINCT b.Id) AS BadgeCount,
  COUNT(DISTINCT pl.RelatedPostId) AS RelatedPostsCount
FROM Posts AS p
LEFT JOIN Users AS u
  ON p.OwnerUserId = u.Id
LEFT JOIN Comments AS c
  ON p.Id = c.PostId
LEFT JOIN Votes AS v
  ON p.Id = v.PostId
LEFT JOIN Badges AS b
  ON u.Id = b.UserId
LEFT JOIN PostLinks AS pl
  ON p.Id = pl.PostId
WHERE
  p.PostTypeId = 1
GROUP BY
  p.Id,
  p.Title,
  p.CreationDate,
  u.DisplayName
ORDER BY
  p.CreationDate DESC;
