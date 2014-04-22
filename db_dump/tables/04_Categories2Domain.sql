CREATE TABLE IF NOT EXISTS Categories2Domain
(
id_cat INT8 NOT NULL,
id_dom INT8 NOT NULL,
FOREIGN KEY(id_cat) REFERENCES Categories(id),
FOREIGN KEY(id_dom) REFERENCES Domains(id)
);
CREATE INDEX IF NOT EXISTS idx_Categories2Domain_id_cat ON Categories2Domain (id_cat);
CREATE INDEX IF NOT EXISTS idx_Categories2Domain_id_dom ON Categories2Domain (id_dom);
CREATE INDEX IF NOT EXISTS idx_Categories2Domain_id_dom_id_cat ON Categories2Domain (id_cat,id_dom);
