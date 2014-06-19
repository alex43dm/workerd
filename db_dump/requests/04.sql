SELECT 	inf.id,
	inf.capacity,
	inf.bannersCss,
	inf.teasersCss,
	inf.domainId,
	inf.accountId,
    inf.range_short_term,
	inf.range_long_term,
	inf.range_context,
	inf.range_search,
	inf.retargeting_capacity
FROM Informer AS inf
WHERE inf.guid='%s'
LIMIT 1;
