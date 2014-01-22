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
ofrs.retargeting,
ofrs.uniqueHits,
ofrs.height,
ofrs.width,
ca.social
FROM Offer AS ofrs
INNER JOIN Campaign AS ca ON ofrs.campaignId=ca.id
WHERE ofrs.id IN(%s)
	AND ofrs.valid=1 
ORDER BY ofrs.rating DESC
LIMIT 200;
