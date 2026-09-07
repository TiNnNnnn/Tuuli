WITH RankedPosts AS (
  SELECT
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    ARRAY_AGG(DISTINCT t.TagName) AS Tags,
    COUNT(DISTINCT a.Id) AS AnswerCount,
    ROW_NUMBER() OVER (ORDER BY p.Score DESC, p.ViewCount DESC) AS PostRank
  FROM Posts AS p
  LEFT JOIN Posts AS a
    ON p.Id = a.ParentId
  LEFT JOIN UNNEST(STRING_TO_ARRAY(SUBSTRING(p.Tags FROM 2 FOR LENGTH(p.Tags) - 2), '><')) AS tag_name
    ON TRUE
  JOIN Tags AS t
    ON t.TagName = tag_name
  WHERE
    p.PostTypeId = 1
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score
), FilteredPosts AS (
  SELECT
    rp.*,
    (
      SELECT
        COUNT(*)
      FROM Votes AS v
      WHERE
        v.PostId = rp.Id AND v.VoteTypeId = 2
    ) AS Upvotes,
    (
      SELECT
        COUNT(*)
      FROM Votes AS v
      WHERE
        v.PostId = rp.Id AND v.VoteTypeId = 3
    ) AS Downvotes
  FROM RankedPosts AS rp
  WHERE
    rp.AnswerCount > 0
)
SELECT
  fp.Id,
  fp.Title,
  fp.CreationDate,
  fp.ViewCount,
  fp.Score,
  fp.AnswerCount,
  fp.Tags,
  fp.Upvotes,
  fp.Downvotes,
  ROUND(COALESCE(CAST(fp.Upvotes AS REAL) / NULLIF(fp.Upvotes + fp.Downvotes, 0), 0), 2) AS UpvoteRatio
FROM FilteredPosts AS fp
WHERE
  fp.PostRank <= 10
ORDER BY
  fp.Score DESC,
  fp.ViewCount DESC;
