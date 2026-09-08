WITH UserReputation AS (
  SELECT
    Id,
    Reputation
  FROM Users
  WHERE
    Reputation > 1000
), PopularPosts AS (
  SELECT
    P.Id,
    P.Title,
    P.ViewCount,
    P.Score,
    P.AnswerCount,
    U.Reputation AS UserReputation
  FROM Posts AS P
  JOIN UserReputation AS U
    ON P.OwnerUserId = U.Id
  WHERE
    P.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
    AND P.Score > 0
), PostTags AS (
  SELECT
    P.Id AS PostId,
    UNNEST(STRING_TO_ARRAY(P.Tags, '><')) AS Tag
  FROM Posts AS P
  WHERE
    P.PostTypeId = 1
), TagPopularity AS (
  SELECT
    Tag,
    COUNT(Pt.PostId) AS PostCount
  FROM PostTags AS Pt
  GROUP BY
    Tag
  HAVING
    COUNT(Pt.PostId) > 5
), PopularityScores AS (
  SELECT
    P.Id,
    P.Title,
    P.ViewCount,
    P.Score,
    P.AnswerCount,
    (
      P.ViewCount * 0.2 + P.Score * 0.7 + P.AnswerCount * 0.1
    ) AS PopularityScore
  FROM PopularPosts AS P
  JOIN TagPopularity AS T
    ON P.Title ILIKE '%' || T.Tag || '%'
), RankedPosts AS (
  SELECT
    Id,
    Title,
    ViewCount,
    Score,
    AnswerCount,
    PopularityScore,
    ROW_NUMBER() OVER (ORDER BY PopularityScore DESC) AS Rank
  FROM PopularityScores
)
SELECT
  R.Title,
  R.ViewCount,
  R.Score,
  R.AnswerCount,
  R.Rank
FROM RankedPosts AS R
WHERE
  R.Rank <= 10
ORDER BY
  R.Rank;
