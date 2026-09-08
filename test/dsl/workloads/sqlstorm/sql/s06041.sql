WITH PostActivity AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    u.DisplayName AS OwnerDisplayName,
    p.CreationDate,
    p.LastActivityDate,
    p.ViewCount,
    COUNT(c.Id) AS CommentCount,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes,
    COUNT(DISTINCT ph.Id) AS HistoryCount
  FROM Posts AS p
  LEFT JOIN Users AS u
    ON p.OwnerUserId = u.Id
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  LEFT JOIN PostHistory AS ph
    ON p.Id = ph.PostId
  WHERE
    p.CreationDate >= CAST('2024-10-01' AS DATE) - INTERVAL '1 YEAR'
    AND p.PostTypeId = 1
  GROUP BY
    p.Id,
    p.Title,
    u.DisplayName,
    p.CreationDate,
    p.LastActivityDate,
    p.ViewCount
), PostRanked AS (
  SELECT
    pa.*,
    RANK() OVER (ORDER BY pa.ViewCount DESC, pa.UpVotes DESC, pa.LastActivityDate DESC) AS Rank
  FROM PostActivity AS pa
)
SELECT
  Rank,
  Title,
  OwnerDisplayName,
  CreationDate,
  LastActivityDate,
  ViewCount,
  CommentCount,
  UpVotes,
  DownVotes,
  HistoryCount
FROM PostRanked
WHERE
  Rank <= 10;
