CREATE VIEW IF NOT EXISTS getOffers01 AS
SELECT ofrs.id,
	ofrs.guid,
	ofrs.title,
	ofrs.price,
	ofrs.description,
	ofrs.url,
	ofrs.image,
	ofrs.swf,
	ofrs.campaignId,
	ofrs.isOnClick,
	ofrs.type,
	ofrs.rating,
	ofrs.uniqueHits,
	ofrs.height,
	ofrs.width,
	ca.social
	FROM Offer AS ofrs
	INNER JOIN Campaign AS ca ON ofrs.campaignId=ca.id
	INNER JOIN (
        SELECT id FROM(
            SELECT cn.id FROM Campaign2Time AS cn
            EXCEPT
            SELECT c2d.id FROM Campaign2Doms AS c2d WHERE c2d.name='google.com'
            ) GROUP BY id ) AS c ON ca.id=c.id;
