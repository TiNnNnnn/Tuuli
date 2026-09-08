WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.ViewCount DESC) AS ViewRank,
    COUNT(DISTINCT c.Id) AS CommentCount,
    p.OwnerUserId
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '365 DAYS'
    AND p.PostTypeId = 1
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.OwnerUserId
), UserStats AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    SUM(COALESCE(p.ViewCount, 0)) AS TotalViews,
    COUNT(DISTINCT p.Id) AS TotalPosts,
    COALESCE(SUM(CASE WHEN b.Class = 1 THEN 1 ELSE 0 END), 0) AS GoldBadges,
    COALESCE(SUM(CASE WHEN b.Class = 2 THEN 1 ELSE 0 END), 0) AS SilverBadges,
    COALESCE(SUM(CASE WHEN b.Class = 3 THEN 1 ELSE 0 END), 0) AS BronzeBadges
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  WHERE
    u.Reputation > 1000
  GROUP BY
    u.Id,
    u.DisplayName
)
SELECT
  us.DisplayName,
  us.TotalPosts,
  us.TotalViews,
  us.GoldBadges,
  us.SilverBadges,
  us.BronzeBadges,
  rp.Title,
  rp.CreationDate,
  rp.ViewCount,
  rp.CommentCount
FROM UserStats AS us
JOIN RankedPosts AS rp
  ON us.UserId = rp.OwnerUserId
WHERE
  rp.ViewRank <= 3
ORDER BY
  us.TotalViews DESC,
  rp.ViewCount DESC;
