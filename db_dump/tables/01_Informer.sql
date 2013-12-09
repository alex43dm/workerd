CREATE TABLE IF NOT EXISTS Informer
(
id INT8 NOT NULL,
guid VARCHAR(35),
title VARCHAR(35),
teasersCss VARCHAR(70),
bannersCss VARCHAR(70),
domain VARCHAR(1024),
user VARCHAR(70),
blocked SMALLINT,
nonrelevant VARCHAR(2048),
user_code VARCHAR(2048),
capacity SMALLINT,
valid SMALLINT,
height SMALLINT,
width SMALLINT,
height_banner SMALLINT,
width_banner SMALLINT,
UNIQUE (id) ON CONFLICT REPLACE,
UNIQUE (guid) ON CONFLICT REPLACE
);

CREATE INDEX IF NOT EXISTS idx_Informer_id ON Informer (id);
CREATE INDEX IF NOT EXISTS idx_Informer_guid ON Informer (guid);

