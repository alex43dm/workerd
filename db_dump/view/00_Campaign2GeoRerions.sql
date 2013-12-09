CREATE VIEW IF NOT EXISTS Campaign2GeoRerions AS
SELECT cam.id,reg.cid,reg.rname FROM Campaign AS cam
INNER JOIN geoTargeting AS geo ON cam.id = geo.id_cam
INNER JOIN GeoRerions AS reg ON geo.id_geo = reg.id;
