CREATE TABLE IF NOT EXISTS CronCampaign
(
id_cam INT8 NOT NULL,
Day SMALLINT,
Hour SMALLINT,
Min SMALLINT,
startStop SMALLINT
);
CREATE INDEX IF NOT EXISTS idx_CronCampaign_Day ON CronCampaign (Day);
CREATE INDEX IF NOT EXISTS idx_CronCampaign_Hour ON CronCampaign (Hour);
CREATE INDEX IF NOT EXISTS idx_CronCampaign_Hour_Min ON CronCampaign (Hour, Min);
CREATE INDEX IF NOT EXISTS idx_CronCampaign_startStop ON CronCampaign (startStop);

