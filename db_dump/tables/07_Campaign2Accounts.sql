CREATE TABLE IF NOT EXISTS Campaign2Accounts
(
id_cam INT8 NOT NULL,
id_acc INT8 NOT NULL,
allowed SMALLINT,
UNIQUE (id_acc,allowed) ON CONFLICT IGNORE,
FOREIGN KEY(id_cam) REFERENCES Campaign(id),
FOREIGN KEY(id_acc) REFERENCES Accounts(id)
);
--CREATE INDEX IF NOT EXISTS idx_Campaign2Accounts_id_cam ON Campaign2Accounts (id_cam);
CREATE INDEX IF NOT EXISTS idx_Campaign2Accounts_id_acc_allowed ON Campaign2Accounts (id_acc,allowed);
