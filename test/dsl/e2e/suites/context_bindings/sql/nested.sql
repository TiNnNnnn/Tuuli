SELECT i, j FROM context_input
WHERE ((i > 1) IS NOT TRUE) OR j > 1
ORDER BY i NULLS FIRST, j;
