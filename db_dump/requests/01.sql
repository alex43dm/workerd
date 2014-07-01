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
CASE WHEN iret.rating IS NOT NULL
THEN iret.rating
ELSE ofrs.rating
END AS rating,
ofrs.retargeting,
ofrs.uniqueHits,
ofrs.height,
ofrs.width,
ca.social,
ca.guid,
ca.offer_by_campaign_unique
FROM Offer AS ofrs
INNER JOIN Campaign AS ca ON ca.valid=1 AND ca.retargeting=0 AND ofrs.campaignId=ca.id %s
INNER JOIN CampaignNow AS cn ON ca.id=cn.id
		%s
INNER JOIN (
        SELECT c2c.id_cam AS id
        FROM Campaign2Categories AS c2c
        INNER JOIN Categories2Domain AS ct2d ON c2c.id_cat=ct2d.id_cat AND ct2d.id_dom=%lld
        UNION ALL
        SELECT c2da.id_cam AS id
        FROM Campaign2Domains AS c2da
        WHERE (c2da.id_dom=%lld OR c2da.id_dom=1) AND c2da.allowed=1
		EXCEPT
        SELECT c2dd.id_cam AS id
        FROM Campaign2Domains AS c2dd
        LEFT JOIN Campaign2Domains AS c2dde ON c2dd.id_cam=c2dde.id_cam AND c2dde.id_dom=%lld AND c2dde.allowed=1
        WHERE c2dde.id_cam IS NULL AND ((c2dd.id_dom=%lld OR c2dd.id_dom=1) AND c2dd.allowed=0)
        UNION ALL
        SELECT c2aa.id_cam AS id
        FROM Campaign2Accounts AS c2aa
        WHERE (c2aa.id_acc=%lld OR c2aa.id_acc=1) AND c2aa.allowed=1
		EXCEPT
        SELECT c2ad.id_cam AS id
        FROM Campaign2Accounts AS c2ad
        LEFT JOIN Campaign2Accounts AS c2ade ON c2ad.id_cam=c2ade.id_cam AND c2ade.id_acc=%lld AND c2ade.allowed=1
        WHERE c2ade.id_cam IS NULL AND ((c2ad.id_acc=%lld OR c2ad.id_acc=1) AND c2ad.allowed=0)
        UNION ALL
        SELECT c2ia.id_cam AS id
        FROM Campaign2Informer AS c2ia
        WHERE (c2ia.id_inf=%lld OR c2ia.id_inf=1) AND c2ia.allowed=1
        EXCEPT
        SELECT c2id.id_cam AS id
        FROM Campaign2Informer AS c2id
        LEFT JOIN Campaign2Informer AS c2ide ON c2id.id_cam=c2ide.id_cam AND c2ide.id_inf=%lld AND c2ide.allowed=1
        WHERE c2ide.id_cam IS NULL AND ((c2id.id_inf=%lld OR c2id.id_inf=1) AND c2id.allowed=0)
) AS c ON ca.id=c.id
LEFT JOIN Session AS ses ON ofrs.id=ses.offerId AND ses.id=%llu AND ses.uniqueHits <= 0
LEFT JOIN Informer2OfferRating AS iret ON iret.id_inf=%lld AND ofrs.id=iret.id_ofr
WHERE ofrs.valid=1
    AND ses.offerId IS NULL
;
