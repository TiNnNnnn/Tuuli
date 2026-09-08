SELECT
  u.Id AS UserId,
  u.DisplayName,
  COUNT(p.Id) AS PostCount
FROM Users AS u
LEFT JOIN Posts AS p
  ON u.Id = p.OwnerUserId
GROUP BY
  u.Id,
  u.DisplayName
ORDER BY
  PostCount DESC
LIMIT 10;
