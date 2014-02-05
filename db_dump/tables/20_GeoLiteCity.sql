CREATE TABLE IF NOT EXISTS GeoLiteCity
(
locId INT8 NOT NULL,
country CHAR(2),
region CHAR(2),
city VARCHAR(128),
postalCode VARCHAR(8),
latitude DECIMAL(5,4),
longitude DECIMAL(5,4),
metroCode INT8 DEFAULT NULL,
areaCode INT8 DEFAULT NULL,
UNIQUE (locId) ON CONFLICT IGNORE
);
CREATE INDEX IF NOT EXISTS idx_GeoRerions_locId ON GeoLiteCity (locId);
CREATE INDEX IF NOT EXISTS idx_GeoRerions_country ON GeoLiteCity (country);
CREATE INDEX IF NOT EXISTS idx_GeoRerions_region ON GeoLiteCity (region);
CREATE INDEX IF NOT EXISTS idx_GeoRerions_city ON GeoLiteCity (city);
