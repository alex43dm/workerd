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
LEFT JOIN Retargeting AS ret ON ret.id=%lli AND ret.uniqueHits <= 0 AND ofrs.id = ret.offerId
WHERE ofrs.id IN(%s)
	AND ofrs.valid=1
	AND ret.id IS NULL
ORDER BY ofrs.rating DESC
LIMIT 200;
