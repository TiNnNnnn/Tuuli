WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    u.DisplayName AS Author,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC, p.ViewCount DESC) AS Rank,
    COUNT(DISTINCT v.Id) AS VoteCount,
    COUNT(c.Id) AS CommentCount,
    COUNT(b.Id) AS BadgeCount
  FROM Posts AS p
  LEFT JOIN Users AS u
    ON p.OwnerUserId = u.Id
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  WHERE
    p.CreationDate > CURRENT_DATE - INTERVAL '1 YEAR'
  GROUP BY
    p.Id,
    p.Title,
    u.DisplayName,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    p.PostTypeId
), FilteredPosts AS (
  SELECT
    rp.PostId,
    rp.Title,
    rp.Author,
    rp.CreationDate,
    rp.ViewCount,
    rp.Score,
    rp.Rank,
    rp.VoteCount,
    rp.CommentCount,
    rp.BadgeCount
  FROM RankedPosts AS rp
  WHERE
    rp.Rank <= 10
)
SELECT
  fp.PostId,
  fp.Title,
  fp.Author,
  fp.CreationDate,
  fp.ViewCount,
  fp.Score,
  fp.VoteCount,
  fp.CommentCount,
  fp.BadgeCount
FROM FilteredPosts AS fp
ORDER BY
  fp.Score DESC,
  fp.ViewCount DESC;
