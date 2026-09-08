WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.Score,
    p.CreationDate,
    p.ViewCount,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC) AS RankScore,
    COUNT(v.Id) OVER (PARTITION BY p.Id) AS VoteCount
  FROM Posts AS p
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
), UserBadges AS (
  SELECT
    b.UserId,
    COUNT(CASE WHEN b.Class = 1 THEN 1 END) AS GoldBadges,
    COUNT(CASE WHEN b.Class = 2 THEN 1 END) AS SilverBadges,
    COUNT(CASE WHEN b.Class = 3 THEN 1 END) AS BronzeBadges
  FROM Badges AS b
  GROUP BY
    b.UserId
), PostHistoryDetails AS (
  SELECT
    ph.PostId,
    ph.PostHistoryTypeId,
    ph.CreationDate AS HistoryDate,
    ph.UserDisplayName,
    STRING_AGG(ph.Comment, ', ') AS Comments
  FROM PostHistory AS ph
  WHERE
    ph.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '6 MONTHS'
  GROUP BY
    ph.PostId,
    ph.PostHistoryTypeId,
    ph.UserDisplayName,
    ph.CreationDate
), FilteredPosts AS (
  SELECT
    rp.PostId,
    rp.Title,
    rp.Score,
    rp.CreationDate,
    rp.ViewCount,
    ub.GoldBadges,
    ub.SilverBadges,
    ub.BronzeBadges,
    pd.HistoryDate,
    pd.UserDisplayName,
    pd.Comments
  FROM RankedPosts AS rp
  LEFT JOIN UserBadges AS ub
    ON rp.PostId IN (
      SELECT
        Id
      FROM Posts
      WHERE
        OwnerUserId = ub.UserId
    )
  LEFT JOIN PostHistoryDetails AS pd
    ON rp.PostId = pd.PostId
  WHERE
    rp.RankScore <= 10
)
SELECT
  fp.PostId,
  fp.Title,
  fp.Score,
  fp.ViewCount,
  COALESCE(fp.GoldBadges, 0) AS GoldBadges,
  COALESCE(fp.SilverBadges, 0) AS SilverBadges,
  COALESCE(fp.BronzeBadges, 0) AS BronzeBadges,
  MAX(fp.HistoryDate) AS LatestHistoryDate,
  STRING_AGG(DISTINCT fp.Comments, ' | ') AS AllComments
FROM FilteredPosts AS fp
GROUP BY
  fp.PostId,
  fp.Title,
  fp.Score,
  fp.ViewCount,
  fp.GoldBadges,
  fp.SilverBadges,
  fp.BronzeBadges
ORDER BY
  fp.Score DESC,
  COUNT(fp.Comments) DESC;
