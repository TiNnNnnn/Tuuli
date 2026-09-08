WITH RankedPosts AS (
  SELECT
    P.Id AS PostId,
    P.Title,
    U.DisplayName AS Author,
    P.CreationDate,
    P.ViewCount,
    P.Score,
    P.AnswerCount,
    P.CommentCount,
    RANK() OVER (PARTITION BY P.PostTypeId ORDER BY P.ViewCount DESC, P.Score DESC) AS RankByPopularity
  FROM Posts AS P
  JOIN Users AS U
    ON P.OwnerUserId = U.Id
  WHERE
    P.CreationDate >= '2023-01-01'
), TopPosts AS (
  SELECT
    PostId,
    Title,
    Author,
    CreationDate,
    ViewCount,
    Score,
    AnswerCount,
    CommentCount
  FROM RankedPosts
  WHERE
    RankByPopularity <= 10
), PostDetails AS (
  SELECT
    TP.*,
    ARRAY_AGG(DISTINCT T.TagName) AS Tags,
    COALESCE(SUM(V.BountyAmount), 0) AS TotalBounties
  FROM TopPosts AS TP
  LEFT JOIN Posts AS P
    ON TP.PostId = P.Id
  LEFT JOIN PostLinks AS PL
    ON P.Id = PL.PostId
  LEFT JOIN Tags AS T
    ON PL.RelatedPostId = T.Id
  LEFT JOIN Votes AS V
    ON P.Id = V.PostId AND V.VoteTypeId = 8
  GROUP BY
    TP.PostId,
    TP.Title,
    TP.Author,
    TP.CreationDate,
    TP.ViewCount,
    TP.Score,
    TP.AnswerCount,
    TP.CommentCount
)
SELECT
  PD.PostId,
  PD.Title,
  PD.Author,
  PD.CreationDate,
  PD.ViewCount,
  PD.Score,
  PD.AnswerCount,
  PD.CommentCount,
  PD.Tags,
  PD.TotalBounties
FROM PostDetails AS PD
ORDER BY
  PD.ViewCount DESC,
  PD.Score DESC;
