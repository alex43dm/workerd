CREATE VIEW IF NOT EXISTS Campaign2Acnts AS
SELECT cam.id,ac.name,c2a.allowed,cam.showCoverage 
FROM Campaign AS cam
LEFT JOIN Campaign2Accounts AS c2a ON cam.id = c2a.id_cam
LEFT JOIN Accounts AS ac ON c2a.id_acc = ac.id;
