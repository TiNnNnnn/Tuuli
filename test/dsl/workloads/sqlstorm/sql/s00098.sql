WITH UserStats AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    U.Reputation,
    COUNT(DISTINCT P.Id) AS PostCount,
    SUM(CASE WHEN NOT V.CreationDate IS NULL THEN 1 ELSE 0 END) AS VoteCount,
    AVG(COALESCE(P.Score, 0)) AS AvgScore
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
    PostCount,
    VoteCount,
    AvgScore,
    RANK() OVER (ORDER BY PostCount DESC) AS Rank
  FROM UserStats
)
SELECT
  TU.DisplayName,
  TU.PostCount,
  TU.VoteCount,
  TU.AvgScore,
  CASE
    WHEN TU.AvgScore > 50
    THEN 'High Scorer'
    WHEN TU.AvgScore BETWEEN 20 AND 50
    THEN 'Medium Scorer'
    ELSE 'Low Scorer'
  END AS ScoreCategory,
  COALESCE(PH.Comment, 'No comments') AS PostHistoryComment
FROM TopUsers AS TU
LEFT JOIN PostHistory AS PH
  ON TU.UserId = PH.UserId
  AND PH.CreationDate = (
    SELECT
      MAX(PH2.CreationDate)
    FROM PostHistory AS PH2
    WHERE
      PH2.UserId = TU.UserId
  )
WHERE
  TU.Rank <= 10
ORDER BY
  TU.Rank;
