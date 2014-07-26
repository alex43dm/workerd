CREATE VIEW IF NOT EXISTS Campaign2Doms AS
SELECT c2d.id_cam AS id,dom.name,c2d.allowed
FROM Campaign2Domains AS c2d
LEFT JOIN Domains AS dom ON c2d.id_dom = dom.id;
