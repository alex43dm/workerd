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
ca.social
FROM Offer AS ofrs
INNER JOIN Campaign AS ca ON ofrs.campaignId=ca.id
INNER JOIN (
		SELECT cn.id FROM CampaignNow AS cn
		%s
		EXCEPT
            SELECT c2d.id_cam AS id
            FROM Campaign2Domains AS c2d
            WHERE c2d.id_dom=%lld AND c2d.allowed=0
		UNION ALL
            SELECT c2a.id_cam AS id
            FROM Campaign2Accounts AS c2a
            WHERE c2a.id_acc=%lld AND c2a.allowed=1
        UNION ALL
            SELECT c2i.id_cam AS id
            FROM Campaign2Informer AS c2i
            WHERE c2i.allowed=1 AND c2i.id_inf=%lld
) AS c ON ca.id=c.id
LEFT JOIN tmp%d%lld AS deph ON ofrs.id=deph.id
LEFT JOIN Informer2OfferRating AS iret ON iret.id_inf=%lld AND ofrs.id=iret.id_ofr
WHERE ofrs.valid=1
    AND deph.id IS NULL
;
