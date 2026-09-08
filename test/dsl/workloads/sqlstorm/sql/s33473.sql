WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC) AS RankScore,
    COUNT(c.Id) AS CommentCount,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
    AND p.Score > 0
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.PostTypeId
), RecentBadges AS (
  SELECT
    b.UserId,
    COUNT(b.Id) AS BadgeCount,
    STRING_AGG(b.Name, ', ') AS BadgeNames
  FROM Badges AS b
  WHERE
    b.Date >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
  GROUP BY
    b.UserId
), PostLinkCount AS (
  SELECT
    pl.PostId,
    COUNT(pl.RelatedPostId) AS RelatedPostsCount
  FROM PostLinks AS pl
  GROUP BY
    pl.PostId
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.Score,
  rp.CommentCount,
  rp.UpVotes,
  rp.DownVotes,
  COALESCE(rb.BadgeCount, 0) AS RecentBadgesCount,
  COALESCE(rb.BadgeNames, 'No Badges') AS RecentBadgeNames,
  COALESCE(plc.RelatedPostsCount, 0) AS RelatedPostsCount
FROM RankedPosts AS rp
LEFT JOIN RecentBadges AS rb
  ON rp.PostId = rb.UserId
LEFT JOIN PostLinkCount AS plc
  ON rp.PostId = plc.PostId
WHERE
  rp.RankScore <= 5
ORDER BY
  rp.Score DESC,
  rp.CreationDate DESC;
