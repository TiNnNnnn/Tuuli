WITH RankedPosts AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.ViewCount,
    p.Score,
    ROW_NUMBER() OVER (PARTITION BY p.PostTypeId ORDER BY p.CreationDate DESC) AS Rank,
    U.DisplayName AS Author,
    COALESCE(
      (
        SELECT
          COUNT(*)
        FROM Votes AS v
        WHERE
          v.PostId = p.Id AND v.VoteTypeId = 2
      ),
      0
    ) AS Upvotes,
    COALESCE(
      (
        SELECT
          COUNT(*)
        FROM Votes AS v
        WHERE
          v.PostId = p.Id AND v.VoteTypeId = 3
      ),
      0
    ) AS Downvotes,
    (
      SELECT
        COUNT(*)
      FROM Comments AS c
      WHERE
        c.PostId = p.Id
    ) AS CommentCount
  FROM Posts AS p
  JOIN Users AS U
    ON p.OwnerUserId = U.Id
  WHERE
    p.CreationDate >= CAST('2023-01-01' AS DATE)
), ClosedPosts AS (
  SELECT
    ph.PostId,
    ph.CreationDate,
    ph.Comment,
    ph.UserDisplayName,
    P.Title
  FROM PostHistory AS ph
  JOIN Posts AS P
    ON ph.PostId = P.Id
  WHERE
    ph.PostHistoryTypeId = 10
), FilteredPosts AS (
  SELECT
    rp.Title,
    rp.PostId,
    rp.CreationDate,
    rp.ViewCount,
    rp.Score,
    rp.Upvotes,
    rp.Downvotes,
    rp.CommentCount,
    COALESCE(cp.Comment, 'Not closed') AS CloseComment,
    COALESCE(cp.UserDisplayName, 'N/A') AS CloseUser
  FROM RankedPosts AS rp
  LEFT JOIN ClosedPosts AS cp
    ON rp.PostId = cp.PostId
  WHERE
    rp.Rank <= 5
)
SELECT
  p.PostId,
  p.Title,
  p.CreationDate,
  p.ViewCount,
  p.Score,
  p.Upvotes,
  p.Downvotes,
  p.CommentCount,
  (
    p.Upvotes - p.Downvotes
  ) AS NetVotes,
  CASE WHEN p.CommentCount > 0 THEN 'Has Comments' ELSE 'No Comments' END AS CommentStatus,
  CONCAT('Closed Comment: ', p.CloseComment, ' by ', p.CloseUser) AS CloseDetails
FROM FilteredPosts AS p
WHERE
  (
    p.Upvotes - p.Downvotes
  ) > 0
ORDER BY
  p.Score DESC,
  p.CreationDate DESC;
