CREATE VIEW Campaign2Time AS
SELECT id FROM CampaignNow WHERE startStop=1
EXCEPT
SELECT id FROM CampaignNow WHERE startStop=0
;
