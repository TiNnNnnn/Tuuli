WITH UserStatistics AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    U.Reputation,
    COALESCE(SUM(CASE WHEN P.PostTypeId = 1 THEN 1 ELSE 0 END), 0) AS QuestionCount,
    COALESCE(SUM(CASE WHEN P.PostTypeId = 2 THEN 1 ELSE 0 END), 0) AS AnswerCount,
    COALESCE(SUM(CASE WHEN V.VoteTypeId = 2 THEN 1 ELSE 0 END), 0) AS UpVoteCount,
    COALESCE(SUM(CASE WHEN V.VoteTypeId = 3 THEN 1 ELSE 0 END), 0) AS DownVoteCount
  FROM Users AS U
  LEFT JOIN Posts AS P
    ON U.Id = P.OwnerUserId
  LEFT JOIN Votes AS V
    ON P.Id = V.PostId
  GROUP BY
    U.Id,
    U.DisplayName,
    U.Reputation
), TopUsers AS (
  SELECT
    UserId,
    DisplayName,
    Reputation,
    QuestionCount,
    AnswerCount,
    UpVoteCount,
    DownVoteCount,
    ROW_NUMBER() OVER (ORDER BY Reputation DESC) AS Rank
  FROM UserStatistics
), PostDetails AS (
  SELECT
    P.Id AS PostId,
    P.Title,
    P.CreationDate,
    P.OwnerUserId,
    CASE WHEN NOT PH.PostId IS NULL THEN 'Closed' ELSE 'Open' END AS PostStatus,
    COUNT(CASE WHEN NOT C.Id IS NULL THEN 1 END) AS CommentCount,
    COUNT(DISTINCT PL.RelatedPostId) AS RelatedPostCount
  FROM Posts AS P
  LEFT JOIN PostHistory AS PH
    ON P.Id = PH.PostId AND PH.PostHistoryTypeId IN (10, 11)
  LEFT JOIN Comments AS C
    ON P.Id = C.PostId
  LEFT JOIN PostLinks AS PL
    ON P.Id = PL.PostId
  GROUP BY
    P.Id,
    P.Title,
    P.CreationDate,
    P.OwnerUserId,
    PH.PostId
)
SELECT
  TU.DisplayName,
  TU.Reputation,
  PD.PostId,
  PD.Title,
  PD.CreationDate,
  PD.PostStatus,
  PD.CommentCount,
  PD.RelatedPostCount
FROM TopUsers AS TU
JOIN PostDetails AS PD
  ON TU.UserId = PD.OwnerUserId
WHERE
  TU.Rank <= 10
ORDER BY
  TU.Reputation DESC,
  PD.PostStatus;
