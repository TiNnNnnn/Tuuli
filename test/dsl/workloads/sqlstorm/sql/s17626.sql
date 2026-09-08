SELECT
  P.Id AS PostId,
  P.Title,
  P.CreationDate,
  U.DisplayName AS OwnerDisplayName,
  P.Score,
  P.ViewCount,
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
  U.DisplayName,
  P.Score,
  P.ViewCount
ORDER BY
  P.CreationDate DESC
LIMIT 10;
