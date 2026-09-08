SELECT
  U.DisplayName AS UserName,
  P.Title AS PostTitle,
  P.CreationDate AS PostDate,
  P.Score AS PostScore,
  COUNT(C.Id) AS CommentCount
FROM Posts AS P
JOIN Users AS U
  ON P.OwnerUserId = U.Id
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
  PostScore DESC
LIMIT 10;
