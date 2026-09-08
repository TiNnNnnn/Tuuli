WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    COUNT(DISTINCT c.Id) AS CommentCount,
    COUNT(DISTINCT a.Id) AS AnswerCount,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.CreationDate DESC) AS PostRank
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Posts AS a
    ON p.Id = a.ParentId
  WHERE
    p.PostTypeId = 1
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    p.OwnerUserId
), UserScores AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    SUM(p.Score) AS TotalScore,
    SUM(p.ViewCount) AS TotalViews,
    SUM(COALESCE(b.Class, 0)) AS TotalBadges
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.DisplayName
), TopPerformers AS (
  SELECT
    us.UserId,
    us.DisplayName,
    us.TotalScore,
    us.TotalViews,
    us.TotalBadges,
    ROW_NUMBER() OVER (ORDER BY us.TotalScore DESC, us.TotalViews DESC) AS Ranking
  FROM UserScores AS us
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.ViewCount,
  rp.Score,
  rp.CommentCount,
  rp.AnswerCount,
  tp.DisplayName AS TopUser,
  tp.TotalScore
FROM RankedPosts AS rp
JOIN TopPerformers AS tp
  ON rp.PostRank = 1
WHERE
  tp.Ranking <= 10
ORDER BY
  rp.Score DESC,
  rp.ViewCount DESC;
