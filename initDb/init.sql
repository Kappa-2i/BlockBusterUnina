DO $$ BEGIN
    CREATE TYPE stato AS ENUM ('effettuato', 'scaduto', 'restituito');
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

CREATE TABLE utenti(
    id SERIAL NOT NULL,
    username varchar(50),
    password varchar(50),
    PRIMARY KEY(id)
);
CREATE UNIQUE INDEX utenti_username_key ON public.utenti USING btree (username);

CREATE TABLE film(
    id SERIAL NOT NULL,
    titolo varchar(100),
    copie_disponibili integer,
    copie_prestito integer,
    PRIMARY KEY(id)
);
CREATE UNIQUE INDEX unique_title ON public.film USING btree (titolo);

CREATE TABLE prestiti(
    id SERIAL NOT NULL,
    id_utente integer,
    id_film integer,
    data_prestito timestamp without time zone,
    data_restituzione timestamp without time zone,
    stato stato,
    PRIMARY KEY(id),
    CONSTRAINT prestiti_id_utente_fkey FOREIGN key(id_utente) REFERENCES utenti(id),
    CONSTRAINT prestiti_id_film_fkey FOREIGN key(id_film) REFERENCES film(id)
);

INSERT INTO film (titolo, copie_disponibili, copie_prestito) VALUES
('La Haine', 5, 0),
('Inception', 3, 0),
('Interstellar', 4, 0),
('Il Padrino', 2, 0),
('Pulp Fiction', 3, 0),
('Fight Club', 4, 0),
('The Matrix', 5, 0),
('Forrest Gump', 2, 0),
('Shutter Island', 3, 0),
('The Dark Knight', 6, 0);
