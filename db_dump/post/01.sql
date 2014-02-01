DELETE FROM CampaignNow
;

INSERT INTO CampaignNow(id, startStop)
SELECT id_cam AS id, startStop FROM CronCampaign
WHERE (Day = cast(strftime('%w','now') AS INT) OR Day IS NULL)
AND (Hour < cast(strftime('%H','now') AS INT)
OR ( Hour = cast(strftime('%H','now') AS INT)
AND Min <= cast(strftime('%M','now') AS INT) ))
;

DELETE FROM CampaignNow
WHERE startStop=0 AND id IN (
    SELECT c.id FROM CampaignNow AS c
    INNER JOIN (
        SELECT id FROM CampaignNow GROUP BY id HAVING count(id)>1
    ) AS cc ON c.id=cc.id WHERE c.startStop=0
)
;
