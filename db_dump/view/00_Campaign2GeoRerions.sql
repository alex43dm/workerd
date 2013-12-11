CREATE VIEW IF NOT EXISTS Campaign2GeoRerions AS
SELECT geo.id_cam AS id,reg.cid,reg.rname
FROM geoTargeting AS geo
INNER JOIN GeoRerions AS reg ON geo.id_geo = reg.id;
