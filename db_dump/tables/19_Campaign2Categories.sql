CREATE TABLE IF NOT EXISTS Campaign2Categories
(
id_cam INT8 NOT NULL,
id_cat INT8 NOT NULL,
FOREIGN KEY(id_cam) REFERENCES Campaign(id),
FOREIGN KEY(id_cat) REFERENCES Categories(id)
);
CREATE INDEX IF NOT EXISTS idx_Campaign2Categories_id_cam ON Campaign2Categories (id_cam);
CREATE INDEX IF NOT EXISTS idx_Campaign2Categories_id_cat ON Campaign2Categories (id_cat);
