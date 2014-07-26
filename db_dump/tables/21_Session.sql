CREATE TABLE IF NOT EXISTS Session
(
id INT8 NOT NULL,
offerId INT8 NOT NULL,
uniqueHits SMALLINT DEFAULT 1,
viewTime INT8,
tail SMALLINT DEFAULT 0,
retargeting SMALLINT DEFAULT 0,
UNIQUE (id,offerId) ON CONFLICT IGNORE,
FOREIGN KEY(offerId) REFERENCES Offer(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_Session_id ON Session (id);
CREATE INDEX IF NOT EXISTS idx_Session_id_offerId ON Session (id,offerId,uniqueHits,tail);
