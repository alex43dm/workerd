SELECT 	inf.id,
	inf.capacity,
	inf.bannersCss,
	inf.teasersCss,
	inf.domainId,
	inf.accountId,
	inf.rtgPercentage
FROM Informer AS inf
WHERE inf.guid='%s'
LIMIT 1;
