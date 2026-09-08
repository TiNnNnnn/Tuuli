WITH PostStats AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    COUNT(c.Id) AS CommentCount,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpvoteCount,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownvoteCount,
    p.CreationDate,
    p.LastActivityDate,
    p.ViewCount,
    p.Score,
    p.AnswerCount
  FROM Posts AS p
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
    p.LastActivityDate,
    p.ViewCount,
    p.Score,
    p.AnswerCount
), UserStats AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    COUNT(b.Id) AS BadgeCount,
    AVG(u.Reputation) AS AverageReputation,
    SUM(p.ViewCount) AS TotalViews,
    SUM(p.Score) AS TotalScore
  FROM Users AS u
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  GROUP BY
    u.Id,
    u.DisplayName
)
SELECT
  ps.PostId,
  ps.Title,
  ps.CommentCount,
  ps.UpvoteCount,
  ps.DownvoteCount,
  ps.CreationDate,
  ps.LastActivityDate,
  ps.ViewCount,
  ps.Score,
  ps.AnswerCount,
  us.UserId,
  us.DisplayName,
  us.BadgeCount,
  us.AverageReputation,
  us.TotalViews,
  us.TotalScore
FROM PostStats AS ps
JOIN UserStats AS us
  ON ps.PostId = us.UserId
ORDER BY
  ps.Score DESC;
