WITH UserVoteStats AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    SUM(CASE WHEN V.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN V.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes,
    COUNT(DISTINCT P.Id) AS PostCount,
    COUNT(DISTINCT B.Id) AS BadgeCount
  FROM Users AS U
  LEFT JOIN Votes AS V
    ON U.Id = V.UserId
  LEFT JOIN Posts AS P
    ON V.PostId = P.Id
  LEFT JOIN Badges AS B
    ON U.Id = B.UserId
  GROUP BY
    U.Id,
    U.DisplayName
), TopUsers AS (
  SELECT
    UserId,
    DisplayName,
    UpVotes - DownVotes AS NetVotes,
    RANK() OVER (ORDER BY UpVotes DESC) AS UpVoteRank
  FROM UserVoteStats
  WHERE
    PostCount > 5
  ORDER BY
    NetVotes DESC
), PostDetails AS (
  SELECT
    P.Id AS PostId,
    P.Title,
    P.CreationDate,
    COALESCE(PT.Name, 'General') AS PostType,
    COALESCE(SUM(CASE WHEN NOT C.Id IS NULL THEN 1 ELSE 0 END), 0) AS CommentCount
  FROM Posts AS P
  LEFT JOIN PostTypes AS PT
    ON P.PostTypeId = PT.Id
  LEFT JOIN Comments AS C
    ON P.Id = C.PostId
  GROUP BY
    P.Id,
    P.Title,
    P.CreationDate,
    PT.Name
), TopPosts AS (
  SELECT
    pd.PostId,
    pd.Title,
    pd.CreationDate,
    pd.PostType,
    pd.CommentCount,
    RANK() OVER (ORDER BY pd.CommentCount DESC) AS CommentRank
  FROM PostDetails AS pd
)
SELECT
  tu.UserId,
  tu.DisplayName,
  tu.NetVotes,
  tp.PostId,
  tp.Title,
  tp.CreationDate,
  tp.PostType,
  tp.CommentCount
FROM TopUsers AS tu
FULL OUTER JOIN TopPosts AS tp
  ON tu.UserId = (
    SELECT
      OwnerUserId
    FROM Posts
    WHERE
      Id = tp.PostId
  )
WHERE
  tu.UpVoteRank <= 10 OR tp.CommentRank <= 10
ORDER BY
  tu.NetVotes DESC,
  tp.CommentCount DESC;
