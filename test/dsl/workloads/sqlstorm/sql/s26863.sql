WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    u.DisplayName AS Author,
    COUNT(c.Id) AS CommentCount,
    COUNT(a.Id) AS AnswerCount,
    p.Score,
    ROW_NUMBER() OVER (PARTITION BY p.OwnerUserId ORDER BY p.CreationDate DESC) AS Rank
  FROM Posts AS p
  LEFT JOIN Users AS u
    ON p.OwnerUserId = u.Id
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Posts AS a
    ON p.Id = a.ParentId AND a.PostTypeId = 2
  WHERE
    p.PostTypeId = 1
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    u.DisplayName,
    p.Score,
    p.OwnerUserId
), PostTagCount AS (
  SELECT
    p.Id AS PostId,
    COUNT(DISTINCT t.TagName) AS TagCount
  FROM Posts AS p
  LEFT JOIN UNNEST(STRING_TO_ARRAY(SUBSTRING(p.Tags FROM 2 FOR LENGTH(p.Tags) - 2), '><')) AS tag_names
    ON TRUE
  LEFT JOIN Tags AS t
    ON t.TagName = tag_names
  GROUP BY
    p.Id
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.ViewCount,
  rp.Author,
  rp.CommentCount,
  rp.AnswerCount,
  rp.Score,
  COALESCE(pt.TagCount, 0) AS UniqueTagCount,
  CASE
    WHEN rp.Score > 0
    THEN 'Positive'
    WHEN rp.Score < 0
    THEN 'Negative'
    ELSE 'Neutral'
  END AS ScoreCategory
FROM RankedPosts AS rp
LEFT JOIN PostTagCount AS pt
  ON rp.PostId = pt.PostId
WHERE
  rp.Rank = 1
ORDER BY
  rp.ViewCount DESC,
  rp.Score DESC
LIMIT 10;
