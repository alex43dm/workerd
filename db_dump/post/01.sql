DELETE FROM CampaignNow
;

INSERT INTO CampaignNow(id)
SELECT co.id_cam AS id
FROM CronCampaign AS co
WHERE co.Day IS NULL AND co.Hour=0 AND co.Min=0 AND co.startStop=0
;

INSERT INTO CampaignNow(id)
SELECT co.id_cam AS id
FROM CronCampaign AS co
INNER JOIN(
    SELECT c.id_cam
    FROM CronCampaign AS c
    INNER JOIN(
        SELECT id_cam
        FROM CronCampaign
        GROUP BY id_cam
        HAVING count(0)>1) AS cc ON c.id_cam=cc.id_cam
    WHERE (c.Day = cast(strftime('%w','now','localtime') AS INT) OR c.Day IS NULL)
    AND (
            (c.Hour < cast(strftime('%H','now','localtime') AS INT) AND c.startStop=1)
            OR
            (
                (c.Hour = cast(strftime('%H','now','localtime') AS INT) AND c.startStop=1)
                AND
                (c.Min <= cast(strftime('%M','now','localtime') AS INT) AND c.startStop=1)
            )
        )
) AS ci ON co.id_cam=ci.id_cam
INNER JOIN(
    SELECT c1.id_cam
    FROM CronCampaign AS c1
    INNER JOIN(
        SELECT id_cam
        FROM CronCampaign
        GROUP BY id_cam
        HAVING count(0)>1) AS cc1 ON c1.id_cam=cc1.id_cam
    WHERE (c1.Day = cast(strftime('%w','now','localtime') AS INT) OR c1.Day IS NULL)
    AND (
            (c1.Hour > cast(strftime('%H','now','localtime') AS INT) AND c1.startStop=0)
            OR
            (
                (c1.Hour = cast(strftime('%H','now','localtime') AS INT) AND c1.startStop=0)
                AND
                (c1.Min <= cast(strftime('%M','now','localtime') AS INT) AND c1.startStop=0)
            )
        )
) AS cx ON co.id_cam=cx.id_cam
GROUP BY id
;

REINDEX idx_CampaignNow_id
;
