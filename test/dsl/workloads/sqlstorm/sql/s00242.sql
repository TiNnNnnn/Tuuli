WITH PostInfo AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.AnswerCount,
    u.DisplayName AS OwnerName,
    COALESCE(p.Score, 0) + COALESCE(b.NumBadges, 0) AS EngagementScore
  FROM Posts AS p
  LEFT JOIN Users AS u
    ON p.OwnerUserId = u.Id
  LEFT JOIN (
    SELECT
      UserId,
      COUNT(*) AS NumBadges
    FROM Badges
    WHERE
      Date > CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
    GROUP BY
      UserId
  ) AS b
    ON u.Id = b.UserId
  WHERE
    p.CreationDate > CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '3 MONTHS'
    AND p.ViewCount > 100
), HighScorePosts AS (
  SELECT
    PostId,
    EngagementScore,
    ROW_NUMBER() OVER (ORDER BY EngagementScore DESC) AS Rank
  FROM PostInfo
), RecentVotes AS (
  SELECT
    PostId,
    COUNT(*) AS VoteCount
  FROM Votes
  WHERE
    CreationDate > CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 MONTH'
  GROUP BY
    PostId
)
SELECT
  p.Title,
  p.CreationDate,
  p.Score,
  p.ViewCount,
  COALESCE(v.VoteCount, 0) AS RecentVoteCount,
  h.Rank
FROM PostInfo AS p
LEFT JOIN RecentVotes AS v
  ON p.PostId = v.PostId
JOIN HighScorePosts AS h
  ON p.PostId = h.PostId
WHERE
  h.Rank <= 10
ORDER BY
  p.Score DESC,
  h.Rank;
