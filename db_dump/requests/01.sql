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
ca.social
FROM Offer AS ofrs
INNER JOIN Campaign AS ca ON ofrs.campaignId=ca.id
INNER JOIN (
		SELECT cn.id FROM Campaign2Time AS cn
		EXCEPT
            SELECT c2d.id_cam AS id
            FROM Campaign2Domains AS c2d
            WHERE c2d.id_dom = @domainsId AND c2d.allowed=0
		UNION ALL
            SELECT c2a.id_cam AS id
            FROM Campaign2Accounts AS c2a
            WHERE c2a.id_acc=@accountId AND c2a.allowed=1
        UNION ALL
            SELECT c2i.id_cam AS id
            FROM Campaign2Informer AS c2i
            WHERE c2i.allowed=1 AND c2i.id_inf=@informerId
        UNION ALL
            SELECT geo.id_cam AS id
            FROM geoTargeting AS geo
            INNER JOIN GeoRerions AS reg ON geo.id_geo = reg.id AND (reg.cid = @countryId OR reg.rid = @regionCode)
) AS c ON ca.id=c.id
LIMIT 4;
