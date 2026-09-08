WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.AnswerCount,
    p.CommentCount,
    U.DisplayName AS OwnerDisplayName,
    RANK() OVER (PARTITION BY p.OwnerUserId ORDER BY p.Score DESC, p.CreationDate DESC) AS RankByScore
  FROM Posts AS p
  JOIN Users AS U
    ON p.OwnerUserId = U.Id
  WHERE
    p.PostTypeId = 1
    AND p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
), TopQuestions AS (
  SELECT
    *
  FROM RankedPosts
  WHERE
    RankByScore = 1
), UserScores AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    SUM(p.Score) AS TotalScore,
    COUNT(DISTINCT p.Id) AS QuestionCount
  FROM Users AS U
  JOIN Posts AS p
    ON U.Id = p.OwnerUserId
  WHERE
    p.PostTypeId = 1
  GROUP BY
    U.Id,
    U.DisplayName
), BadgeCounts AS (
  SELECT
    B.UserId,
    COUNT(B.Id) AS BadgeTotal
  FROM Badges AS B
  WHERE
    B.Class = 1
  GROUP BY
    B.UserId
)
SELECT
  U.DisplayName,
  US.TotalScore,
  US.QuestionCount,
  COALESCE(BC.BadgeTotal, 0) AS GoldBadges,
  COUNT(C.Id) AS CommentCount,
  SUM(P.Score) AS TotalPostScore
FROM UserScores AS US
JOIN Users AS U
  ON US.UserId = U.Id
LEFT JOIN BadgeCounts AS BC
  ON U.Id = BC.UserId
LEFT JOIN Comments AS C
  ON C.UserId = U.Id
LEFT JOIN Posts AS P
  ON P.OwnerUserId = U.Id
WHERE
  U.Reputation >= 1000
GROUP BY
  U.DisplayName,
  US.TotalScore,
  US.QuestionCount,
  BC.BadgeTotal
ORDER BY
  TotalScore DESC,
  GoldBadges DESC,
  QuestionCount DESC
LIMIT 10;
