WITH UserStats AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    U.Reputation,
    COUNT(DISTINCT P.Id) AS PostCount,
    COUNT(DISTINCT C.Id) AS CommentCount,
    SUM(CASE WHEN V.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVoteCount,
    SUM(CASE WHEN V.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVoteCount
  FROM Users AS U
  LEFT JOIN Posts AS P
    ON U.Id = P.OwnerUserId
  LEFT JOIN Comments AS C
    ON P.Id = C.PostId
  LEFT JOIN Votes AS V
    ON P.Id = V.PostId
  GROUP BY
    U.Id,
    U.DisplayName,
    U.Reputation
), PostStats AS (
  SELECT
    P.Id AS PostId,
    P.Title,
    P.CreationDate,
    P.OwnerUserId,
    P.ViewCount,
    P.Score,
    P.AnswerCount,
    P.CommentCount,
    CASE
      WHEN P.PostTypeId = 1
      THEN 'Question'
      WHEN P.PostTypeId = 2
      THEN 'Answer'
      ELSE 'Other'
    END AS PostType,
    COUNT(CASE WHEN V.VoteTypeId = 2 THEN 1 END) AS UpVotes,
    COUNT(CASE WHEN V.VoteTypeId = 3 THEN 1 END) AS DownVotes
  FROM Posts AS P
  LEFT JOIN Votes AS V
    ON P.Id = V.PostId
  GROUP BY
    P.Id,
    P.Title,
    P.CreationDate,
    P.OwnerUserId,
    P.ViewCount,
    P.Score,
    P.AnswerCount,
    P.CommentCount,
    P.PostTypeId
)
SELECT
  U.DisplayName,
  U.Reputation,
  U.PostCount,
  U.CommentCount,
  U.UpVoteCount,
  U.DownVoteCount,
  P.PostId,
  P.Title,
  P.CreationDate,
  P.ViewCount,
  P.Score,
  P.AnswerCount,
  P.CommentCount,
  P.PostType,
  P.UpVotes,
  P.DownVotes
FROM UserStats AS U
JOIN PostStats AS P
  ON U.UserId = P.OwnerUserId
ORDER BY
  U.Reputation DESC,
  P.ViewCount DESC;
