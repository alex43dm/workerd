CREATE TABLE IF NOT EXISTS regionTargeting
(
id_cam INT8 NOT NULL,
id_geo INT8 NOT NULL,
FOREIGN KEY(id_cam) REFERENCES Campaign(id),
FOREIGN KEY(id_geo) REFERENCES GeoRerions(id)
);
