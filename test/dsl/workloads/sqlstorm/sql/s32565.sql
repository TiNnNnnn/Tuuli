WITH RECURSIVE PostHierarchy AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.PostTypeId,
    p.ParentId,
    1 AS Level
  FROM Posts AS p
  WHERE
    p.PostTypeId = 1
  UNION ALL
  SELECT
    p.Id,
    p.Title,
    p.PostTypeId,
    p.ParentId,
    ph.Level + 1
  FROM Posts AS p
  INNER JOIN PostHierarchy AS ph
    ON p.ParentId = ph.PostId
), PostMetrics AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    COALESCE(p.Score, 0) AS Score,
    COALESCE(p.ViewCount, 0) AS ViewCount,
    COUNT(c.Id) AS CommentCount,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes,
    COUNT(DISTINCT b.Id) AS BadgeCount
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  LEFT JOIN Badges AS b
    ON p.OwnerUserId = b.UserId
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
  GROUP BY
    p.Id,
    p.Title
), ClosedPostDetails AS (
  SELECT
    ph.PostId,
    ph.Title,
    COUNT(CASE WHEN ph.PostTypeId = 1 THEN 1 END) AS QuestionCount,
    COUNT(CASE WHEN ph.PostTypeId = 2 THEN 1 END) AS AnswerCount,
    MAX(
      CASE
        WHEN ph.PostTypeId = 1
        AND (
          NOT p.ClosedDate IS NULL
          OR p.LastActivityDate < CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
        )
        THEN 1
        ELSE 0
      END
    ) AS IsClosed
  FROM PostHierarchy AS ph
  LEFT JOIN Posts AS p
    ON ph.PostId = p.Id
  GROUP BY
    ph.PostId,
    ph.Title
)
SELECT
  pm.PostId,
  pm.Title,
  pm.Score,
  pm.ViewCount,
  pm.CommentCount,
  pm.UpVotes,
  pm.DownVotes,
  pm.BadgeCount,
  cp.QuestionCount,
  cp.AnswerCount,
  cp.IsClosed
FROM PostMetrics AS pm
LEFT JOIN ClosedPostDetails AS cp
  ON pm.PostId = cp.PostId
WHERE
  pm.Score > 10
ORDER BY
  pm.ViewCount DESC,
  pm.Score DESC;
