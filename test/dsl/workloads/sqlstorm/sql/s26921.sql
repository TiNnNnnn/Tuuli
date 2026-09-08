WITH TagCounts AS (
  SELECT
    UNNEST(STRING_TO_ARRAY(SUBSTRING(Tags FROM 2 FOR LENGTH(Tags) - 2), '><')) AS TagName,
    COUNT(*) AS PostCount
  FROM Posts
  WHERE
    PostTypeId = 1
  GROUP BY
    TagName
), TopTags AS (
  SELECT
    TagName,
    PostCount
  FROM TagCounts
  ORDER BY
    PostCount DESC
  LIMIT 10
), RecentPosts AS (
  SELECT
    Id,
    Title,
    CreationDate,
    ViewCount,
    OwnerDisplayName,
    Score
  FROM Posts
  WHERE
    CreationDate > CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
  ORDER BY
    CreationDate DESC
  LIMIT 10
), UserReputation AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    U.Reputation,
    COUNT(DISTINCT P.Id) AS TotalPosts,
    SUM(COALESCE(P.Score, 0)) AS TotalScore
  FROM Users AS U
  LEFT JOIN Posts AS P
    ON U.Id = P.OwnerUserId
  GROUP BY
    U.Id,
    U.DisplayName,
    U.Reputation
)
SELECT
  T.TagName,
  T.PostCount,
  R.Id AS RecentPostId,
  R.Title AS RecentPostTitle,
  R.CreationDate AS RecentPostDate,
  R.ViewCount AS RecentPostViews,
  R.OwnerDisplayName AS RecentPostOwner,
  R.Score AS RecentPostScore,
  U.DisplayName AS UserName,
  U.Reputation AS UserReputation,
  U.TotalPosts AS UserTotalPosts,
  U.TotalScore AS UserTotalScore
FROM TopTags AS T
LEFT JOIN RecentPosts AS R
  ON TRUE
LEFT JOIN UserReputation AS U
  ON R.OwnerDisplayName = U.DisplayName
ORDER BY
  T.PostCount DESC,
  R.CreationDate DESC,
  U.Reputation DESC;
