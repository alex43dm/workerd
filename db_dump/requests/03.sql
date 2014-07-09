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
CASE WHEN ses.uniqueHits IS NULL
THEN ofrs.rating
ELSE ofrs.rating - (ofrs.uniqueHits-ses.uniqueHits)*ofrs.rating/ofrs.uniqueHits
END AS rating,
ofrs.retargeting,
ofrs.uniqueHits,
ofrs.height,
ofrs.width,
ca.social,
ca.guid,
ca.offer_by_campaign_unique
FROM Offer AS ofrs INDEXED BY idx_Offer_id
INNER JOIN Campaign AS ca INDEXED BY idx_Campaign_id ON ca.valid=1 AND ca.retargeting=1 AND ofrs.campaignId=ca.id
LEFT JOIN Session AS ses INDEXED BY idx_Session_id_offerId ON ofrs.id=ses.offerId AND ses.id=%llu AND ses.uniqueHits <= 0 AND ses.tail=0
--LEFT JOIN Retargeting AS ret1 INDEXED BY idx_Retargeting_offerId ON ofrs.id = ret1.offerId
WHERE ofrs.id IN(%s)
	AND ofrs.valid=1
	AND ses.offerId IS NULL
--ORDER BY ret1.viewTime ASC
--LIMIT 100
;
