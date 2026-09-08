WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.AnswerCount,
    p.CommentCount,
    ROW_NUMBER() OVER (PARTITION BY pt.Name ORDER BY p.Score DESC, p.ViewCount DESC) AS Rank,
    STRING_AGG(DISTINCT t.TagName, ', ') AS Tags
  FROM Posts AS p
  JOIN PostTypes AS pt
    ON p.PostTypeId = pt.Id
  LEFT JOIN UNNEST(STRING_TO_ARRAY(SUBSTRING(p.Tags FROM 2 FOR LENGTH(p.Tags) - 2), '><')) AS tag
    ON NOT tag IS NULL
  LEFT JOIN Tags AS t
    ON t.TagName = tag
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.AnswerCount,
    p.CommentCount,
    pt.Name
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.Score,
  rp.ViewCount,
  rp.AnswerCount,
  rp.CommentCount,
  rp.Rank,
  (
    SELECT
      COUNT(*)
    FROM Comments AS c
    WHERE
      c.PostId = rp.PostId
  ) AS TotalComments,
  (
    SELECT
      COUNT(*)
    FROM Votes AS v
    WHERE
      v.PostId = rp.PostId AND v.VoteTypeId = 2
  ) AS TotalUpvotes,
  rp.Tags
FROM RankedPosts AS rp
WHERE
  rp.Rank <= 5
ORDER BY
  rp.Score DESC
FETCH FIRST 50 ROWS ONLY;
