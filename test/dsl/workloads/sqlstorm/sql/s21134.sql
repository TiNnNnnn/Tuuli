WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC) AS ScoreRank,
    COUNT(c.Id) AS CommentCount
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
    p.ViewCount
), PostVoteSummary AS (
  SELECT
    p.Id AS PostId,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes,
    COALESCE(SUM(CASE WHEN v.VoteTypeId = 1 THEN 1 ELSE 0 END), 0) AS AcceptedVotes
  FROM Posts AS p
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  GROUP BY
    p.Id
), UserBadgesCount AS (
  SELECT
    b.UserId,
    COUNT(b.Id) AS BadgeCount,
    MAX(b.Class) AS MaxBadgeClass
  FROM Badges AS b
  WHERE
    b.Date >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '6 MONTHS'
  GROUP BY
    b.UserId
), FilteredUsers AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    u.Reputation,
    COALESCE(ub.BadgeCount, 0) AS BadgeCount,
    CASE
      WHEN ub.MaxBadgeClass = 1
      THEN 'Gold'
      WHEN ub.MaxBadgeClass = 2
      THEN 'Silver'
      WHEN ub.MaxBadgeClass = 3
      THEN 'Bronze'
      ELSE 'No Badge'
    END AS MaxBadgeCategory
  FROM Users AS u
  LEFT JOIN UserBadgesCount AS ub
    ON u.Id = ub.UserId
  WHERE
    u.Reputation > 1000 AND NOT u.Location IS NULL
), MostCommentedPosts AS (
  SELECT
    rp.PostId,
    rp.Title,
    rp.CreationDate,
    rp.CommentCount,
    pv.UpVotes,
    pv.DownVotes,
    RANK() OVER (ORDER BY rp.CommentCount DESC) AS CommentRank
  FROM RankedPosts AS rp
  JOIN PostVoteSummary AS pv
    ON rp.PostId = pv.PostId
  WHERE
    rp.CommentCount > 0
)
SELECT
  f.DisplayName,
  f.Reputation,
  f.MaxBadgeCategory,
  mcp.Title AS MostCommentedPostTitle,
  mcp.CommentCount AS MostComments,
  mcp.UpVotes,
  mcp.DownVotes
FROM FilteredUsers AS f
LEFT JOIN MostCommentedPosts AS mcp
  ON f.UserId = (
    SELECT
      p.OwnerUserId
    FROM Posts AS p
    WHERE
      p.Id = mcp.PostId
    LIMIT 1
  )
WHERE
  mcp.CommentRank <= 10
ORDER BY
  f.Reputation DESC;
