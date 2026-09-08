WITH UserActivity AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    COUNT(p.Id) AS PostCount,
    SUM(v.BountyAmount) AS TotalBounty,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes,
    SUM(CASE WHEN NOT b.Id IS NULL THEN 1 ELSE 0 END) AS BadgeCount,
    SUM(COALESCE(p.ViewCount, 0)) AS TotalViews
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
)
SELECT
  ua.DisplayName,
  ua.PostCount,
  ua.TotalBounty,
  ua.UpVotes,
  ua.DownVotes,
  ua.BadgeCount,
  ua.TotalViews,
  RANK() OVER (ORDER BY ua.TotalViews DESC) AS ViewRank,
  RANK() OVER (ORDER BY ua.PostCount DESC) AS PostRank
FROM UserActivity AS ua
ORDER BY
  ua.TotalViews DESC,
  ua.PostCount DESC;
