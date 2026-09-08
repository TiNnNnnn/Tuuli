WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.Score,
    p.ViewCount,
    p.CreationDate,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC) AS Rank
  FROM Posts AS p
  WHERE
    p.CreationDate >= CURRENT_DATE - INTERVAL '1 YEAR'
), PostVoteDetails AS (
  SELECT
    v.PostId,
    SUM(CASE WHEN vt.Name = 'UpMod' THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN vt.Name = 'DownMod' THEN 1 ELSE 0 END) AS DownVotes
  FROM Votes AS v
  INNER JOIN VoteTypes AS vt
    ON v.VoteTypeId = vt.Id
  GROUP BY
    v.PostId
), PostHistoryFilter AS (
  SELECT
    ph.PostId,
    COUNT(CASE WHEN pht.Name IN ('Post Locked', 'Post Closed') THEN 1 END) AS LockCloseCount
  FROM PostHistory AS ph
  INNER JOIN PostHistoryTypes AS pht
    ON ph.PostHistoryTypeId = pht.Id
  GROUP BY
    ph.PostId
)
SELECT
  rp.PostId,
  rp.Title,
  rp.Score,
  COALESCE(pvd.UpVotes, 0) AS UpVotes,
  COALESCE(pvd.DownVotes, 0) AS DownVotes,
  rp.ViewCount,
  rp.CreationDate,
  COALESCE(phf.LockCloseCount, 0) AS LockCloseCount,
  CASE WHEN rp.Rank <= 3 THEN 'Top Post' ELSE 'Regular Post' END AS PostCategory
FROM RankedPosts AS rp
LEFT JOIN PostVoteDetails AS pvd
  ON rp.PostId = pvd.PostId
LEFT JOIN PostHistoryFilter AS phf
  ON rp.PostId = phf.PostId
WHERE
  COALESCE(phf.LockCloseCount, 0) > 0 AND rp.Rank <= 10
ORDER BY
  rp.Score DESC,
  rp.ViewCount DESC;
