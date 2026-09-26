CREATE TABLE column_values(i int, j int, b bool, v varchar(8));
INSERT INTO column_values VALUES (1,10,true,'one'), (2,NULL,NULL,NULL), (3,30,false,'three'), (3,30,false,'three');
ANALYZE column_values;
