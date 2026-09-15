SELECT a.i, b.j
FROM binding_input a JOIN binding_input b ON a.i = b.i
WHERE a.i + 1 > 2
ORDER BY a.i, b.j NULLS FIRST;
