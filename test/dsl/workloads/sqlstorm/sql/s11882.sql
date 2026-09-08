WITH PostStatistics AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.AnswerCount,
    p.CommentCount,
    p.OwnerUserId,
    u.Reputation AS OwnerReputation,
    COUNT(v.Id) AS VoteCount
  FROM Posts AS p
  LEFT JOIN Users AS u
    ON p.OwnerUserId = u.Id
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.Score,
    p.ViewCount,
    p.AnswerCount,
    p.CommentCount,
    p.OwnerUserId,
    u.Reputation
), UserStatistics AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    u.Reputation,
    COUNT(DISTINCT p.Id) AS PostsCount,
    COUNT(DISTINCT b.Id) AS BadgesCount
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Badges AS b
    ON u.Id = b.UserId
  GROUP BY
    u.Id,
    u.DisplayName,
    u.Reputation
), PostHistoryCount AS (
  SELECT
    p.Id AS PostId,
    COUNT(ph.Id) AS HistoryCount
  FROM Posts AS p
  LEFT JOIN PostHistory AS ph
    ON p.Id = ph.PostId
  GROUP BY
    p.Id
)
SELECT
  ps.PostId,
  ps.Title,
  ps.CreationDate,
  ps.Score,
  ps.ViewCount,
  ps.AnswerCount,
  ps.CommentCount,
  ps.OwnerUserId,
  ps.OwnerReputation,
  ps.VoteCount,
  us.UserId,
  us.DisplayName AS OwnerDisplayName,
  us.Reputation AS OwnerReputation,
  us.PostsCount,
  us.BadgesCount,
  phc.HistoryCount
FROM PostStatistics AS ps
JOIN UserStatistics AS us
  ON ps.OwnerUserId = us.UserId
JOIN PostHistoryCount AS phc
  ON ps.PostId = phc.PostId
ORDER BY
  ps.ViewCount DESC,
  ps.Score DESC
LIMIT 100;
