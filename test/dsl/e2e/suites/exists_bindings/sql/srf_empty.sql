SELECT i FROM exists_outer WHERE EXISTS (SELECT generate_series(1,0) FROM exists_inner WHERE j > 0) ORDER BY i NULLS FIRST;
