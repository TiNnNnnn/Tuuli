WITH RecentPosts AS (
  SELECT
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    u.DisplayName AS OwnerName,
    COUNT(c.Id) AS CommentCount,
    COUNT(DISTINCT v.UserId) AS VoteCount
  FROM Posts AS p
  LEFT JOIN Users AS u
    ON p.OwnerUserId = u.Id
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId AND v.VoteTypeId = 2
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    u.DisplayName
), PostHistoryInfo AS (
  SELECT
    ph.PostId,
    STRING_AGG(DISTINCT pht.Name, ', ') AS EditHistory,
    MAX(ph.CreationDate) AS LastEditDate
  FROM PostHistory AS ph
  JOIN PostHistoryTypes AS pht
    ON ph.PostHistoryTypeId = pht.Id
  GROUP BY
    ph.PostId
), TopPosts AS (
  SELECT
    rp.*,
    COALESCE(phe.EditHistory, 'No edits') AS EditHistory,
    phe.LastEditDate
  FROM RecentPosts AS rp
  LEFT JOIN PostHistoryInfo AS phe
    ON rp.Id = phe.PostId
  WHERE
    rp.ViewCount > (
      SELECT
        AVG(ViewCount)
      FROM RecentPosts
    )
)
SELECT
  tp.Id,
  tp.Title,
  tp.CreationDate,
  tp.ViewCount,
  tp.OwnerName,
  tp.CommentCount,
  tp.VoteCount,
  tp.EditHistory,
  tp.LastEditDate,
  CASE
    WHEN tp.CommentCount > 10
    THEN 'High Engagement'
    WHEN tp.CommentCount BETWEEN 5 AND 10
    THEN 'Moderate Engagement'
    ELSE 'Low Engagement'
  END AS EngagementLevel
FROM TopPosts AS tp
ORDER BY
  tp.ViewCount DESC,
  tp.CreationDate ASC
LIMIT 100;
