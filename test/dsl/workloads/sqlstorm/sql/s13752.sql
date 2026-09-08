SELECT
  U.DisplayName AS UserName,
  P.Title AS PostTitle,
  P.CreationDate AS PostCreationDate,
  P.ViewCount,
  P.Score,
  COUNT(C.ID) AS CommentCount,
  SUM(CASE WHEN V.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
  SUM(CASE WHEN V.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes
FROM Users AS U
JOIN Posts AS P
  ON U.Id = P.OwnerUserId
LEFT JOIN Comments AS C
  ON P.Id = C.PostId
LEFT JOIN Votes AS V
  ON P.Id = V.PostId
WHERE
  P.PostTypeId = 1
GROUP BY
  U.DisplayName,
  P.Title,
  P.CreationDate,
  P.ViewCount,
  P.Score
ORDER BY
  P.Score DESC,
  P.ViewCount DESC
LIMIT 100;
