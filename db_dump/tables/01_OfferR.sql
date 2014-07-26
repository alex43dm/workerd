CREATE TABLE IF NOT EXISTS OfferR
(
id INT8 NOT NULL,
guid VARCHAR(35),
categoryId INT8,
campaignId INT8,
rating DECIMAL(5,2),
image VARCHAR(2048),
isOnClick SMALLINT,
height SMALLINT,
width SMALLINT,
uniqueHits SMALLINT,
swf VARCHAR(2048),
type SMALLINT,
description VARCHAR(70),
url VARCHAR(2048),
title  VARCHAR(70),
valid SMALLINT DEFAULT 1,
UNIQUE (id) ON CONFLICT IGNORE,
UNIQUE (guid) ON CONFLICT IGNORE,
FOREIGN KEY(campaignId) REFERENCES Campaign(id),
FOREIGN KEY(categoryId) REFERENCES Category(id)
);

CREATE INDEX IF NOT EXISTS idx_OfferR_id ON OfferR (id);
CREATE INDEX IF NOT EXISTS idx_OfferR_valid ON OfferR (valid) WHERE valid=1;
CREATE INDEX IF NOT EXISTS idx_OfferR_id_valid_camp ON OfferR (id,valid,campaignId) WHERE valid=1;
