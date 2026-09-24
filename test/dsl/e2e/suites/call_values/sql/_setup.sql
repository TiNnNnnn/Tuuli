CREATE EXTENSION IF NOT EXISTS pg_orca;
CREATE TABLE call_input(i int, j int, label text);
INSERT INTO call_input VALUES
    (NULL,2,'null'),(0,1,'zero'),(1,1,'one'),(2,NULL,'two'),(2,NULL,'two'),(3,2,'three');
CREATE FUNCTION call_volatile(int) RETURNS int VOLATILE LANGUAGE plpgsql AS
$$ BEGIN RETURN $1; END $$;
ANALYZE call_input;
