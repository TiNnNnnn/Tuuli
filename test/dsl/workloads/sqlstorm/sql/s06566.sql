WITH UserActivity AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    COUNT(p.Id) AS TotalPosts,
    SUM(CASE WHEN p.PostTypeId = 1 THEN 1 ELSE 0 END) AS TotalQuestions,
    SUM(CASE WHEN p.PostTypeId = 2 THEN 1 ELSE 0 END) AS TotalAnswers,
    SUM(CASE WHEN p.PostTypeId = 3 THEN 1 ELSE 0 END) AS TotalWikis,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS TotalUpvotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS TotalDownvotes,
    SUM(CASE WHEN NOT b.Date IS NULL THEN 1 ELSE 0 END) AS TotalBadges
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId AND v.UserId = u.Id
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.DisplayName
), TopUsers AS (
  SELECT
    UserId,
    DisplayName,
    TotalPosts,
    TotalQuestions,
    TotalAnswers,
    TotalWikis,
    TotalUpvotes,
    TotalDownvotes,
    TotalBadges,
    RANK() OVER (ORDER BY TotalPosts DESC) AS Rank
  FROM UserActivity
)
SELECT
  t.UserId,
  t.DisplayName,
  t.TotalPosts,
  t.TotalQuestions,
  t.TotalAnswers,
  t.TotalWikis,
  t.TotalUpvotes,
  t.TotalDownvotes,
  t.TotalBadges,
  t.Rank
FROM TopUsers AS t
WHERE
  t.Rank <= 10
ORDER BY
  t.Rank;
