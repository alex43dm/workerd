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
ca.social,
ca.guid,
ca.offer_by_campaign_unique
FROM Offer AS ofrs INDEXED BY idx_Offer_id
INNER JOIN Campaign AS ca INDEXED BY idx_Campaign_id ON ca.valid=1 AND ca.retargeting=1 AND ofrs.campaignId=ca.id
LEFT JOIN Retargeting AS ret INDEXED BY idx_Retargeting_offerId_uniqueHits ON ret.id=%llu AND ret.uniqueHits <= 0 AND ofrs.id = ret.offerId
LEFT JOIN Retargeting AS ret1 INDEXED BY idx_Retargeting_offerId ON ofrs.id = ret1.offerId
WHERE ofrs.id IN(%s)
	AND ofrs.valid=1
	AND ret.id IS NULL
ORDER BY ret1.viewTime ASC
LIMIT 200;
