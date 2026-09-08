WITH UserBadgeCounts AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    COUNT(b.Id) FILTER(WHERE
      b.Class = 1) AS GoldBadges,
    COUNT(b.Id) FILTER(WHERE
      b.Class = 2) AS SilverBadges,
    COUNT(b.Id) FILTER(WHERE
      b.Class = 3) AS BronzeBadges,
    SUM(CASE WHEN b.TagBased THEN 1 ELSE 0 END) AS TagBasedBadges
  FROM Users AS u
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.DisplayName
), PostActivity AS (
  SELECT
    p.OwnerUserId,
    COUNT(c.Id) AS CommentCount,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpvoteCount,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownvoteCount,
    COALESCE(SUM(p.FavoriteCount), 0) AS FavoriteCount
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  GROUP BY
    p.OwnerUserId
), PostHistoryDetails AS (
  SELECT
    ph.UserId,
    ph.PostId,
    ph.PostHistoryTypeId,
    ph.CreationDate,
    r.Name AS PostHistoryTypeName
  FROM PostHistory AS ph
  JOIN PostHistoryTypes AS r
    ON ph.PostHistoryTypeId = r.Id
  WHERE
    ph.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
), RankedPosts AS (
  SELECT
    p.Id,
    p.Title,
    p.CreationDate,
    ROW_NUMBER() OVER (ORDER BY p.CreationDate DESC) AS RecentPostRank
  FROM Posts AS p
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '365 DAYS'
)
SELECT
  u.Id AS UserId,
  u.DisplayName,
  u.Reputation,
  ub.GoldBadges,
  ub.SilverBadges,
  ub.BronzeBadges,
  ub.TagBasedBadges,
  pa.CommentCount,
  pa.UpvoteCount,
  pa.DownvoteCount,
  pa.FavoriteCount,
  COUNT(DISTINCT ph.PostId) AS PostHistoryCount,
  COUNT(DISTINCT rp.Id) AS RecentPostsCount
FROM Users AS u
LEFT JOIN UserBadgeCounts AS ub
  ON u.Id = ub.UserId
LEFT JOIN PostActivity AS pa
  ON u.Id = pa.OwnerUserId
LEFT JOIN PostHistoryDetails AS ph
  ON u.Id = ph.UserId
LEFT JOIN RankedPosts AS rp
  ON u.Id = (
    SELECT
      OwnerUserId
    FROM Posts
    WHERE
      Posts.Id = rp.Id
    LIMIT 1
  )
GROUP BY
  u.Id,
  u.DisplayName,
  u.Reputation,
  ub.GoldBadges,
  ub.SilverBadges,
  ub.BronzeBadges,
  ub.TagBasedBadges,
  pa.CommentCount,
  pa.UpvoteCount,
  pa.DownvoteCount,
  pa.FavoriteCount
HAVING
  COUNT(DISTINCT rp.Id) > 5
  AND SUM(COALESCE(pa.UpvoteCount, 0) - COALESCE(pa.DownvoteCount, 0)) > 10
ORDER BY
  u.Reputation DESC,
  PostHistoryCount DESC;
