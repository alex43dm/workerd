CREATE VIEW IF NOT EXISTS Campaign2Infs AS
SELECT cam.id,inf.guid,c2i.allowed,cam.showCoverage 
FROM Campaign AS cam
LEFT JOIN Campaign2Informer AS c2i ON cam.id = c2i.id_cam
LEFT JOIN Informer AS inf ON c2i.id_inf = inf.id;
