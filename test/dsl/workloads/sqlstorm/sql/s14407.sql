WITH PostStats AS (
  SELECT
    P.Id AS PostId,
    P.PostTypeId,
    P.CreationDate,
    P.Score,
    P.ViewCount,
    P.AnswerCount,
    P.CommentCount,
    P.FavoriteCount,
    U.Reputation AS OwnerReputation,
    U.DisplayName AS OwnerDisplayName
  FROM Posts AS P
  JOIN Users AS U
    ON P.OwnerUserId = U.Id
), VoteStats AS (
  SELECT
    V.PostId,
    COUNT(V.Id) AS VoteCount,
    SUM(CASE WHEN V.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN V.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes
  FROM Votes AS V
  GROUP BY
    V.PostId
), BadgeStats AS (
  SELECT
    B.UserId,
    COUNT(B.Id) AS BadgeCount
  FROM Badges AS B
  GROUP BY
    B.UserId
)
SELECT
  PS.PostId,
  PS.PostTypeId,
  PS.CreationDate,
  PS.Score,
  PS.ViewCount,
  PS.AnswerCount,
  PS.CommentCount,
  PS.FavoriteCount,
  PS.OwnerReputation,
  PS.OwnerDisplayName,
  COALESCE(VS.VoteCount, 0) AS VoteCount,
  COALESCE(VS.UpVotes, 0) AS UpVotes,
  COALESCE(VS.DownVotes, 0) AS DownVotes,
  COALESCE(BS.BadgeCount, 0) AS OwnerBadgeCount
FROM PostStats AS PS
LEFT JOIN VoteStats AS VS
  ON PS.PostId = VS.PostId
LEFT JOIN BadgeStats AS BS
  ON PS.OwnerReputation = BS.UserId
ORDER BY
  PS.CreationDate DESC;
