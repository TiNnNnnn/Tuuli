SELECT
  P.Id AS PostID,
  P.Title,
  P.CreationDate,
  U.DisplayName AS Author,
  COUNT(C.Id) AS CommentCount
FROM Posts AS P
JOIN Users AS U
  ON P.OwnerUserId = U.Id
LEFT JOIN Comments AS C
  ON P.Id = C.PostId
WHERE
  P.PostTypeId = 1
GROUP BY
  P.Id,
  P.Title,
  P.CreationDate,
  U.DisplayName
ORDER BY
  P.CreationDate DESC
LIMIT 10;
