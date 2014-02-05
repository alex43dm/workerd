CREATE TABLE IF NOT EXISTS Campaign
(
id INT8 NOT NULL,
guid VARCHAR(35),
title VARCHAR(35),
project VARCHAR(70),
social SMALLINT,
valid SMALLINT,
impressionsPerDayLimit SMALLINT,
showCoverage VARCHAR(35),
retargeting SMALLINT,
UNIQUE (id) ON CONFLICT IGNORE,
UNIQUE (guid) ON CONFLICT IGNORE
);

CREATE INDEX IF NOT EXISTS idx_Campaign_id ON Campaign (id);
CREATE INDEX IF NOT EXISTS idx_Campaign_guid ON Campaign (guid);
CREATE INDEX IF NOT EXISTS idx_Campaign_showCoverage ON Campaign (showCoverage);
CREATE INDEX IF NOT EXISTS idx_Campaign_impressionsPerDayLimit ON Campaign (impressionsPerDayLimit);
CREATE INDEX IF NOT EXISTS idx_Campaign_valid ON Campaign (valid);
CREATE INDEX IF NOT EXISTS idx_Campaign_retargeting ON Campaign (retargeting);
CREATE INDEX IF NOT EXISTS idx_Campaign_retargeting_valid ON Campaign (valid,retargeting);
