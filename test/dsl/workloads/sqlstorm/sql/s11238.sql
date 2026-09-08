WITH UserActivity AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    COUNT(DISTINCT P.Id) AS TotalPosts,
    SUM(CASE WHEN P.PostTypeId = 1 THEN 1 ELSE 0 END) AS TotalQuestions,
    SUM(CASE WHEN P.PostTypeId = 2 THEN 1 ELSE 0 END) AS TotalAnswers,
    COUNT(C) AS TotalComments,
    SUM(CASE WHEN NOT V.CreationDate IS NULL THEN 1 ELSE 0 END) AS TotalVotes,
    SUM(CASE WHEN NOT B.Id IS NULL THEN 1 ELSE 0 END) AS TotalBadges
  FROM Users AS U
  LEFT JOIN Posts AS P
    ON U.Id = P.OwnerUserId
  LEFT JOIN Comments AS C
    ON P.Id = C.PostId
  LEFT JOIN Votes AS V
    ON P.Id = V.PostId
  LEFT JOIN Badges AS B
    ON U.Id = B.UserId
  GROUP BY
    U.Id,
    U.DisplayName
)
SELECT
  UA.UserId,
  UA.DisplayName,
  UA.TotalPosts,
  UA.TotalQuestions,
  UA.TotalAnswers,
  UA.TotalComments,
  UA.TotalVotes,
  UA.TotalBadges,
  COALESCE(RANK() OVER (ORDER BY UA.TotalPosts DESC), 0) AS UserRank
FROM UserActivity AS UA
ORDER BY
  UA.TotalPosts DESC
FETCH FIRST 100 ROWS ONLY;
