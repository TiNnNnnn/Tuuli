WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.ViewCount,
    p.Score,
    p.CreationDate,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.ViewCount DESC, p.Score DESC) AS Rank,
    COALESCE(p.AcceptedAnswerId, -1) AS AnswerStatus
  FROM Posts AS p
  WHERE
    p.CreationDate >= CAST('2024-10-01' AS DATE) - INTERVAL '1 YEAR'
    AND NOT p.ViewCount IS NULL
), PostStatistics AS (
  SELECT
    u.Id AS UserId,
    SUM(CASE WHEN p.AnswerCount > 0 THEN 1 ELSE 0 END) AS TotalQuestionsAnswered,
    COUNT(DISTINCT p.Id) AS TotalPosts,
    AVG(p.Score) AS AverageScore,
    SUM(CASE WHEN b.Class = 1 THEN 1 ELSE 0 END) AS GoldBadges,
    SUM(CASE WHEN b.Class = 2 THEN 1 ELSE 0 END) AS SilverBadges,
    SUM(CASE WHEN b.Class = 3 THEN 1 ELSE 0 END) AS BronzeBadges
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id
), ClosedPostReasons AS (
  SELECT
    ph.PostId,
    COUNT(*) AS CloseVoteCount,
    STRING_AGG(CASE WHEN ph.PostHistoryTypeId = 10 THEN cr.Name END, ', ') AS CloseReasons
  FROM PostHistory AS ph
  LEFT JOIN CloseReasonTypes AS cr
    ON CAST(cr.Id AS TEXT) = ph.Comment
  GROUP BY
    ph.PostId
), UserPostLinkages AS (
  SELECT
    pl.PostId,
    pl.RelatedPostId,
    COUNT(pl.Id) AS LinkCount
  FROM PostLinks AS pl
  JOIN Posts AS p
    ON pl.PostId = p.Id
  WHERE
    p.CreationDate < CAST('2024-10-01' AS DATE) - INTERVAL '6 MONTHS'
  GROUP BY
    pl.PostId,
    pl.RelatedPostId
), FinalStats AS (
  SELECT
    ps.UserId,
    ps.TotalQuestionsAnswered,
    ps.TotalPosts,
    ps.AverageScore,
    ps.GoldBadges,
    ps.SilverBadges,
    ps.BronzeBadges,
    COALESCE(rp.PostId, 0) AS TopPostId,
    COALESCE(rp.Title, 'No Trending Post') AS TopPostTitle,
    COALESCE(rp.ViewCount, 0) AS TopPostViewCount,
    COALESCE(rp.Score, 0) AS TopPostScore,
    COALESCE(cpr.CloseVoteCount, 0) AS TotalCloseVotes,
    COALESCE(cpr.CloseReasons, 'No Close Reasons') AS CloseReasons,
    COALESCE(pl.LinkCount, 0) AS TotalRelatedLinks
  FROM PostStatistics AS ps
  LEFT JOIN RankedPosts AS rp
    ON ps.UserId = rp.PostId
  LEFT JOIN ClosedPostReasons AS cpr
    ON rp.PostId = cpr.PostId
  LEFT JOIN UserPostLinkages AS pl
    ON ps.UserId = pl.PostId
)
SELECT
  *,
  CASE WHEN TotalPosts = 0 THEN 'No activity' ELSE 'Active User' END AS UserActivityStatus
FROM FinalStats
WHERE
  TotalQuestionsAnswered > 5 AND GoldBadges > 0
ORDER BY
  TotalPosts DESC,
  AverageScore DESC;
