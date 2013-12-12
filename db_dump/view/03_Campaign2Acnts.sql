CREATE VIEW IF NOT EXISTS Campaign2Acnts AS
SELECT c2a.id_cam AS id,ac.name,c2a.allowed
FROM Campaign2Accounts AS c2a
LEFT JOIN Accounts AS ac ON c2a.id_acc = ac.id;
