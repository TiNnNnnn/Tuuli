WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.Tags,
    COUNT(c.Id) AS CommentCount,
    RANK() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC) AS ScoreRank
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.Tags
), TopPosts AS (
  SELECT
    rp.PostId,
    rp.Title,
    rp.CreationDate,
    rp.Score,
    rp.ViewCount,
    rp.Tags,
    rp.CommentCount
  FROM RankedPosts AS rp
  WHERE
    rp.ScoreRank <= 10
), PostDetails AS (
  SELECT
    tp.PostId,
    tp.Title,
    tp.CreationDate,
    tp.Score,
    tp.ViewCount,
    tp.CommentCount,
    u.DisplayName AS OwnerDisplayName,
    u.Reputation AS OwnerReputation,
    bh.Name AS BadgeName,
    COUNT(v.Id) AS VoteCount
  FROM TopPosts AS tp
  JOIN Users AS u
    ON tp.PostId = u.Id
  LEFT JOIN Badges AS bh
    ON u.Id = bh.UserId
  LEFT JOIN Votes AS v
    ON tp.PostId = v.PostId
  GROUP BY
    tp.PostId,
    tp.Title,
    tp.CreationDate,
    tp.Score,
    tp.ViewCount,
    tp.CommentCount,
    u.DisplayName,
    u.Reputation,
    bh.Name
)
SELECT
  pd.PostId,
  pd.Title,
  pd.CreationDate,
  pd.Score,
  pd.ViewCount,
  pd.CommentCount,
  pd.OwnerDisplayName,
  pd.OwnerReputation,
  ARRAY_AGG(DISTINCT pd.BadgeName) AS Badges,
  pd.VoteCount
FROM PostDetails AS pd
GROUP BY
  pd.PostId,
  pd.Title,
  pd.CreationDate,
  pd.Score,
  pd.ViewCount,
  pd.CommentCount,
  pd.OwnerDisplayName,
  pd.OwnerReputation,
  pd.VoteCount
ORDER BY
  pd.Score DESC,
  pd.ViewCount DESC;
