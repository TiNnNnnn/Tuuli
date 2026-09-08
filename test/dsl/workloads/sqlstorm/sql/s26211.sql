WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.Body,
    p.CreationDate,
    p.Tags,
    u.DisplayName AS Author,
    p.Score,
    ROW_NUMBER() OVER (PARTITION BY p.Tags ORDER BY p.Score DESC) AS TagRank
  FROM Posts AS p
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  WHERE
    p.PostTypeId = 1
    AND p.CreationDate > (
      CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
    )
), TagStatistics AS (
  SELECT
    UNNEST(STRING_TO_ARRAY(Tags, '>')) AS Tag,
    COUNT(*) AS TotalPosts,
    SUM(CASE WHEN Score > 0 THEN 1 ELSE 0 END) AS UpvotedPosts,
    SUM(CASE WHEN Score < 0 THEN 1 ELSE 0 END) AS DownvotedPosts
  FROM Posts
  WHERE
    PostTypeId = 1
  GROUP BY
    UNNEST(STRING_TO_ARRAY(Tags, '>')) /* Group by the Tag from the UNNEST function */
), PostHistoryStats AS (
  SELECT
    ph.PostId,
    COUNT(CASE WHEN ph.PostHistoryTypeId = 10 THEN 1 END) AS CloseVotes,
    COUNT(CASE WHEN ph.PostHistoryTypeId = 11 THEN 1 END) AS ReopenVotes,
    MAX(ph.CreationDate) AS LastEdit
  FROM PostHistory AS ph
  GROUP BY
    ph.PostId
)
SELECT
  rp.PostId,
  rp.Title,
  rp.Body,
  rp.CreationDate,
  rp.Tags,
  rp.Author,
  rp.Score,
  ts.TotalPosts,
  ts.UpvotedPosts,
  ts.DownvotedPosts,
  phs.CloseVotes,
  phs.ReopenVotes,
  phs.LastEdit
FROM RankedPosts AS rp
JOIN TagStatistics AS ts
  ON ts.Tag = ANY(
    STRING_TO_ARRAY(rp.Tags, '>')
  )
JOIN PostHistoryStats AS phs
  ON phs.PostId = rp.PostId
WHERE
  rp.TagRank <= 5
ORDER BY
  rp.Score DESC,
  rp.CreationDate DESC;
