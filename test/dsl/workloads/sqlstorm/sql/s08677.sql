WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.Score,
    p.CreationDate,
    COUNT(c.Id) AS CommentCount,
    COUNT(v.Id) FILTER(WHERE
      v.VoteTypeId = 2) AS UpvoteCount,
    COUNT(v.Id) FILTER(WHERE
      v.VoteTypeId = 3) AS DownvoteCount,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.Score DESC, p.CreationDate DESC) AS Rank
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  WHERE
    p.PostTypeId IN (1, 2)
  GROUP BY
    p.Id,
    p.Title,
    p.Score,
    p.CreationDate
), TopPosts AS (
  SELECT
    rp.PostId,
    rp.Title,
    rp.Score,
    rp.CommentCount,
    rp.UpvoteCount,
    rp.DownvoteCount
  FROM RankedPosts AS rp
  WHERE
    rp.Rank <= 10
)
SELECT
  tp.Title,
  tp.Score,
  tp.CommentCount,
  tp.UpvoteCount,
  tp.DownvoteCount,
  COALESCE(b.Name, 'No Badge') AS UserBadgeName,
  COUNT(DISTINCT pl.RelatedPostId) AS RelatedPostsCount,
  COUNT(c.Id) AS TotalComments
FROM TopPosts AS tp
LEFT JOIN Posts AS p
  ON tp.PostId = p.Id
LEFT JOIN Badges AS b
  ON p.OwnerUserId = b.UserId AND b.Date <= p.CreationDate
LEFT JOIN PostLinks AS pl
  ON tp.PostId = pl.PostId
LEFT JOIN Comments AS c
  ON tp.PostId = c.PostId
GROUP BY
  tp.PostId,
  tp.Title,
  tp.Score,
  tp.CommentCount,
  tp.UpvoteCount,
  tp.DownvoteCount,
  b.Name
ORDER BY
  tp.Score DESC,
  tp.CommentCount DESC;
