WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    u.DisplayName AS OwnerDisplayName,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC, p.CreationDate DESC) AS Rank
  FROM Posts AS p
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  WHERE
    p.PostTypeId IN (1, 2)
    AND p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
), PopularTags AS (
  SELECT
    t.TagName,
    COUNT(DISTINCT p.Id) AS PostCount
  FROM Tags AS t
  JOIN Posts AS p
    ON p.Tags LIKE CONCAT('%', t.TagName, '%')
  GROUP BY
    t.TagName
  ORDER BY
    PostCount DESC
  LIMIT 10
), PostDetails AS (
  SELECT
    rp.PostId,
    rp.Title,
    rp.CreationDate,
    rp.Score,
    rp.ViewCount,
    rp.OwnerDisplayName,
    pt.Name AS PostType,
    COALESCE(cht.Name, 'No Close Reason') AS CloseReason
  FROM RankedPosts AS rp
  LEFT JOIN CloseReasonTypes AS cht
    ON EXISTS(
      SELECT
        1
      FROM PostHistory AS ph
      WHERE
        ph.PostId = rp.PostId AND ph.PostHistoryTypeId = 10
    )
  JOIN PostTypes AS pt
    ON pt.Id = (
      SELECT
        PostTypeId
      FROM Posts
      WHERE
        Id = rp.PostId
    )
  WHERE
    rp.Rank <= 10
)
SELECT
  pd.PostId,
  pd.Title,
  pd.CreationDate,
  pd.Score,
  pd.ViewCount,
  pd.OwnerDisplayName,
  pd.PostType,
  pt.TagName AS MostPopularTag
FROM PostDetails AS pd
JOIN PopularTags AS pt
  ON pd.PostId IN (
    SELECT
      PostId
    FROM Posts
    WHERE
      Tags LIKE CONCAT('%', pt.TagName, '%')
  )
ORDER BY
  pd.Score DESC,
  pd.ViewCount DESC;
