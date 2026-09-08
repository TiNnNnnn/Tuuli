WITH UserPostStats AS (
  SELECT
    u.Id AS UserId,
    u.Reputation,
    COUNT(p.Id) AS TotalPosts,
    SUM(CASE WHEN p.PostTypeId = 2 THEN 1 ELSE 0 END) AS TotalAnswers,
    SUM(CASE WHEN p.PostTypeId = 1 AND NOT p.AcceptedAnswerId IS NULL THEN 1 ELSE 0 END) AS AcceptedAnswers
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  GROUP BY
    u.Id,
    u.Reputation
), TopUsers AS (
  SELECT
    UserId,
    Reputation,
    TotalPosts,
    TotalAnswers,
    AcceptedAnswers,
    RANK() OVER (ORDER BY Reputation DESC) AS ReputationRank
  FROM UserPostStats
), UserBadges AS (
  SELECT
    b.UserId,
    COUNT(b.Id) AS BadgeCount,
    STRING_AGG(b.Name, ', ') AS BadgeNames
  FROM Badges AS b
  GROUP BY
    b.UserId
)
SELECT
  u.UserId,
  u.Reputation,
  u.TotalPosts,
  u.TotalAnswers,
  u.AcceptedAnswers,
  ub.BadgeCount,
  COALESCE(ub.BadgeNames, 'No Badges') AS BadgeNames,
  CASE WHEN u.AcceptedAnswers > 0 THEN 'Yes' ELSE 'No' END AS HasAcceptedAnswers
FROM TopUsers AS u
LEFT JOIN UserBadges AS ub
  ON u.UserId = ub.UserId
WHERE
  u.ReputationRank <= 10
ORDER BY
  u.Reputation DESC;
