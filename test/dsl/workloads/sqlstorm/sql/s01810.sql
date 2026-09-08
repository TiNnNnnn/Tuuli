WITH UserVoteStats AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    COUNT(V.Id) AS TotalVotes,
    SUM(CASE WHEN V.VoteTypeId = 2 THEN 1 ELSE 0 END) AS UpVotes,
    SUM(CASE WHEN V.VoteTypeId = 3 THEN 1 ELSE 0 END) AS DownVotes
  FROM Users AS U
  LEFT JOIN Votes AS V
    ON U.Id = V.UserId
  GROUP BY
    U.Id,
    U.DisplayName
), TopUsers AS (
  SELECT
    UserId,
    DisplayName,
    TotalVotes,
    UpVotes,
    DownVotes,
    RANK() OVER (ORDER BY TotalVotes DESC) AS UserRank
  FROM UserVoteStats
  WHERE
    TotalVotes > 0
), RecentPosts AS (
  SELECT
    P.Id AS PostId,
    P.Title,
    P.CreationDate,
    P.OwnerUserId,
    DENSE_RANK() OVER (PARTITION BY P.OwnerUserId ORDER BY P.CreationDate DESC) AS PostRank
  FROM Posts AS P
  WHERE
    P.CreationDate >= (
      CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '30 DAYS'
    )
)
SELECT
  TU.DisplayName,
  TU.TotalVotes,
  TU.UpVotes,
  TU.DownVotes,
  RP.Title,
  RP.CreationDate,
  COALESCE(
    (
      SELECT
        U.DisplayName
      FROM Users AS U
      WHERE
        U.Id = RP.OwnerUserId
    ),
    'Anonymous'
  ) AS PostOwner,
  (
    SELECT
      COUNT(C.Id)
    FROM Comments AS C
    WHERE
      C.PostId = RP.PostId
  ) AS CommentCount
FROM TopUsers AS TU
LEFT JOIN RecentPosts AS RP
  ON TU.UserId = RP.OwnerUserId
WHERE
  TU.UserRank <= 10
ORDER BY
  TU.TotalVotes DESC,
  RP.CreationDate DESC;
