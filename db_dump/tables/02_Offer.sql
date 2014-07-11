CREATE TABLE IF NOT EXISTS Offer
(
id INT8 NOT NULL,
guid VARCHAR(35),
categoryId INT8,
campaignId INT8,
accountId VARCHAR(35),
rating DECIMAL(5,2),
retargeting SMALLINT,
image VARCHAR(2048),
height SMALLINT,
width SMALLINT,
isOnClick SMALLINT,
cost DECIMAL(4,2),
uniqueHits SMALLINT,
swf VARCHAR(2048),
type SMALLINT,
description VARCHAR(70),
price VARCHAR(35),
url VARCHAR(2048),
title  VARCHAR(70),
valid SMALLINT DEFAULT 1,
UNIQUE (id) ON CONFLICT IGNORE,
UNIQUE (guid) ON CONFLICT IGNORE,
FOREIGN KEY(campaignId) REFERENCES Campaign(id),
FOREIGN KEY(categoryId) REFERENCES Category(id)
);

CREATE INDEX IF NOT EXISTS idx_Offer_id ON Offer (id);
CREATE INDEX IF NOT EXISTS idx_Offer_valid ON Offer (valid) WHERE valid=1;
CREATE INDEX IF NOT EXISTS idx_Offer_id_valid_camp ON Offer (id,valid,campaignId) WHERE valid=1;
