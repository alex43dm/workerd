CREATE TABLE IF NOT EXISTS Categories2Informer
(
id_cat INT8 NOT NULL,
id_inf INT8 NOT NULL,
FOREIGN KEY(id_cat) REFERENCES Categories(id),
FOREIGN KEY(id_inf) REFERENCES Informer(id)
);
