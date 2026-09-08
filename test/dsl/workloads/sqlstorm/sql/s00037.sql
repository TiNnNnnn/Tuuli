WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.CreationDate DESC) AS rn
  FROM Posts AS p
  WHERE
    p.Score > 0
    AND p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
), UserStats AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    COUNT(b.Id) AS BadgeCount,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVoteCount,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVoteCount
  FROM Users AS u
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  LEFT JOIN Votes AS v
    ON u.Id = v.UserId
  GROUP BY
    u.Id,
    u.DisplayName
), PostWithVotes AS (
  SELECT
    p.Id AS PostId,
    COALESCE(SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END), 0) AS TotalUpVotes,
    COALESCE(SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END), 0) AS TotalDownVotes,
    COALESCE(SUM(CASE WHEN v.VoteTypeId = 1 THEN 1 ELSE 0 END), 0) AS AcceptedByOriginator
  FROM Posts AS p
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  GROUP BY
    p.Id
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.Score,
  rp.ViewCount,
  us.DisplayName AS UserName,
  us.BadgeCount,
  us.UpVoteCount,
  us.DownVoteCount,
  pw.TotalUpVotes,
  pw.TotalDownVotes,
  pw.AcceptedByOriginator
FROM RankedPosts AS rp
JOIN Posts AS p
  ON rp.PostId = p.Id
LEFT JOIN UserStats AS us
  ON p.OwnerUserId = us.UserId
LEFT JOIN PostWithVotes AS pw
  ON p.Id = pw.PostId
WHERE
  rp.rn = 1 AND (
    us.BadgeCount > 3 OR us.UpVoteCount > 10
  )
ORDER BY
  rp.ViewCount DESC,
  rp.Score DESC;
