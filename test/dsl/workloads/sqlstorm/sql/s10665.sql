WITH UserStats AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    U.Reputation,
    COUNT(DISTINCT P.Id) AS PostCount,
    SUM(CASE WHEN P.PostTypeId = 1 THEN 1 ELSE 0 END) AS QuestionCount,
    SUM(CASE WHEN P.PostTypeId = 2 THEN 1 ELSE 0 END) AS AnswerCount,
    SUM(CASE WHEN NOT C.Id IS NULL THEN 1 ELSE 0 END) AS CommentCount
  FROM Users AS U
  LEFT JOIN Posts AS P
    ON U.Id = P.OwnerUserId
  LEFT JOIN Comments AS C
    ON P.Id = C.PostId
  WHERE
    U.Reputation > 0
  GROUP BY
    U.Id,
    U.DisplayName,
    U.Reputation
), PostDetails AS (
  SELECT
    P.Id AS PostId,
    P.Title,
    P.CreationDate,
    P.Score,
    P.ViewCount,
    PT.Name AS PostType,
    PH.CreationDate AS HistoryDate,
    P.OwnerUserId
  FROM Posts AS P
  JOIN PostTypes AS PT
    ON P.PostTypeId = PT.Id
  LEFT JOIN PostHistory AS PH
    ON P.Id = PH.PostId
)
SELECT
  US.UserId,
  US.DisplayName,
  US.Reputation,
  US.PostCount,
  US.QuestionCount,
  US.AnswerCount,
  US.CommentCount,
  PD.PostId,
  PD.Title AS PostTitle,
  PD.CreationDate AS PostCreationDate,
  PD.Score AS PostScore,
  PD.ViewCount AS PostViewCount,
  PD.PostType,
  COUNT(PD.HistoryDate) AS HistoryCount
FROM UserStats AS US
LEFT JOIN PostDetails AS PD
  ON US.UserId = PD.OwnerUserId
GROUP BY
  US.UserId,
  US.DisplayName,
  US.Reputation,
  US.PostCount,
  US.QuestionCount,
  US.AnswerCount,
  US.CommentCount,
  PD.PostId,
  PD.Title,
  PD.CreationDate,
  PD.Score,
  PD.ViewCount,
  PD.PostType
ORDER BY
  US.Reputation DESC,
  US.PostCount DESC;
