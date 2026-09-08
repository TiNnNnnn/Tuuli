WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    u.DisplayName AS OwnerDisplayName,
    COUNT(c.Id) AS CommentCount,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC, p.ViewCount DESC) AS Rank
  FROM Posts AS p
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    u.DisplayName,
    p.PostTypeId
), TopPosts AS (
  SELECT
    PostId,
    Title,
    CreationDate,
    ViewCount,
    Score,
    OwnerDisplayName
  FROM RankedPosts
  WHERE
    Rank <= 10
), VoteSummary AS (
  SELECT
    PostId,
    SUM(CASE WHEN VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes
  FROM Votes
  GROUP BY
    PostId
)
SELECT
  tp.PostId,
  tp.Title,
  tp.CreationDate,
  tp.ViewCount,
  tp.Score,
  tp.OwnerDisplayName,
  COALESCE(vs.UpVotes, 0) AS UpVotes,
  COALESCE(vs.DownVotes, 0) AS DownVotes,
  (
    tp.ViewCount + COALESCE(vs.UpVotes, 0) - COALESCE(vs.DownVotes, 0)
  ) AS EngagementScore
FROM TopPosts AS tp
LEFT JOIN VoteSummary AS vs
  ON tp.PostId = vs.PostId
ORDER BY
  EngagementScore DESC;
