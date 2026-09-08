WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.Body,
    p.CreationDate,
    p.Score,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.Score DESC) AS Rank,
    p.OwnerUserId
  FROM Posts AS p
  WHERE
    p.PostTypeId = 1 AND NOT p.Score IS NULL
), UserReputation AS (
  SELECT
    u.Id AS UserId,
    u.Reputation,
    COUNT(a.Id) AS AnswerCount,
    SUM(CASE WHEN b.Class = 1 THEN 1 ELSE 0 END) AS GoldBadges,
    SUM(CASE WHEN b.Class = 2 THEN 1 ELSE 0 END) AS SilverBadges,
    SUM(CASE WHEN b.Class = 3 THEN 1 ELSE 0 END) AS BronzeBadges
  FROM Users AS u
  LEFT JOIN Posts AS a
    ON u.Id = a.OwnerUserId AND a.PostTypeId = 2
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.Reputation
), CloseReasons AS (
  SELECT
    ph.PostId,
    cr.Name AS CloseReason
  FROM PostHistory AS ph
  JOIN CloseReasonTypes AS cr
    ON ph.Comment = CAST(cr.Id AS TEXT)
  WHERE
    ph.PostHistoryTypeId IN (10, 11)
)
SELECT
  rp.PostId,
  rp.Title,
  rp.Body,
  rp.CreationDate,
  ur.UserId,
  ur.Reputation,
  ur.AnswerCount,
  ur.GoldBadges,
  ur.SilverBadges,
  ur.BronzeBadges,
  COALESCE(cr.CloseReason, 'Not Closed') AS CloseReason,
  CASE WHEN rp.Rank = 1 THEN 'Top Post' ELSE 'Regular Post' END AS PostRank
FROM RankedPosts AS rp
JOIN UserReputation AS ur
  ON rp.OwnerUserId = ur.UserId
LEFT JOIN CloseReasons AS cr
  ON rp.PostId = cr.PostId
WHERE
  ur.Reputation >= (
    SELECT
      AVG(Reputation)
    FROM Users
  )
  AND rp.CreationDate >= (
    CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
  )
ORDER BY
  rp.Score DESC,
  ur.Reputation DESC;
