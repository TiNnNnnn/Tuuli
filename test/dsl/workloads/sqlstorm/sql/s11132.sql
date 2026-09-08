WITH UserStatistics AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    COUNT(DISTINCT p.Id) AS TotalPosts,
    COUNT(DISTINCT v.Id) AS TotalVotes,
    SUM(b.Class) AS TotalBadgeClass,
    SUM(CASE WHEN p.PostTypeId = 1 THEN 1 ELSE 0 END) AS TotalQuestions,
    SUM(CASE WHEN p.PostTypeId = 2 THEN 1 ELSE 0 END) AS TotalAnswers
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.DisplayName
), PostStatistics AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    COUNT(c.Id) AS TotalComments,
    p.OwnerUserId
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON c.PostId = p.Id
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    p.OwnerUserId
)
SELECT
  us.UserId,
  us.DisplayName,
  us.TotalPosts,
  us.TotalVotes,
  us.TotalBadgeClass,
  us.TotalQuestions,
  us.TotalAnswers,
  ps.PostId,
  ps.Title,
  ps.CreationDate,
  ps.ViewCount,
  ps.Score,
  ps.TotalComments
FROM UserStatistics AS us
JOIN PostStatistics AS ps
  ON us.UserId = ps.OwnerUserId
ORDER BY
  us.TotalPosts DESC,
  ps.ViewCount DESC;
