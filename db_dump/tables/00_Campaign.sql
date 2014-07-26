CREATE TABLE IF NOT EXISTS Campaign
(
id INT8 NOT NULL,
guid VARCHAR(35),
title VARCHAR(35),
project VARCHAR(70),
social SMALLINT,
valid SMALLINT,
impressionsPerDayLimit SMALLINT,
showCoverage SMALLINT,
retargeting SMALLINT,
offer_by_campaign_unique SMALLINT,
UNIQUE (id) ON CONFLICT IGNORE,
UNIQUE (guid) ON CONFLICT IGNORE
);

CREATE INDEX IF NOT EXISTS idx_Campaign_id ON Campaign (id);
CREATE INDEX IF NOT EXISTS idx_Campaign_guid ON Campaign (guid);
CREATE INDEX IF NOT EXISTS idx_Campaign_retargeting_valid ON Campaign (id,valid,retargeting) WHERE valid=1 AND retargeting=0;
CREATE INDEX IF NOT EXISTS idx_Campaign_retargeting_valid_inv ON Campaign (id,valid,retargeting) WHERE valid=1 AND retargeting=1;
