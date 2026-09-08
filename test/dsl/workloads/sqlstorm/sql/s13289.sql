SELECT
  P.Id AS PostId,
  P.Title,
  P.CreationDate AS PostCreationDate,
  P.ViewCount,
  P.Score,
  P.AnswerCount,
  P.CommentCount,
  U.DisplayName AS OwnerDisplayName,
  U.Reputation AS OwnerReputation,
  COUNT(C.Id) AS TotalComments,
  COUNT(V.Id) AS TotalVotes,
  SUM(CASE WHEN V.VoteTypeId = 2 THEN 1 ELSE 0 END) AS TotalUpvotes,
  SUM(CASE WHEN V.VoteTypeId = 3 THEN 1 ELSE 0 END) AS TotalDownvotes
FROM Posts AS P
JOIN Users AS U
  ON P.OwnerUserId = U.Id
LEFT JOIN Comments AS C
  ON P.Id = C.PostId
LEFT JOIN Votes AS V
  ON P.Id = V.PostId
WHERE
  P.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
GROUP BY
  P.Id,
  U.DisplayName,
  U.Reputation
ORDER BY
  P.CreationDate DESC;
