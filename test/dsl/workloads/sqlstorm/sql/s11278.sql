SELECT
  U.Id AS UserId,
  U.Reputation,
  P.Title,
  COUNT(C.Id) AS CommentCount
FROM Users AS U
JOIN Posts AS P
  ON U.Id = P.OwnerUserId
LEFT JOIN Comments AS C
  ON P.Id = C.PostId
GROUP BY
  U.Id,
  U.Reputation,
  P.Title
ORDER BY
  U.Reputation DESC,
  CommentCount DESC
LIMIT 100;
