WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    ROW_NUMBER() OVER (PARTITION BY pt.Name ORDER BY p.CreationDate DESC) AS Rank,
    COALESCE(NULLIF(u.Location, ''), 'Unknown Location') AS UserLocation
  FROM Posts AS p
  JOIN PostTypes AS pt
    ON p.PostTypeId = pt.Id
  LEFT JOIN Users AS u
    ON p.OwnerUserId = u.Id
  WHERE
    p.CreationDate >= CURRENT_DATE - INTERVAL '1 YEAR'
    AND p.Score > (
      SELECT
        AVG(Score)
      FROM Posts
    )
), PostComments AS (
  SELECT
    c.PostId,
    COUNT(*) AS CommentCount,
    STRING_AGG(c.Text, '; ' ORDER BY c.CreationDate) AS CommentsSummary
  FROM Comments AS c
  GROUP BY
    c.PostId
), UserBadges AS (
  SELECT
    b.UserId,
    COUNT(*) AS BadgeCount,
    STRING_AGG(b.Name, ', ') AS Badges
  FROM Badges AS b
  WHERE
    b.Class = 1
  GROUP BY
    b.UserId
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.Score,
  rp.ViewCount,
  COALESCE(pc.CommentCount, 0) AS NumberOfComments,
  COALESCE(pc.CommentsSummary, 'No comments') AS CommentSnippet,
  ub.BadgeCount,
  ub.Badges,
  rp.UserLocation
FROM RankedPosts AS rp
LEFT JOIN PostComments AS pc
  ON rp.PostId = pc.PostId
LEFT JOIN UserBadges AS ub
  ON NOT rp.UserLocation IS NULL
  AND rp.UserLocation <> 'Unknown Location'
  AND EXISTS(
    SELECT
      1
    FROM Users AS u
    WHERE
      u.Id = ub.UserId AND COALESCE(u.Location, '') = rp.UserLocation
  )
WHERE
  (
    rp.Rank <= 5 OR ub.BadgeCount > 0
  )
ORDER BY
  rp.Score DESC,
  rp.CreationDate ASC
OFFSET 10
FETCH NEXT 10 ROWS ONLY;
