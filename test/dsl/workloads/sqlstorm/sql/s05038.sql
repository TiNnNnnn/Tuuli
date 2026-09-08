WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    u.DisplayName AS OwnerDisplayName,
    p.AnswerCount,
    p.CommentCount,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.Score DESC, p.CreationDate DESC) AS Rank,
    p.OwnerUserId
  FROM Posts AS p
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  WHERE
    p.PostTypeId = 1 AND p.CreationDate >= CURRENT_TIMESTAMP - INTERVAL '1 YEAR'
), PostBadges AS (
  SELECT
    b.UserId,
    COUNT(b.Id) AS BadgeCount
  FROM Badges AS b
  WHERE
    b.Date >= CURRENT_TIMESTAMP - INTERVAL '1 YEAR'
  GROUP BY
    b.UserId
), TopUsers AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    u.Reputation,
    COALESCE(pb.BadgeCount, 0) AS BadgeCount
  FROM Users AS u
  LEFT JOIN PostBadges AS pb
    ON u.Id = pb.UserId
  ORDER BY
    u.Reputation DESC
  LIMIT 10
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.Score,
  rp.OwnerDisplayName,
  ru.Reputation,
  ru.BadgeCount,
  COUNT(c.Id) AS CommentCount,
  SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
  SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes
FROM RankedPosts AS rp
JOIN TopUsers AS ru
  ON rp.OwnerUserId = ru.UserId
LEFT JOIN Comments AS c
  ON c.PostId = rp.PostId
LEFT JOIN Votes AS v
  ON v.PostId = rp.PostId
WHERE
  rp.Rank <= 3
GROUP BY
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.Score,
  rp.OwnerDisplayName,
  ru.UserId,
  ru.Reputation,
  ru.BadgeCount
ORDER BY
  rp.Score DESC,
  ru.Reputation DESC;
