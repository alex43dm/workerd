SELECT ofrs.id,
ofrs.guid,
ofrs.title,
ofrs.description,
ofrs.url,
ofrs.image,
ofrs.swf,
ofrs.campaignId,
ofrs.type,
CASE WHEN ses.uniqueHits IS NULL
THEN ofrs.rating
ELSE ofrs.rating - (ofrs.uniqueHits-ses.uniqueHits)*ofrs.rating/ofrs.uniqueHits
END AS rating,
ofrs.uniqueHits,
ofrs.height,
ofrs.width,
ofrs.isOnClick,
ca.social,
ca.guid AS campaign_guid,
ca.offer_by_campaign_unique
FROM OfferR AS ofrs INDEXED BY idx_OfferR_id
INNER JOIN Campaign AS ca INDEXED BY idx_Campaign_id ON ca.valid=1 AND ca.retargeting=1 AND ofrs.campaignId=ca.id
LEFT JOIN Session AS ses INDEXED BY idx_Session_id_offerId ON ofrs.id=ses.offerId
    AND ses.id=%llu AND ses.tail=0
WHERE ofrs.id IN(%s)
	AND ofrs.valid = 1
	AND (ses.uniqueHits IS NULL OR ses.uniqueHits > 0)
--ORDER BY ses.viewTime ASC
--LIMIT 100
;
