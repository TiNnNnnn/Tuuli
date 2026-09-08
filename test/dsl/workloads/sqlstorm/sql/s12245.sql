SELECT
  u.Id AS UserId,
  u.DisplayName,
  COUNT(DISTINCT p.Id) AS TotalPosts,
  COUNT(DISTINCT CASE WHEN p.PostTypeId = 1 THEN p.Id END) AS TotalQuestions,
  COUNT(DISTINCT CASE WHEN p.PostTypeId = 2 THEN p.Id END) AS TotalAnswers,
  SUM(p.Score) AS TotalScore,
  SUM(p.ViewCount) AS TotalViews,
  SUM(COALESCE(c.Score, 0)) AS TotalCommentScore,
  COUNT(DISTINCT b.Id) AS TotalBadges
FROM Users AS u
LEFT JOIN Posts AS p
  ON u.Id = p.OwnerUserId
LEFT JOIN Comments AS c
  ON p.Id = c.PostId
LEFT JOIN Badges AS b
  ON u.Id = b.UserId
GROUP BY
  u.Id,
  u.DisplayName
ORDER BY
  TotalPosts DESC
LIMIT 10;
