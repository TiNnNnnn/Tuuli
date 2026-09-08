WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.Score,
    p.CreationDate,
    u.DisplayName AS OwnerDisplayName,
    COUNT(c.Id) AS CommentCount,
    COUNT(DISTINCT v.Id) AS VoteCount,
    ROW_NUMBER() OVER (PARTITION BY p.Id ORDER BY p.CreationDate DESC) AS PostRank
  FROM Posts AS p
  LEFT JOIN Users AS u
    ON p.OwnerUserId = u.Id
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  WHERE
    p.PostTypeId = 1
  GROUP BY
    p.Id,
    p.Title,
    p.Score,
    p.CreationDate,
    u.DisplayName
), TopPosts AS (
  SELECT
    PostId,
    Title,
    Score,
    CreationDate,
    OwnerDisplayName,
    CommentCount,
    VoteCount
  FROM RankedPosts
  WHERE
    PostRank = 1
  ORDER BY
    Score DESC,
    CreationDate DESC
  LIMIT 10
)
SELECT
  tp.Title,
  tp.Score,
  tp.CreationDate,
  tp.OwnerDisplayName,
  tp.CommentCount,
  tp.VoteCount,
  pht.Name AS PostHistoryType,
  COUNT(ph.Id) AS HistoryCount
FROM TopPosts AS tp
LEFT JOIN PostHistory AS ph
  ON tp.PostId = ph.PostId
LEFT JOIN PostHistoryTypes AS pht
  ON ph.PostHistoryTypeId = pht.Id
GROUP BY
  tp.PostId,
  tp.Title,
  tp.Score,
  tp.CreationDate,
  tp.OwnerDisplayName,
  tp.CommentCount,
  tp.VoteCount,
  pht.Name
ORDER BY
  tp.Score DESC;
