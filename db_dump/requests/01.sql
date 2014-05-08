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
ca.guid
FROM Offer AS ofrs
INNER JOIN Campaign AS ca ON ca.valid=1 AND ca.retargeting=0 AND ofrs.campaignId=ca.id
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
        WHERE (c2dd.id_dom=%lld OR c2dd.id_dom=1) AND c2dd.allowed=0
        UNION ALL
        SELECT c2aa.id_cam AS id
        FROM Campaign2Accounts AS c2aa
        WHERE (c2aa.id_acc=%lld OR c2aa.id_acc=1) AND c2aa.allowed=1
		EXCEPT
        SELECT c2ad.id_cam AS id
        FROM Campaign2Accounts AS c2ad
        WHERE (c2ad.id_acc=%lld OR c2ad.id_acc=1) AND c2ad.allowed=0
        UNION ALL
        SELECT c2ia.id_cam AS id
        FROM Campaign2Informer AS c2ia
        WHERE (c2ia.id_inf=%lld OR c2ia.id_inf=1) AND c2ia.allowed=1
        EXCEPT
        SELECT c2id.id_cam AS id
        FROM Campaign2Informer AS c2id
        WHERE (c2id.id_inf=%lld OR c2id.id_inf=1) AND c2id.allowed=0
) AS c ON ca.id=c.id
LEFT JOIN tmp%d%lld AS deph ON ofrs.id=deph.id
LEFT JOIN Informer2OfferRating AS iret ON iret.id_inf=%lld AND ofrs.id=iret.id_ofr
WHERE ofrs.valid=1
    AND deph.id IS NULL
;
