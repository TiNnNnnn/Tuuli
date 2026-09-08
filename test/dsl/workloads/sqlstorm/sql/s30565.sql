WITH RECURSIVE PostHierarchy AS (
  SELECT
    p.Id,
    p.Title,
    p.ParentId,
    0 AS Level
  FROM Posts AS p
  WHERE
    p.ParentId IS NULL
  UNION ALL
  SELECT
    p.Id,
    p.Title,
    p.ParentId,
    ph.Level + 1
  FROM Posts AS p
  INNER JOIN PostHierarchy AS ph
    ON p.ParentId = ph.Id
), UserBadges AS (
  SELECT
    b.UserId,
    COUNT(CASE WHEN b.Class = 1 THEN 1 END) AS GoldBadges,
    COUNT(CASE WHEN b.Class = 2 THEN 1 END) AS SilverBadges,
    COUNT(CASE WHEN b.Class = 3 THEN 1 END) AS BronzeBadges
  FROM Badges AS b
  GROUP BY
    b.UserId
), VoteStatistics AS (
  SELECT
    v.PostId,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes
  FROM Votes AS v
  GROUP BY
    v.PostId
)
SELECT
  p.Id AS PostId,
  p.Title,
  COALESCE(ph.Level, -1) AS HierarchyLevel,
  u.Reputation,
  ub.GoldBadges,
  ub.SilverBadges,
  ub.BronzeBadges,
  vs.UpVotes,
  vs.DownVotes,
  CASE
    WHEN vs.UpVotes IS NULL OR vs.UpVotes <= 0
    THEN 'No Votes'
    ELSE CONCAT('Positive Net Votes: ', vs.UpVotes - COALESCE(vs.DownVotes, 0))
  END AS VoteSummary,
  STRING_AGG(t.TagName, ', ') AS Tags
FROM Posts AS p
LEFT JOIN PostHierarchy AS ph
  ON p.Id = ph.Id
INNER JOIN Users AS u
  ON p.OwnerUserId = u.Id
LEFT JOIN UserBadges AS ub
  ON u.Id = ub.UserId
LEFT JOIN VoteStatistics AS vs
  ON p.Id = vs.PostId
LEFT JOIN LATERAL (
  SELECT
    UNNEST(STRING_TO_ARRAY(p.Tags, ',')) AS TagName
) AS t
  ON TRUE
WHERE
  p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
GROUP BY
  p.Id,
  p.Title,
  ph.Level,
  u.Reputation,
  ub.GoldBadges,
  ub.SilverBadges,
  ub.BronzeBadges,
  vs.UpVotes,
  vs.DownVotes
ORDER BY
  p.LastActivityDate DESC
FETCH FIRST 100 ROWS ONLY;
