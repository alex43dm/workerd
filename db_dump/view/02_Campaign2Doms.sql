CREATE VIEW IF NOT EXISTS Campaign2Doms AS
SELECT cam.id,dom.name,c2d.allowed,cam.showCoverage FROM Campaign AS cam
LEFT JOIN Campaign2Domains AS c2d ON cam.id = c2d.id_cam
LEFT JOIN Domains AS dom ON c2d.id_dom = dom.id;
