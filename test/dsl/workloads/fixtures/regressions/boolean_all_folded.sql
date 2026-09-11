-- Pending translator/quantified-value regression; see README.md in this folder.
SELECT o.case_id,
       TRUE = ALL (SELECT i.y = 1 FROM dsl_notin_inner i WHERE i.set_id = o.set_id),
       FALSE = ALL (SELECT i.y = 1 FROM dsl_notin_inner i WHERE i.set_id = o.set_id),
       TRUE <> ALL (SELECT i.y = 1 FROM dsl_notin_inner i WHERE i.set_id = o.set_id),
       FALSE <> ALL (SELECT i.y = 1 FROM dsl_notin_inner i WHERE i.set_id = o.set_id)
FROM dsl_notin_outer o
ORDER BY o.case_id;
