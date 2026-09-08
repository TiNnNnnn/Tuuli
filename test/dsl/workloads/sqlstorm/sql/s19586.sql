SELECT
  u.Id AS UserId,
  u.DisplayName,
  COUNT(p.Id) AS PostCount,
  SUM(CASE WHEN p.PostTypeId = 1 THEN 1 ELSE 0 END) AS QuestionCount,
  SUM(CASE WHEN p.PostTypeId = 2 THEN 1 ELSE 0 END) AS AnswerCount
FROM Users AS u
LEFT JOIN Posts AS p
  ON u.Id = p.OwnerUserId
GROUP BY
  u.Id,
  u.DisplayName
ORDER BY
  PostCount DESC
LIMIT 10;
