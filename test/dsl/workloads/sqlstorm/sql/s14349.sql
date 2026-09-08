WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.AnswerCount,
    (
      SELECT
        COUNT(*)
      FROM Comments AS c
      WHERE
        c.PostId = p.Id
    ) AS CommentCount,
    ROW_NUMBER() OVER (ORDER BY p.CreationDate DESC) AS RowNumber
  FROM Posts AS p
  WHERE
    p.PostTypeId = 1
)
SELECT
  rp.PostId,
  rp.Title,
  rp.CreationDate,
  rp.Score,
  rp.ViewCount,
  rp.AnswerCount,
  rp.CommentCount
FROM RankedPosts AS rp
WHERE
  rp.RowNumber <= 100;
