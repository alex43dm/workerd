CREATE TABLE CampaignNow(
id INT8 NOT NULL,
startStop SMALLINT,
FOREIGN KEY(id) REFERENCES Campaign(id)
);

CREATE INDEX IF NOT EXISTS idx_CampaignNow_startStop ON CampaignNow (startStop);

