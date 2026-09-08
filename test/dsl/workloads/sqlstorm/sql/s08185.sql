WITH RecentPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    u.DisplayName AS OwnerDisplayName,
    COUNT(c.Id) AS CommentCount,
    COUNT(DISTINCT v.Id) AS VoteCount
  FROM Posts AS p
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId AND v.VoteTypeId = 2
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
    AND p.PostTypeId = 1
  GROUP BY
    p.Id,
    u.DisplayName
), TopPosts AS (
  SELECT
    rp.PostId,
    rp.Title,
    rp.CreationDate,
    rp.ViewCount,
    rp.Score,
    rp.OwnerDisplayName,
    rp.CommentCount,
    rp.VoteCount,
    ROW_NUMBER() OVER (ORDER BY rp.Score DESC, rp.ViewCount DESC) AS Rank
  FROM RecentPosts AS rp
)
SELECT
  tp.PostId,
  tp.Title,
  tp.OwnerDisplayName,
  tp.CreationDate,
  tp.ViewCount,
  tp.Score,
  tp.CommentCount,
  tp.VoteCount,
  COALESCE(b.Name, 'No Badge') AS UserBadge
FROM TopPosts AS tp
LEFT JOIN Badges AS b
  ON tp.OwnerDisplayName = CAST(b.UserId AS TEXT)
WHERE
  tp.Rank <= 10
ORDER BY
  tp.Rank;
