SELECT
  U.DisplayName AS UserDisplayName,
  P.Title AS PostTitle,
  P.CreationDate AS PostCreationDate,
  P.Score AS PostScore,
  COUNT(C.Id) AS CommentCount
FROM Users AS U
JOIN Posts AS P
  ON U.Id = P.OwnerUserId
LEFT JOIN Comments AS C
  ON P.Id = C.PostId
WHERE
  P.PostTypeId = 1
GROUP BY
  U.DisplayName,
  P.Title,
  P.CreationDate,
  P.Score
ORDER BY
  P.CreationDate DESC
LIMIT 10;
