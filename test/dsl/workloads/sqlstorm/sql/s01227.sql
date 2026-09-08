WITH UserActivity AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    COUNT(DISTINCT p.Id) AS TotalPosts,
    COUNT(DISTINCT c.Id) AS TotalComments,
    SUM(CASE WHEN v.VoteTypeId IN (2, 3) THEN 1 ELSE 0 END) AS TotalVotes,
    AVG(
      EXTRACT(EPOCH FROM (
        CAST('2024-10-01 12:34:56' AS TIMESTAMP) - u.CreationDate
      ))
    ) AS AvgAccountAgeInSec
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Comments AS c
    ON u.Id = c.UserId
  LEFT JOIN Votes AS v
    ON u.Id = v.UserId
  GROUP BY
    u.Id,
    u.DisplayName
), RecentPostHistory AS (
  SELECT
    ph.PostId,
    ph.CreationDate,
    ph.Comment,
    p.Title,
    p.OwnerUserId,
    ROW_NUMBER() OVER (PARTITION BY ph.PostId ORDER BY ph.CreationDate DESC) AS rn
  FROM PostHistory AS ph
  JOIN Posts AS p
    ON ph.PostId = p.Id
  WHERE
    ph.CreationDate > CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
)
SELECT
  ua.DisplayName,
  ua.TotalPosts,
  ua.TotalComments,
  ua.TotalVotes,
  ua.AvgAccountAgeInSec,
  rp.Title,
  rp.Comment,
  rp.CreationDate
FROM UserActivity AS ua
LEFT JOIN RecentPostHistory AS rp
  ON ua.UserId = rp.OwnerUserId AND rp.rn = 1
WHERE
  ua.TotalPosts > 10 OR ua.TotalComments > 50
ORDER BY
  ua.TotalVotes DESC,
  ua.TotalPosts ASC;
