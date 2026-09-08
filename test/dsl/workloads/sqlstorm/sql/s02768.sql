WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.AnswerCount,
    ROW_NUMBER() OVER (PARTITION BY p.Id ORDER BY p.CreationDate DESC) AS rn
  FROM Posts AS p
  INNER JOIN Users AS u
    ON p.OwnerUserId = u.Id
  WHERE
    u.Reputation > 50
), PostVoteStatistics AS (
  SELECT
    v.PostId,
    COUNT(CASE WHEN v.VoteTypeId = 2 THEN 1 END) AS UpVotes,
    COUNT(CASE WHEN v.VoteTypeId = 3 THEN 1 END) AS DownVotes
  FROM Votes AS v
  GROUP BY
    v.PostId
), ClosedPosts AS (
  SELECT
    ph.PostId,
    ph.CreationDate,
    STRING_AGG(ct.Name, ', ') AS CloseReasons
  FROM PostHistory AS ph
  JOIN CloseReasonTypes AS ct
    ON CAST(ph.Comment AS INT) = ct.Id
  WHERE
    ph.PostHistoryTypeId = 10
  GROUP BY
    ph.PostId,
    ph.CreationDate
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.ViewCount,
  COALESCE(pvs.UpVotes, 0) AS TotalUpVotes,
  COALESCE(pvs.DownVotes, 0) AS TotalDownVotes,
  cp.CloseReasons,
  CASE WHEN NOT cp.CloseReasons IS NULL THEN 'Closed' ELSE 'Open' END AS PostStatus
FROM RankedPosts AS rp
LEFT JOIN PostVoteStatistics AS pvs
  ON rp.PostId = pvs.PostId
LEFT JOIN ClosedPosts AS cp
  ON rp.PostId = cp.PostId
WHERE
  rp.rn = 1
  AND (
    cp.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
    OR cp.CreationDate IS NULL
  )
ORDER BY
  rp.ViewCount DESC
LIMIT 10;
