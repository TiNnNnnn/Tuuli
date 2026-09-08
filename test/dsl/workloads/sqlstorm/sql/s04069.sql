WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.OwnerUserId,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.CreationDate DESC) AS PostRank,
    COUNT(c.Id) AS CommentCount,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  WHERE
    p.CreationDate >= CURRENT_DATE - INTERVAL '1 YEAR'
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.Score,
    p.OwnerUserId
), AggregatedUsers AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    COUNT(DISTINCT p.Id) AS TotalPosts,
    SUM(CASE WHEN b.Class = 1 THEN 1 ELSE 0 END) AS GoldBadges,
    SUM(CASE WHEN b.Class = 2 THEN 1 ELSE 0 END) AS SilverBadges,
    SUM(CASE WHEN b.Class = 3 THEN 1 ELSE 0 END) AS BronzeBadges
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.DisplayName
)
SELECT
  au.UserId,
  au.DisplayName,
  au.TotalPosts,
  au.GoldBadges,
  au.SilverBadges,
  au.BronzeBadges,
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.Score,
  rp.CommentCount,
  rp.UpVotes,
  rp.DownVotes
FROM AggregatedUsers AS au
LEFT JOIN RankedPosts AS rp
  ON au.UserId = rp.OwnerUserId
WHERE
  au.TotalPosts > 10 AND rp.PostRank <= 5
ORDER BY
  au.DisplayName,
  rp.CreationDate DESC;
