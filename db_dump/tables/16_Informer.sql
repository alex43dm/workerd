CREATE TABLE IF NOT EXISTS Informer
(
id INT8 NOT NULL,
guid VARCHAR(35),
title VARCHAR(35),
teasersCss VARCHAR(70),
bannersCss VARCHAR(70),
domainId INTEGER,
accountId INTEGER,
blocked SMALLINT,
nonrelevant VARCHAR(2048),
user_code VARCHAR(2048),
capacity SMALLINT,
valid SMALLINT,
height SMALLINT,
width SMALLINT,
height_banner SMALLINT,
width_banner SMALLINT,
rtgPercentage SMALLINT,
UNIQUE (id) ON CONFLICT IGNORE,
UNIQUE (guid) ON CONFLICT IGNORE,
FOREIGN KEY(domainId) REFERENCES Domains(id),
FOREIGN KEY(accountId) REFERENCES Accounts(id)
);

CREATE INDEX IF NOT EXISTS idx_Informer_id ON Informer (id);
CREATE INDEX IF NOT EXISTS idx_Informer_guid ON Informer (guid);
CREATE INDEX IF NOT EXISTS idx_Informer_domainId ON Informer (domainId);
CREATE INDEX IF NOT EXISTS idx_Informer_accountId ON Accounts (id);

