CREATE VIEW Campaign2Time AS
SELECT id FROM Campaign2Now AS alw WHERE startStop=1
EXCEPT
SELECT id FROM Campaign2Now AS alw WHERE startStop=0
;
