CREATE TABLE IF NOT EXISTS Campaign2Informer
(
id_cam INT8 NOT NULL,
id_inf INT8 NOT NULL,
allowed SMALLINT,
FOREIGN KEY(id_cam) REFERENCES Campaign(id),
FOREIGN KEY(id_inf) REFERENCES Informer(id)
);
CREATE INDEX IF NOT EXISTS idx_Campaign2Informer_allowed ON Campaign2Informer (allowed);
CREATE INDEX IF NOT EXISTS idx_Campaign2Informer_id_inf ON Campaign2Informer (id_inf);
CREATE INDEX IF NOT EXISTS idx_Campaign2Informer_id_inf_allowed ON Campaign2Informer (id_inf,allowed);
