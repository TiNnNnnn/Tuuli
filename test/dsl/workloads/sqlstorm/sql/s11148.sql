WITH PostStatistics AS (
  SELECT
    P.Id AS PostId,
    P.Title,
    P.CreationDate,
    P.ViewCount,
    P.Score,
    COUNT(CASE WHEN NOT C.Id IS NULL THEN 1 END) AS CommentCount,
    COUNT(CASE WHEN NOT A.Id IS NULL THEN 1 END) AS AnswerCount,
    MAX(CASE WHEN NOT V.Id IS NULL THEN 1 ELSE 0 END) AS HasVote,
    MAX(V.CreationDate) AS LastVoteDate
  FROM Posts AS P
  LEFT JOIN Comments AS C
    ON P.Id = C.PostId
  LEFT JOIN Posts AS A
    ON P.Id = A.ParentId
  LEFT JOIN Votes AS V
    ON P.Id = V.PostId
  WHERE
    P.CreationDate >= CAST('2024-10-01' AS DATE) - INTERVAL '1 YEAR'
  GROUP BY
    P.Id,
    P.Title,
    P.CreationDate,
    P.ViewCount,
    P.Score
)
SELECT
  PS.PostId,
  PS.Title,
  PS.CreationDate,
  PS.ViewCount,
  PS.Score,
  PS.CommentCount,
  PS.AnswerCount,
  PS.HasVote,
  PS.LastVoteDate,
  U.DisplayName AS AuthorDisplayName,
  U.Reputation AS AuthorReputation
FROM PostStatistics AS PS
JOIN Users AS U
  ON PS.PostId = U.Id
ORDER BY
  PS.Score DESC,
  PS.ViewCount DESC;
