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
	ca.social,
	null,
	null
	FROM Offer AS ofrs
	INNER JOIN Campaign AS ca ON ofrs.campaignId=ca.id
	INNER JOIN Campaign2Time AS c2t ON ca.id=c2t.id
ORDER BY ofrs.rating DESC;
