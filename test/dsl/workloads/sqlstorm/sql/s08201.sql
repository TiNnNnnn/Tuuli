WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    COALESCE(u.DisplayName, 'Community User') AS OwnerDisplayName,
    COUNT(c.Id) AS CommentCount,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes,
    RANK() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC, p.ViewCount DESC) AS Rank
  FROM Posts AS p
  LEFT JOIN Users AS u
    ON p.OwnerUserId = u.Id
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    u.DisplayName,
    p.PostTypeId
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.Score,
  rp.ViewCount,
  rp.OwnerDisplayName,
  rp.CommentCount,
  rp.UpVotes,
  rp.DownVotes,
  pt.Name AS PostTypeName
FROM RankedPosts AS rp
JOIN PostTypes AS pt
  ON rp.Rank = 1
  AND pt.Id = (
    CASE
      WHEN rp.PostId IN (
        SELECT
          PostId
        FROM Posts
        WHERE
          NOT AcceptedAnswerId IS NULL
      )
      THEN 1
      ELSE rp.PostId
    END
  )
ORDER BY
  rp.ViewCount DESC
LIMIT 10;
