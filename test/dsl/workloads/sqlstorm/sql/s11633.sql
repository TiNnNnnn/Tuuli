WITH PostStats AS (
  SELECT
    p.Id AS PostId,
    p.PostTypeId,
    COUNT(c.Id) AS CommentCount,
    COUNT(v.Id) AS VoteCount,
    MAX(p.CreationDate) AS LastActive
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  GROUP BY
    p.Id,
    p.PostTypeId
), UserStats AS (
  SELECT
    u.Id AS UserId,
    COUNT(b.Id) AS BadgeCount,
    SUM(u.UpVotes) AS TotalUpVotes,
    AVG(u.Reputation) AS AvgReputation
  FROM Users AS u
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id
)
SELECT
  ps.PostId,
  ps.PostTypeId,
  ps.CommentCount,
  ps.VoteCount,
  ps.LastActive,
  us.UserId,
  us.BadgeCount,
  us.TotalUpVotes,
  us.AvgReputation
FROM PostStats AS ps
JOIN UserStats AS us
  ON ps.PostTypeId = 1
WHERE
  ps.LastActive BETWEEN CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS' AND CAST('2024-10-01 12:34:56' AS TIMESTAMP)
ORDER BY
  ps.VoteCount DESC
LIMIT 100;
