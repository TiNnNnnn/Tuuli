WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.Body,
    p.Tags,
    u.DisplayName AS OwnerDisplayName,
    u.Reputation AS OwnerReputation,
    p.CreationDate,
    p.LastActivityDate,
    p.ViewCount,
    p.Score,
    ROW_NUMBER() OVER (PARTITION BY u.Id ORDER BY p.CreationDate DESC) AS PostRank
  FROM Posts AS p
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  WHERE
    p.PostTypeId = 1
    AND p.CreationDate > CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
), TaggedQuestions AS (
  SELECT
    PostId,
    Title,
    Body,
    Tags,
    OwnerDisplayName,
    OwnerReputation,
    CreationDate,
    LastActivityDate,
    ViewCount,
    Score
  FROM RankedPosts
  WHERE
    PostRank = 1 AND NOT Tags IS NULL
), MostCommonTags AS (
  SELECT
    TRIM(UNNEST(STRING_TO_ARRAY(Tags, ','))) AS TagName,
    COUNT(*) AS TagCount
  FROM TaggedQuestions
  GROUP BY
    TagName
  ORDER BY
    TagCount DESC
  LIMIT 10
)
SELECT
  tq.OwnerDisplayName,
  tq.Title,
  tq.Body,
  tq.Tags,
  tq.ViewCount,
  tq.Score,
  ct.TagName,
  ct.TagCount
FROM TaggedQuestions AS tq
JOIN MostCommonTags AS ct
  ON STRPOS(tq.Tags, ct.TagName) > 0
ORDER BY
  tq.Score DESC,
  tq.ViewCount DESC;
