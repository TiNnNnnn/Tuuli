WITH UserVoteSummary AS (
  SELECT
    U.Id AS UserId,
    U.DisplayName,
    COUNT(CASE WHEN V.VoteTypeId = 2 THEN 1 END) AS UpVotes,
    COUNT(CASE WHEN V.VoteTypeId = 3 THEN 1 END) AS DownVotes,
    SUM(CASE WHEN P.PostTypeId = 1 THEN P.Score ELSE 0 END) AS TotalQuestionScore
  FROM Users AS U
  LEFT JOIN Votes AS V
    ON U.Id = V.UserId
  LEFT JOIN Posts AS P
    ON V.PostId = P.Id
  GROUP BY
    U.Id,
    U.DisplayName
), ClosedPostDetails AS (
  SELECT
    P.Id AS PostId,
    P.Title,
    PH.CreationDate AS ClosedDate,
    (
      SELECT
        COUNT(*)
      FROM Comments AS C
      WHERE
        C.PostId = P.Id
    ) AS TotalComments,
    (
      SELECT
        COUNT(*)
      FROM Votes AS V
      WHERE
        V.PostId = P.Id AND V.VoteTypeId IN (6, 12)
    ) AS CloseVotes
  FROM Posts AS P
  JOIN PostHistory AS PH
    ON P.Id = PH.PostId
  WHERE
    PH.PostHistoryTypeId = 10
), RankedPosts AS (
  SELECT
    PD.PostId,
    PD.Title,
    PD.ClosedDate,
    PD.TotalComments,
    PD.CloseVotes,
    RANK() OVER (ORDER BY PD.CloseVotes DESC, PD.TotalComments DESC) AS PostRank
  FROM ClosedPostDetails AS PD
)
SELECT
  U.DisplayName,
  U.UpVotes,
  U.DownVotes,
  U.TotalQuestionScore,
  RP.Title,
  RP.ClosedDate,
  RP.TotalComments,
  RP.CloseVotes,
  RP.PostRank
FROM UserVoteSummary AS U
FULL OUTER JOIN RankedPosts AS RP
  ON U.UserId = RP.PostId
WHERE
  NOT U.UserId IS NULL
  AND (
    UPPER(U.DisplayName) LIKE '%DAVID%' OR RP.TotalComments > 10
  )
ORDER BY
  COALESCE(RP.CloseVotes, 0) DESC,
  U.UpVotes DESC;
