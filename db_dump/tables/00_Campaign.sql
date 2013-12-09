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
UNIQUE (id) ON CONFLICT REPLACE,
UNIQUE (guid) ON CONFLICT REPLACE
);

CREATE INDEX IF NOT EXISTS idx_Campaign_id ON Campaign (id);
CREATE INDEX IF NOT EXISTS idx_Campaign_guid ON Campaign (guid);
CREATE INDEX IF NOT EXISTS idx_Campaign_showCoverage ON Campaign (showCoverage);
CREATE INDEX IF NOT EXISTS idx_Campaign_impressionsPerDayLimit ON Campaign (impressionsPerDayLimit);
