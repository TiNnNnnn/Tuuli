SELECT
  U.Id AS UserId,
  U.DisplayName,
  U.Reputation,
  COUNT(DISTINCT P.Id) AS TotalPosts,
  COUNT(DISTINCT C.Id) AS TotalComments,
  SUM(CASE WHEN V.VoteTypeId = 2 THEN 1 ELSE 0 END) AS TotalUpVotes,
  SUM(CASE WHEN V.VoteTypeId = 3 THEN 1 ELSE 0 END) AS TotalDownVotes,
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
WHERE
  U.Reputation > 0
GROUP BY
  U.Id,
  U.DisplayName,
  U.Reputation
ORDER BY
  TotalPosts DESC,
  U.Reputation DESC
LIMIT 100;
