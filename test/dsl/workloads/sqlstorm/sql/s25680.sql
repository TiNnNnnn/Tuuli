WITH TagCounts AS (
  SELECT
    UNNEST(STRING_TO_ARRAY(SUBSTRING(Tags FROM 2 FOR LENGTH(Tags) - 2), '><')) AS Tag,
    COUNT(*) AS PostCount
  FROM Posts
  WHERE
    PostTypeId = 1
  GROUP BY
    Tag
), TopTags AS (
  SELECT
    Tag,
    PostCount,
    ROW_NUMBER() OVER (ORDER BY PostCount DESC) AS TagRank
  FROM TagCounts
  WHERE
    PostCount > 5
), MostRecentPosts AS (
  SELECT
    p.Id,
    p.Title,
    p.CreationDate,
    p.Tags,
    u.DisplayName AS OwnerName,
    pt.Name AS PostType
  FROM Posts AS p
  JOIN Users AS u
    ON p.OwnerUserId = u.Id
  JOIN PostTypes AS pt
    ON p.PostTypeId = pt.Id
  WHERE
    p.CreationDate >= (
      CAST('2024-10-01 12:34:56' AS TIMESTAMP) - INTERVAL '1 MONTH'
    )
)
SELECT
  t.Tag,
  t.PostCount AS TagPopularity,
  p.Title AS RecentPostTitle,
  p.OwnerName AS RecentPostOwner,
  p.CreationDate AS RecentPostDate,
  p.PostType AS RecentPostType
FROM TopTags AS t
LEFT JOIN MostRecentPosts AS p
  ON t.Tag LIKE '%' || p.Tags || '%'
ORDER BY
  t.PostCount DESC,
  p.CreationDate DESC;
