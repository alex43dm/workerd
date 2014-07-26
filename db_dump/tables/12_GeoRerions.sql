CREATE TABLE IF NOT EXISTS GeoRerions
(
id INTEGER PRIMARY KEY AUTOINCREMENT,
cid CHAR(2) NOT NULL,
rid CHAR(2) NOT NULL,
rname VARCHAR(35)
);
CREATE INDEX IF NOT EXISTS idx_GeoRerions_cid ON GeoRerions (cid);
CREATE INDEX IF NOT EXISTS idx_GeoRerions_rid ON GeoRerions (rid);
CREATE INDEX IF NOT EXISTS idx_GeoRerions_rname ON GeoRerions (rname);
