WITH RankedUsers AS (
  SELECT
    u.Id,
    u.DisplayName,
    u.Reputation,
    u.Views,
    RANK() OVER (ORDER BY u.Reputation DESC) AS UserRank
  FROM Users AS u
  WHERE
    NOT u.Reputation IS NULL
), PostStats AS (
  SELECT
    p.OwnerUserId,
    COUNT(*) AS TotalPosts,
    COUNT(CASE WHEN p.PostTypeId = 1 THEN 1 END) AS TotalQuestions,
    COUNT(CASE WHEN p.PostTypeId = 2 THEN 1 END) AS TotalAnswers,
    SUM(p.Score) AS TotalScore,
    SUM(p.ViewCount) AS TotalViews
  FROM Posts AS p
  GROUP BY
    p.OwnerUserId
), AnsweredQuestions AS (
  SELECT
    p.OwnerUserId,
    MIN(CASE WHEN NOT p.AcceptedAnswerId IS NULL THEN p.CreationDate END) AS FirstAcceptedAnswerDate
  FROM Posts AS p
  WHERE
    p.PostTypeId = 1
  GROUP BY
    p.OwnerUserId
), UserBadges AS (
  SELECT
    b.UserId,
    COUNT(CASE WHEN b.Class = 1 THEN 1 END) AS GoldBadges,
    COUNT(CASE WHEN b.Class = 2 THEN 1 END) AS SilverBadges,
    COUNT(CASE WHEN b.Class = 3 THEN 1 END) AS BronzeBadges
  FROM Badges AS b
  GROUP BY
    b.UserId
)
SELECT
  ur.DisplayName,
  ur.Reputation,
  ur.Views,
  ps.TotalPosts,
  ps.TotalQuestions,
  ps.TotalAnswers,
  ps.TotalScore,
  ps.TotalViews,
  aq.FirstAcceptedAnswerDate,
  ub.GoldBadges,
  ub.SilverBadges,
  ub.BronzeBadges,
  CASE
    WHEN aq.FirstAcceptedAnswerDate IS NULL
    THEN 'No Accepted Answers'
    ELSE 'Has Accepted Answers'
  END AS AnswerStatus,
  CASE WHEN ur.UserRank <= 10 THEN 'Top 10 User' ELSE 'Regular User' END AS UserCategory
FROM RankedUsers AS ur
JOIN PostStats AS ps
  ON ur.Id = ps.OwnerUserId
LEFT JOIN AnsweredQuestions AS aq
  ON ur.Id = aq.OwnerUserId
LEFT JOIN UserBadges AS ub
  ON ur.Id = ub.UserId
WHERE
  ur.Reputation > 1000
ORDER BY
  ur.UserRank,
  ps.TotalViews DESC
LIMIT 50
OFFSET 0;
