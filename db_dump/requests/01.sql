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
THEN ifnull(iret.rating,ofrs.rating)
ELSE ifnull(iret.rating,ofrs.rating) - (ofrs.uniqueHits-ses.uniqueHits)*ifnull(iret.rating,ofrs.rating)/ofrs.uniqueHits
END AS rating,
ofrs.uniqueHits,
ofrs.height,
ofrs.width,
ofrs.isOnClick,
ca.social,
ca.guid AS campaign_guid,
ca.offer_by_campaign_unique
FROM Offer AS ofrs
INNER JOIN Campaign AS ca ON ofrs.campaignId=ca.id
INNER JOIN %s AS cn ON ca.id=cn.id
LEFT JOIN Session AS ses INDEXED BY idx_Session_id_offerId ON ofrs.id=ses.offerId AND ses.id=%llu AND ses.tail=0
LEFT JOIN Informer2OfferRating AS iret ON iret.id_inf=%lld AND ofrs.id=iret.id_ofr
WHERE ofrs.valid=1
    AND (ses.uniqueHits IS NULL OR ses.uniqueHits > 0)
;
