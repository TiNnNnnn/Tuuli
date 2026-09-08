WITH RankedPosts AS (
  SELECT
    p.Id,
    p.Title,
    p.CreationDate,
    p.Score,
    COUNT(c.Id) AS CommentCount,
    RANK() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC) AS ScoreRank
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  WHERE
    p.CreationDate >= '2023-01-01' AND NOT p.Score IS NULL
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.Score,
    p.PostTypeId
), TopUsers AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    SUM(v.BountyAmount) AS TotalBounty
  FROM Users AS u
  JOIN Votes AS v
    ON u.Id = v.UserId
  WHERE
    v.VoteTypeId IN (8, 9)
  GROUP BY
    u.Id,
    u.DisplayName
  HAVING
    SUM(v.BountyAmount) > 0
), ClosedPosts AS (
  SELECT
    ph.PostId,
    STRING_AGG(DISTINCT ctr.Name, ', ') AS ClosedReasons
  FROM PostHistory AS ph
  JOIN CloseReasonTypes AS ctr
    ON CAST(ph.Comment AS INT) = ctr.Id
  WHERE
    ph.PostHistoryTypeId = 10
  GROUP BY
    ph.PostId
)
SELECT
  rp.Title,
  rp.Score,
  rp.CommentCount,
  tu.DisplayName AS TopUser,
  tu.TotalBounty,
  cp.ClosedReasons
FROM RankedPosts AS rp
LEFT JOIN TopUsers AS tu
  ON rp.ScoreRank = 1 AND NOT tu.TotalBounty IS NULL
LEFT JOIN ClosedPosts AS cp
  ON rp.Id = cp.PostId
WHERE
  rp.Score > 0 AND COALESCE(cp.ClosedReasons, '') <> ''
ORDER BY
  rp.Score DESC,
  rp.CreationDate DESC;
