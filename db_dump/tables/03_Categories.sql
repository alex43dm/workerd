CREATE TABLE IF NOT EXISTS Categories
(
id INT8 NOT NULL,
guid VARCHAR(38),
title VARCHAR(1024),
UNIQUE (id) ON CONFLICT IGNORE,
UNIQUE (guid) ON CONFLICT IGNORE
);
CREATE INDEX IF NOT EXISTS idx_Categories_id ON Categories (id);
CREATE INDEX IF NOT EXISTS idx_Categories_guid ON Categories (guid);
INSERT INTO Categories(id,guid,title) VALUES(1,'','all');
