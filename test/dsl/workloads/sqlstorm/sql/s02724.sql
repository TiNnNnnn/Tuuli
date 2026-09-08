WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.Score DESC) AS ScoreRank,
    COUNT(c.Id) AS CommentCount,
    AVG(CASE WHEN v.VoteTypeId = 8 THEN v.BountyAmount ELSE NULL END) AS AvgBounty
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  GROUP BY
    p.Id,
    p.OwnerUserId,
    p.Title,
    p.CreationDate,
    p.Score
), UserStats AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    u.Reputation,
    u.CreationDate,
    u.LastAccessDate,
    COALESCE(SUM(CASE WHEN b.Class = 1 THEN 1 ELSE 0 END), 0) AS GoldBadges,
    COALESCE(SUM(CASE WHEN b.Class = 2 THEN 1 ELSE 0 END), 0) AS SilverBadges,
    COALESCE(SUM(CASE WHEN b.Class = 3 THEN 1 ELSE 0 END), 0) AS BronzeBadges
  FROM Users AS u
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.DisplayName,
    u.Reputation,
    u.CreationDate,
    u.LastAccessDate
)
SELECT
  us.DisplayName,
  us.Reputation,
  rp.Title,
  rp.Score,
  rp.CreationDate,
  rp.CommentCount,
  us.GoldBadges,
  us.SilverBadges,
  us.BronzeBadges,
  rp.AvgBounty
FROM RankedPosts AS rp
JOIN UserStats AS us
  ON rp.PostId = us.UserId
WHERE
  rp.ScoreRank <= 3
  AND us.Reputation > (
    SELECT
      AVG(Reputation)
    FROM Users
    WHERE
      NOT Reputation IS NULL
  )
ORDER BY
  us.Reputation DESC,
  rp.Score DESC
LIMIT 10;
