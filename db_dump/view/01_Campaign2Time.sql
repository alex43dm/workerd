CREATE VIEW Campaign2Time AS 
SELECT id_cam AS id FROM CronCampaign AS alw 
WHERE (Day = cast(strftime('%w','now') AS INT) OR Day IS NULL) 
AND (Hour < cast(strftime('%H','now') AS INT) 
OR ( Hour = cast(strftime('%H','now') AS INT) 
AND Min <= cast(strftime('%M','now') AS INT) )) 
AND startStop=1 
EXCEPT 
SELECT cron.id_cam AS id FROM CronCampaign AS cron  
WHERE (Day = cast(strftime('%w','now') AS INT) OR Day IS NULL) 
AND ( Hour < cast(strftime('%H','now') AS INT) 
OR ( Hour = cast(strftime('%H','now') AS INT) 
AND Min <= cast(strftime('%M','now') AS INT) )) 
AND cron.startStop=0;
