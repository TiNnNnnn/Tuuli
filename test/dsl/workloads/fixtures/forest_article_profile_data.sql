-- Synthetic article/tag domain: selected/unselected status and tag predicates,
-- multiple references per article, and articles with no tag. No schema changes.
INSERT INTO article (article_id, article_title, article_status)
SELECT i, 'article_' || i, i % 2 FROM generate_series(1, 1024) AS g(i);
INSERT INTO article_tag_ref (article_id, tag_id)
SELECT i, tag FROM generate_series(1, 1024) AS g(i)
CROSS JOIN generate_series(1, 4) AS t(tag) WHERE i % 5 <> 0;
ANALYZE;
