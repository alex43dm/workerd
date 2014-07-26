CREATE TABLE IF NOT EXISTS geoTargeting
(
id_cam INT8 NOT NULL,
id_geo INT8 NOT NULL,
FOREIGN KEY(id_cam) REFERENCES Campaign(id),
FOREIGN KEY(id_geo) REFERENCES GeoRerions(id),
UNIQUE (id_cam,id_geo) ON CONFLICT IGNORE
);
CREATE INDEX IF NOT EXISTS idx_geoTargeting_id_geo_id_cam ON geoTargeting (id_geo,id_cam);
