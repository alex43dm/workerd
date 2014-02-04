CREATE TABLE CampaignNow(
id INT8 NOT NULL,
FOREIGN KEY(id) REFERENCES Campaign(id)
);

CREATE INDEX IF NOT EXISTS idx_CampaignNow_id ON CampaignNow (id);

