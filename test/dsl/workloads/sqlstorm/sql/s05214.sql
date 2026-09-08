WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    COUNT(c.Id) AS CommentCount,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpvoteCount,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownvoteCount,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY COUNT(c.Id) DESC, SUM(v.VoteTypeId) DESC) AS Rank
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  WHERE
    p.CreationDate >= CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 YEAR'
  GROUP BY
    p.Id,
    p.Title,
    p.PostTypeId
), TopPosts AS (
  SELECT
    rp.PostId,
    rp.Title,
    rp.CommentCount,
    rp.UpvoteCount,
    rp.DownvoteCount
  FROM RankedPosts AS rp
  WHERE
    rp.Rank <= 5
)
SELECT
  tp.PostId,
  tp.Title,
  tp.CommentCount,
  tp.UpvoteCount,
  tp.DownvoteCount,
  CASE
    WHEN tp.UpvoteCount - tp.DownvoteCount > 0
    THEN 'Positive'
    WHEN tp.UpvoteCount - tp.DownvoteCount < 0
    THEN 'Negative'
    ELSE 'Neutral'
  END AS Sentiment
FROM TopPosts AS tp
ORDER BY
  tp.UpvoteCount DESC;
