CREATE VIEW IF NOT EXISTS Campaign2Infs AS
SELECT c2i.id_cam AS id,inf.guid,c2i.allowed
FROM Campaign2Informer AS c2i
LEFT JOIN Informer AS inf ON c2i.id_inf = inf.id;
