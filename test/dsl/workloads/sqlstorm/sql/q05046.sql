WITH RankedPosts AS (
  SELECT
    P.Id AS PostId,
    P.Title,
    P.CreationDate,
    P.Score,
    P.ViewCount,
    U.DisplayName AS Owner,
    COUNT(CASE WHEN NOT C.PostId IS NULL THEN 1 END) AS CommentCount,
    RANK() OVER (PARTITION BY P.PostTypeId ORDER BY P.Score DESC) AS RankByScore
  FROM Posts AS P
  LEFT JOIN Users AS U
    ON P.OwnerUserId = U.Id
  LEFT JOIN Comments AS C
    ON P.Id = C.PostId
  WHERE
    P.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
  GROUP BY
    P.Id,
    P.Title,
    P.CreationDate,
    P.Score,
    P.ViewCount,
    U.DisplayName,
    P.PostTypeId
), TopPosts AS (
  SELECT
    RP.PostId,
    RP.Title,
    RP.CreationDate,
    RP.Score,
    RP.ViewCount,
    RP.Owner,
    RP.CommentCount
  FROM RankedPosts AS RP
  WHERE
    RP.RankByScore <= 10
)
SELECT
  TP.PostId,
  TP.Title,
  TP.CreationDate,
  TP.Score,
  TP.ViewCount,
  TP.Owner,
  TP.CommentCount,
  (
    SELECT
      STRING_AGG(T.TagName, ', ')
    FROM Tags AS T
    JOIN LATERAL UNNEST(STRING_TO_ARRAY(SUBSTRING(P.Tags FROM 2 FOR LENGTH(P.Tags) - 2), '><')) AS tag
      ON T.TagName = tag
    WHERE
      P.Id = TP.PostId
  ) AS TagsList
FROM TopPosts AS TP
LEFT JOIN Posts AS P
  ON TP.PostId = P.Id
ORDER BY
  TP.Score DESC,
  TP.ViewCount DESC;
