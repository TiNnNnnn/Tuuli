SELECT i FROM quant_outer WHERE v < ANY (SELECT DISTINCT 10 / v FROM quant_error) ORDER BY i;
