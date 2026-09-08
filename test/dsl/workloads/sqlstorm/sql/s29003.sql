WITH TagStats AS (
  SELECT
    t.TagName,
    COUNT(DISTINCT p.Id) AS PostCount,
    SUM(p.ViewCount) AS TotalViews,
    SUM(CASE WHEN NOT p.AcceptedAnswerId IS NULL THEN 1 ELSE 0 END) AS AcceptedAnswers,
    AVG(p.Score) AS AverageScore
  FROM Tags AS t
  LEFT JOIN Posts AS p
    ON p.Tags LIKE CONCAT('%', t.TagName, '%')
  GROUP BY
    t.TagName
), UserReputation AS (
  SELECT
    u.Id AS UserId,
    u.DisplayName,
    u.Reputation,
    COUNT(DISTINCT p.Id) AS TotalPosts,
    SUM(v.BountyAmount) AS TotalBounties
  FROM Users AS u
  LEFT JOIN Posts AS p
    ON u.Id = p.OwnerUserId
  LEFT JOIN Votes AS v
    ON v.UserId = u.Id
    AND v.PostId IN (
      SELECT
        Id
      FROM Posts
      WHERE
        NOT OwnerUserId IS NULL
    )
  GROUP BY
    u.Id,
    u.DisplayName,
    u.Reputation
), PostActivity AS (
  SELECT
    p.Id AS PostId,
    p.Title,
    p.CreationDate,
    p.LastEditDate,
    COUNT(c.Id) AS CommentCount,
    COUNT(DISTINCT v.UserId) AS UniqueVoters,
    SUM(CASE WHEN v.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN v.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes
  FROM Posts AS p
  LEFT JOIN Comments AS c
    ON p.Id = c.PostId
  LEFT JOIN Votes AS v
    ON p.Id = v.PostId
  GROUP BY
    p.Id,
    p.Title,
    p.CreationDate,
    p.LastEditDate
)
SELECT
  ts.TagName,
  ts.PostCount,
  ts.TotalViews,
  ts.AcceptedAnswers,
  ts.AverageScore,
  ur.DisplayName AS TopUserDisplayName,
  ur.Reputation AS TopUserReputation,
  ur.TotalPosts AS TopUserPostCount,
  ur.TotalBounties AS TopUserBountyCount,
  pa.PostId,
  pa.Title AS PostTitle,
  pa.CreationDate AS PostCreationDate,
  pa.LastEditDate AS PostLastEditDate,
  pa.CommentCount AS PostCommentCount,
  pa.UniqueVoters AS PostUniqueVoterCount,
  pa.UpVotes AS PostUpVotes,
  pa.DownVotes AS PostDownVotes
FROM TagStats AS ts
JOIN UserReputation AS ur
  ON ur.TotalPosts = (
    SELECT
      MAX(TotalPosts)
    FROM UserReputation
  )
JOIN PostActivity AS pa
  ON pa.PostId = (
    SELECT
      p.Id
    FROM Posts AS p
    WHERE
      p.Tags LIKE CONCAT('%', ts.TagName, '%')
    ORDER BY
      p.ViewCount DESC
    LIMIT 1
  )
ORDER BY
  ts.TotalViews DESC,
  ts.AcceptedAnswers DESC;
