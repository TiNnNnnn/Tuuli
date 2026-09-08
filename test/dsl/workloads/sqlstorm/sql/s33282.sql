WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    u.Id AS UserId,
    u.DisplayName AS OwnerDisplayName,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.Score DESC) AS RankPerUser
  FROM Posts AS p
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  WHERE
    p.CreationDate >= CAST('2024-10-01' AS DATE) - INTERVAL '1 YEAR'
    AND p.PostTypeId = 1
), PostVoteCounts AS (
  SELECT
    v.PostId,
    COUNT(CASE WHEN v.VoteTypeId = 2 THEN 1 END) AS UpVotes,
    COUNT(CASE WHEN v.VoteTypeId = 3 THEN 1 END) AS DownVotes
  FROM Votes AS v
  GROUP BY
    v.PostId
), AggregatedData AS (
  SELECT
    rp.PostId,
    rp.Title,
    rp.CreationDate,
    rp.Score,
    rp.ViewCount,
    rp.OwnerDisplayName,
    COALESCE(pvc.UpVotes, 0) AS TotalUpVotes,
    COALESCE(pvc.DownVotes, 0) AS TotalDownVotes,
    rp.RankPerUser
  FROM RankedPosts AS rp
  LEFT JOIN PostVoteCounts AS pvc
    ON rp.PostId = pvc.PostId
), TopPosts AS (
  SELECT
    PostId,
    Title,
    CreationDate,
    Score,
    ViewCount,
    OwnerDisplayName,
    TotalUpVotes,
    TotalDownVotes,
    RankPerUser
  FROM AggregatedData
  WHERE
    RankPerUser <= 5
), PostHistorySummary AS (
  SELECT
    ph.PostId,
    MIN(ph.CreationDate) AS FirstHistoryDate,
    COUNT(*) AS TotalEdits,
    SUM(CASE WHEN ph.PostHistoryTypeId = 10 THEN 1 ELSE 0 END) AS CloseVotes
  FROM PostHistory AS ph
  GROUP BY
    ph.PostId
)
SELECT
  tp.PostId,
  tp.Title,
  tp.OwnerDisplayName,
  tp.CreationDate,
  tp.Score,
  tp.ViewCount,
  tp.TotalUpVotes,
  tp.TotalDownVotes,
  phs.FirstHistoryDate,
  phs.TotalEdits,
  phs.CloseVotes,
  CASE WHEN phs.TotalEdits > 0 THEN 'Edited' ELSE 'Not Edited' END AS EditStatus
FROM TopPosts AS tp
LEFT JOIN PostHistorySummary AS phs
  ON tp.PostId = phs.PostId
ORDER BY
  tp.Score DESC,
  tp.CreationDate DESC;
