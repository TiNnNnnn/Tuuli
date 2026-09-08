WITH UserActivity AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    COUNT(DISTINCT P.Id) AS TotalPosts,
    COUNT(DISTINCT C.Id) AS TotalComments,
    SUM(CASE WHEN V.VoteTypeId = 2 THEN 1 ELSE 0 END) AS TotalUpVotes,
    SUM(CASE WHEN V.VoteTypeId = 3 THEN 1 ELSE 0 END) AS TotalDownVotes,
    SUM(P.Score) AS TotalScore
  FROM Users AS U
  LEFT JOIN Posts AS P
    ON U.Id = P.OwnerUserId
  LEFT JOIN Comments AS C
    ON P.Id = C.PostId
  LEFT JOIN Votes AS V
    ON P.Id = V.PostId
  WHERE
    U.Reputation > 1000
  GROUP BY
    U.Id,
    U.DisplayName
), TopUsers AS (
  SELECT
    UserId,
    DisplayName,
    TotalPosts,
    TotalComments,
    TotalUpVotes,
    TotalDownVotes,
    TotalScore,
    RANK() OVER (ORDER BY TotalScore DESC) AS Rank
  FROM UserActivity
)
SELECT
  T.DisplayName,
  T.TotalPosts,
  T.TotalComments,
  T.TotalUpVotes,
  T.TotalDownVotes,
  T.TotalScore,
  T.Rank
FROM TopUsers AS T
WHERE
  T.Rank <= 10
ORDER BY
  T.Rank;
