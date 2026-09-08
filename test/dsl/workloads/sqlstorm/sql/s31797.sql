WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.OwnerUserId,
    U.DisplayName AS OwnerDisplayName,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.Score DESC) AS RN
  FROM Posts AS p
  JOIN Users AS U
    ON p.OwnerUserId = U.Id
  WHERE
    p.PostTypeId = 1
), RecentBadges AS (
  SELECT
    b.UserId,
    b.Name AS BadgeName,
    COUNT(b.Id) AS BadgeCount,
    RANK() OVER (ORDER BY COUNT(b.Id) DESC) AS BadgeRank
  FROM Badges AS b
  GROUP BY
    b.UserId,
    b.Name
), UserMetrics AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    U.Reputation,
    COALESCE(RB.BadgeCount, 0) AS BadgeCount,
    COALESCE(RankedPosts.Score, 0) AS MaxPostScore
  FROM Users AS U
  LEFT JOIN (
    SELECT DISTINCT
      UserId,
      BadgeCount
    FROM RecentBadges
  ) AS RB
    ON U.Id = RB.UserId
  LEFT JOIN (
    SELECT
      OwnerUserId,
      MAX(Score) AS Score
    FROM Posts
    GROUP BY
      OwnerUserId
  ) AS RankedPosts
    ON U.Id = RankedPosts.OwnerUserId
)
SELECT
  UM.DisplayName,
  UM.Reputation,
  UM.BadgeCount,
  RP.Title AS TopPostTitle,
  RP.Score AS TopPostScore,
  RP.ViewCount AS TopPostViews
FROM UserMetrics AS UM
LEFT JOIN RankedPosts AS RP
  ON UM.UserId = RP.OwnerUserId AND RP.RN = 1
WHERE
  (
    UM.Reputation > 1000 OR UM.BadgeCount > 0
  ) AND NOT RP.Score IS NULL
ORDER BY
  UM.Reputation DESC,
  UM.BadgeCount DESC
LIMIT 50;
