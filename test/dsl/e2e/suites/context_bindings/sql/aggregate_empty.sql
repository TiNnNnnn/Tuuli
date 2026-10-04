SELECT count(*), bool_or((i > 1) IS NOT TRUE)
FROM context_input WHERE j < 0;
