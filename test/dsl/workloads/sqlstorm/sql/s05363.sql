WITH RankedPosts AS (
  SELECT
    p.Id,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    u.DisplayName AS OwnerDisplayName,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.Score DESC) AS PostRank
  FROM Posts AS p
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  WHERE
    p.PostTypeId = 1
    AND p.Score > 0
    AND p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
), TopPosts AS (
  SELECT
    Id,
    Title,
    CreationDate,
    Score,
    ViewCount,
    OwnerDisplayName
  FROM RankedPosts
  WHERE
    PostRank <= 5
), PostComments AS (
  SELECT
    c.PostId,
    COUNT(c.Id) AS CommentCount
  FROM Comments AS c
  GROUP BY
    c.PostId
), PostVoteTypes AS (
  SELECT
    v.PostId,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes
  FROM Votes AS v
  GROUP BY
    v.PostId
)
SELECT
  tp.Title,
  tp.CreationDate,
  tp.Score,
  tp.ViewCount,
  tp.OwnerDisplayName,
  COALESCE(pc.CommentCount, 0) AS TotalComments,
  COALESCE(pvt.UpVotes, 0) AS TotalUpVotes,
  COALESCE(pvt.DownVotes, 0) AS TotalDownVotes
FROM TopPosts AS tp
LEFT JOIN PostComments AS pc
  ON tp.Id = pc.PostId
LEFT JOIN PostVoteTypes AS pvt
  ON tp.Id = pvt.PostId
ORDER BY
  tp.Score DESC,
  tp.ViewCount DESC;
