WITH UserReputation AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    U.Reputation,
    COUNT(DISTINCT P.Id) AS QuestionsAsked,
    COUNT(DISTINCT A.Id) AS AnswersProvided,
    SUM(CASE WHEN V.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpvotesReceived,
    SUM(CASE WHEN V.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownvotesReceived
  FROM Users AS U
  LEFT JOIN Posts AS P
    ON U.Id = P.OwnerUserId AND P.PostTypeId = 1
  LEFT JOIN Posts AS A
    ON A.ParentId = P.Id AND A.PostTypeId = 2
  LEFT JOIN Votes AS V
    ON V.UserId = U.Id
    AND V.PostId IN (
      SELECT
        Id
      FROM Posts
      WHERE
        OwnerUserId = U.Id
    )
  GROUP BY
    U.Id,
    U.DisplayName,
    U.Reputation
), TopTags AS (
  SELECT
    T.TagName,
    COUNT(P.Id) AS PostsCount,
    SUM(CASE WHEN P.PostTypeId = 1 THEN 1 ELSE 0 END) AS QuestionsCount,
    SUM(CASE WHEN P.PostTypeId = 2 THEN 1 ELSE 0 END) AS AnswersCount
  FROM Tags AS T
  LEFT JOIN Posts AS P
    ON P.Tags LIKE CONCAT('%', T.TagName, '%')
  GROUP BY
    T.TagName
  ORDER BY
    PostsCount DESC
  LIMIT 10
), UserBadgeCount AS (
  SELECT
    U.Id AS UserId,
    COUNT(B.Id) AS BadgeCount
  FROM Users AS U
  LEFT JOIN Badges AS B
    ON U.Id = B.UserId
  GROUP BY
    U.Id
)
SELECT
  UR.DisplayName,
  UR.Reputation,
  UR.QuestionsAsked,
  UR.AnswersProvided,
  UR.UpvotesReceived,
  UR.DownvotesReceived,
  UBC.BadgeCount,
  TT.TagName,
  TT.PostsCount,
  TT.QuestionsCount,
  TT.AnswersCount
FROM UserReputation AS UR
JOIN UserBadgeCount AS UBC
  ON UR.UserId = UBC.UserId
JOIN TopTags AS TT
  ON TT.QuestionsCount > 0
ORDER BY
  UR.Reputation DESC,
  UBC.BadgeCount DESC
FETCH FIRST 20 ROWS ONLY;
