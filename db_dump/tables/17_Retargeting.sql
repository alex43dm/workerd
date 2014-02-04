CREATE TABLE IF NOT EXISTS Retargeting
(
id INT8 NOT NULL,
offerId INT8,
uniqueHits SMALLINT,
viewTime INT8,
UNIQUE (id,offerId) ON CONFLICT IGNORE,
FOREIGN KEY(offerId) REFERENCES Offer(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_Offer_offerId ON Retargeting (offerId);
CREATE INDEX IF NOT EXISTS idx_Offer_uniqueHits ON Retargeting (uniqueHits);
CREATE INDEX IF NOT EXISTS idx_Offer_viewTime ON Retargeting (viewTime);
