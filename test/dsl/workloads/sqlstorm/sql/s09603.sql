WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.OwnerUserId,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.Score DESC) AS RankPerUser
  FROM Posts AS p
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
    AND p.PostTypeId = 1
), TopPosts AS (
  SELECT
    rp.PostId,
    rp.Title,
    rp.CreationDate,
    rp.Score,
    u.DisplayName AS OwnerDisplayName
  FROM RankedPosts AS rp
  JOIN Users AS u
    ON rp.OwnerUserId = u.Id
  WHERE
    rp.RankPerUser <= 5
), PostStatistics AS (
  SELECT
    tp.PostId,
    tp.Title,
    tp.CreationDate,
    tp.Score,
    tp.OwnerDisplayName,
    COUNT(c.Id) AS CommentCount,
    COUNT(v.Id) FILTER(WHERE
      v.VoteTypeId = 2) AS UpVoteCount,
    COUNT(v.Id) FILTER(WHERE
      v.VoteTypeId = 3) AS DownVoteCount
  FROM TopPosts AS tp
  LEFT JOIN Comments AS c
    ON tp.PostId = c.PostId
  LEFT JOIN Votes AS v
    ON tp.PostId = v.PostId
  GROUP BY
    tp.PostId,
    tp.Title,
    tp.CreationDate,
    tp.Score,
    tp.OwnerDisplayName
)
SELECT
  ps.PostId,
  ps.Title,
  ps.CreationDate,
  ps.Score,
  ps.OwnerDisplayName,
  ps.CommentCount,
  ps.UpVoteCount,
  ps.DownVoteCount,
  CASE
    WHEN ps.Score > 100
    THEN 'High'
    WHEN ps.Score BETWEEN 50 AND 100
    THEN 'Medium'
    ELSE 'Low'
  END AS ScoreCategory
FROM PostStatistics AS ps
ORDER BY
  ps.Score DESC
LIMIT 10;
