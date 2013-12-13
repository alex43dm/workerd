CREATE VIEW CampaignNow AS
SELECT id_cam, startStop AS id FROM CronCampaign AS alw
WHERE (Day = cast(strftime('%w','now') AS INT) OR Day IS NULL)
    AND (Hour < cast(strftime('%H','now') AS INT)
    OR ( Hour = cast(strftime('%H','now') AS INT)
        AND Min <= cast(strftime('%M','now') AS INT) ))

