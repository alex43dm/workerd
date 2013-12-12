DELETE FROM CampaignNow;

INSERT INTO CampaignNow(id, startStop)
SELECT id_cam AS id, startStop FROM CronCampaign
WHERE (Day = cast(strftime('%w','now') AS INT) OR Day IS NULL)
AND (Hour < cast(strftime('%H','now') AS INT)
OR ( Hour = cast(strftime('%H','now') AS INT)
AND Min <= cast(strftime('%M','now') AS INT) ))
;
