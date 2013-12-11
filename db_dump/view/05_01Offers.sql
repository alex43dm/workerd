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
	c2r.rname AS rname,
	c2r.cid AS cid,
	c2d.name AS dname
	FROM Offer AS ofrs
	INNER JOIN Campaign AS ca ON ofrs.campaignId=ca.id
	INNER JOIN Campaign2Time AS cn ON cn.id=ca.id
	LEFT JOIN Campaign2GeoRerions AS c2r ON cn.id=c2r.id
	LEFT JOIN Campaign2Doms AS c2d ON cn.id=c2d.id AND allowed = 1;
