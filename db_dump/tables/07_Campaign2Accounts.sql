CREATE TABLE IF NOT EXISTS Campaign2Accounts
(
id_cam INT8 NOT NULL,
id_acc INT8 NOT NULL,
allowed SMALLINT,
FOREIGN KEY(id_cam) REFERENCES Campaign(id),
FOREIGN KEY(id_acc) REFERENCES Accounts(id)
);
CREATE INDEX IF NOT EXISTS idx_Campaign2Accounts_allowed ON Campaign2Accounts (allowed);
CREATE INDEX IF NOT EXISTS idx_Campaign2Accounts_id_acc ON Campaign2Accounts (id_acc);
CREATE INDEX IF NOT EXISTS idx_Campaign2Accounts_id_acc_allowed ON Campaign2Accounts (id_acc,allowed);
