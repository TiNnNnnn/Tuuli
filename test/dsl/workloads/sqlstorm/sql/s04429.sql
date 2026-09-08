WITH UserStats AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    U.Reputation,
    COALESCE(SUM(CASE WHEN V.VoteTypeId = 2 THEN 1 ELSE 0 END), 0) AS Upvotes,
    COALESCE(SUM(CASE WHEN V.VoteTypeId = 3 THEN 1 ELSE 0 END), 0) AS Downvotes,
    COUNT(DISTINCT P.Id) AS PostCount
  FROM Users AS U
  LEFT JOIN Posts AS P
    ON U.Id = P.OwnerUserId
  LEFT JOIN Votes AS V
    ON P.Id = V.PostId
  GROUP BY
    U.Id,
    U.DisplayName,
    U.Reputation
), PostActivity AS (
  SELECT
    P.Id AS PostId,
    P.Title,
    P.CreationDate,
    COUNT(C) AS CommentCount,
    ROW_NUMBER() OVER (PARTITION BY P.OwnerUserId ORDER BY P.LastActivityDate DESC) AS ActivityRank
  FROM Posts AS P
  LEFT JOIN Comments AS C
    ON P.Id = C.PostId
  WHERE
    P.CreationDate >= (
      CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
    )
  GROUP BY
    P.Id,
    P.Title,
    P.CreationDate
), TopUsers AS (
  SELECT
    U.Id,
    U.DisplayName,
    US.Reputation,
    ROW_NUMBER() OVER (ORDER BY US.Reputation DESC) AS Ranking
  FROM UserStats AS US
  JOIN Users AS U
    ON US.UserId = U.Id
  WHERE
    US.Upvotes > US.Downvotes
)
SELECT
  U.DisplayName,
  U.Reputation,
  PA.Title,
  PA.CommentCount,
  PA.CreationDate AS PostCreationDate,
  COALESCE(LT.Name, 'No Link') AS LinkType,
  CASE WHEN PA.CommentCount > 0 THEN 'Active' ELSE 'Inactive' END AS ActivityStatus
FROM TopUsers AS U
LEFT JOIN PostActivity AS PA
  ON PA.PostId IN (
    SELECT
      PL.PostId
    FROM PostLinks AS PL
    WHERE
      PL.LinkTypeId = 1 AND PL.RelatedPostId = PA.PostId
  )
LEFT JOIN LinkTypes AS LT
  ON LT.Id = (
    SELECT
      PL.LinkTypeId
    FROM PostLinks AS PL
    WHERE
      PL.PostId = PA.PostId
    LIMIT 1
  )
WHERE
  U.Ranking <= 10
ORDER BY
  U.Reputation DESC,
  PA.CreationDate DESC;
