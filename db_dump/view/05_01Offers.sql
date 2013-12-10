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
	c2d.name,
	c2i.guid
	FROM Offer AS ofrs
	INNER JOIN Campaign AS ca ON ofrs.campaignId=ca.id
	INNER JOIN Campaign2Time AS c2t ON ca.id=c2t.id 
	LEFT JOIN Campaign2Doms AS c2d ON c2d.allowed=1 
		AND ca.id=c2d.id 
	LEFT JOIN Campaign2Acnts AS c2a ON c2a.allowed=1 
		AND ca.id=c2a.id 
	LEFT JOIN Campaign2Infs AS c2i ON c2i.allowed=1 
		AND ca.id=c2a.id 
ORDER BY ofrs.rating DESC;
