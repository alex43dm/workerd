CREATE TABLE IF NOT EXISTS GeoLiteCity
(
locId INT8 NOT NULL,
country CHAR(2),
region CHAR(2),
city VARCHAR(512),
postalCode VARCHAR(8),
latitude DECIMAL(5,4),
longitude DECIMAL(5,4),
metroCode INT8 DEFAULT NULL,
areaCode INT8 DEFAULT NULL,
UNIQUE (locId) ON CONFLICT IGNORE,
UNIQUE (country,city) ON CONFLICT IGNORE
);
CREATE INDEX IF NOT EXISTS idx_GeoRerions_country_city ON GeoLiteCity (country,city);
