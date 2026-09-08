WITH RecentPosts AS (
  SELECT
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    p.OwnerUserId,
    u.DisplayName AS OwnerDisplayName,
    COUNT(c.Id) AS CommentCount,
    COALESCE(SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END), 0) AS UpVotes,
    COALESCE(SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END), 0) AS DownVotes
  FROM Posts AS p
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    p.OwnerUserId,
    u.DisplayName
), TopUsers AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    SUM(b.Class) AS TotalBadges,
    COUNT(p.Id) AS PostCount
  FROM Users AS u
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  GROUP BY
    u.Id,
    u.DisplayName
  ORDER BY
    TotalBadges DESC,
    PostCount DESC
  LIMIT 10
), PostVoteSummary AS (
  SELECT
    p.Id AS PostId,
    COUNT(CASE WHEN v.VoteTypeId = 2 THEN 1 END) AS UpVotes,
    COUNT(CASE WHEN v.VoteTypeId = 3 THEN 1 END) AS DownVotes
  FROM Posts AS p
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  GROUP BY
    p.Id
)
SELECT
  rp.Title,
  rp.CreationDate,
  rp.ViewCount,
  rp.Score,
  rp.CommentCount,
  rp.UpVotes,
  rp.DownVotes,
  tu.DisplayName AS TopUserDisplayName,
  tu.TotalBadges,
  tu.PostCount
FROM RecentPosts AS rp
JOIN TopUsers AS tu
  ON rp.OwnerUserId = tu.UserId
JOIN PostVoteSummary AS pvs
  ON rp.Id = pvs.PostId
ORDER BY
  rp.CreationDate DESC,
  rp.Score DESC;
