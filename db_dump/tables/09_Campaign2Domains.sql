CREATE TABLE IF NOT EXISTS Campaign2Domains
(
id_cam INT8 NOT NULL,
id_dom INT8 NOT NULL,
allowed SMALLINT,
FOREIGN KEY(id_cam) REFERENCES Campaign(id),
FOREIGN KEY(id_dom) REFERENCES Domains(id)
);
CREATE INDEX IF NOT EXISTS idx_Campaign2Domains_allowed ON Campaign2Domains (allowed);
CREATE INDEX IF NOT EXISTS idx_Campaign2Domains_id_dom ON Campaign2Domains (id_dom);
CREATE INDEX IF NOT EXISTS idx_Campaign2Domains_id_dom_allowed ON Campaign2Domains (id_dom,allowed);
