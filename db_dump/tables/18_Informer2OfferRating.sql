CREATE TABLE IF NOT EXISTS Informer2OfferRating
(
id_inf INT8 NOT NULL,
id_ofr INT8 NOT NULL,
rating DECIMAL(5,2),
FOREIGN KEY(id_ofr) REFERENCES Offer(id) ON DELETE CASCADE,
FOREIGN KEY(id_inf) REFERENCES Informer(id) ON DELETE CASCADE
);
CREATE INDEX IF NOT EXISTS idx_Informer2OfferRating_id_inf ON Informer2OfferRating (id_inf);
CREATE INDEX IF NOT EXISTS idx_Informer2OfferRating_id_ofr ON Informer2OfferRating (id_ofr);
