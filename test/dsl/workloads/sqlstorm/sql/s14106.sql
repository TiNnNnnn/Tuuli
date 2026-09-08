WITH PostStatistics AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    COUNT(c.Id) AS CommentCount,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVoteCount,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVoteCount,
    AVG(p.Score) AS AverageScore,
    MAX(p.CreationDate) AS LastActivityDate
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  GROUP BY
    p.Id,
    p.Title
), UserStatistics AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    COUNT(b.Id) AS BadgeCount,
    SUM(u.UpVotes) AS TotalUpVotes,
    SUM(u.DownVotes) AS TotalDownVotes
  FROM Users AS u
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.DisplayName
)
SELECT
  ps.PostId,
  ps.Title,
  ps.CommentCount,
  ps.UpVoteCount,
  ps.DownVoteCount,
  ps.AverageScore,
  ps.LastActivityDate,
  us.UserId,
  us.DisplayName,
  us.BadgeCount,
  us.TotalUpVotes,
  us.TotalDownVotes
FROM PostStatistics AS ps
JOIN UserStatistics AS us
  ON ps.PostId = us.UserId
ORDER BY
  ps.LastActivityDate DESC,
  ps.AverageScore DESC
LIMIT 100;
