SELECT 	inf.id,
	inf.capacity,
	inf.bannersCss,
	inf.teasersCss,
	inf.domainId,
	inf.accountId
FROM Informer AS inf
WHERE inf.guid=@informerUid
LIMIT 1;
