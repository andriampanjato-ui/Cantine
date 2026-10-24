DROP DATABASE IF EXISTS cantine_universitaire;
CREATE DATABASE cantine_universitaire CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE cantine_universitaire;

CREATE TABLE client (
  id_client INT AUTO_INCREMENT PRIMARY KEY,
  nom VARCHAR(100) NOT NULL,
  telephone VARCHAR(20),
  CONSTRAINT chk_client_nom_nonvide CHECK (TRIM(nom) <> '')
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE plat (
  id_plat INT AUTO_INCREMENT PRIMARY KEY,
  nom_plat VARCHAR(100) NOT NULL,
  prix DECIMAL(10,2) NOT NULL,
  CONSTRAINT chk_plat_nom_nonvide CHECK (TRIM(nom_plat) <> ''),
  CONSTRAINT chk_plat_prix_positif CHECK (prix >= 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE menu_jour (
  id_menu INT AUTO_INCREMENT PRIMARY KEY,
  id_plat INT NOT NULL,
  date_menu DATE NOT NULL,
  quantite_initiale INT NOT NULL,
  CONSTRAINT fk_menu_plat FOREIGN KEY (id_plat) REFERENCES plat(id_plat) ON UPDATE RESTRICT ON DELETE RESTRICT,
  CONSTRAINT chk_menu_qte_init_positif CHECK (quantite_initiale >= 0),
  CONSTRAINT uq_menu_plat_date UNIQUE (id_plat, date_menu)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE INDEX idx_menu_date ON menu_jour(date_menu);
CREATE INDEX idx_menu_plat ON menu_jour(id_plat);

CREATE TABLE commande (
  id_commande INT AUTO_INCREMENT PRIMARY KEY,
  id_client INT NOT NULL,
  date_commande DATETIME NOT NULL,
  statut VARCHAR(20) NOT NULL,
  CONSTRAINT fk_commande_client FOREIGN KEY (id_client) REFERENCES client(id_client) ON UPDATE RESTRICT ON DELETE RESTRICT,
  CONSTRAINT chk_commande_statut CHECK (statut IN ('EN_ATTENTE','VALIDEE','ANNULEE'))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE INDEX idx_commande_client ON commande(id_client);
CREATE INDEX idx_commande_date ON commande(date_commande);
CREATE INDEX idx_commande_statut ON commande(statut);

CREATE TABLE detail_commande (
  id_detail INT AUTO_INCREMENT PRIMARY KEY,
  id_commande INT NOT NULL,
  id_menu INT NOT NULL,
  quantite INT NOT NULL,
  prix_unitaire DECIMAL(10,2) NOT NULL,
  CONSTRAINT fk_detail_commande FOREIGN KEY (id_commande) REFERENCES commande(id_commande) ON UPDATE RESTRICT ON DELETE RESTRICT,
  CONSTRAINT fk_detail_menu FOREIGN KEY (id_menu) REFERENCES menu_jour(id_menu) ON UPDATE RESTRICT ON DELETE RESTRICT,
  CONSTRAINT chk_detail_quantite_positif CHECK (quantite > 0),
  CONSTRAINT chk_detail_prix_unitaire_positif CHECK (prix_unitaire >= 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE INDEX idx_detail_commande ON detail_commande(id_commande);
CREATE INDEX idx_detail_menu ON detail_commande(id_menu);

CREATE OR REPLACE VIEW v_quantite_commandee_par_menu AS
SELECT dc.id_menu, COALESCE(SUM(dc.quantite), 0) AS quantite_commandee
FROM detail_commande dc
INNER JOIN commande c ON c.id_commande = dc.id_commande
WHERE c.statut = 'VALIDEE'
GROUP BY dc.id_menu;

CREATE OR REPLACE VIEW v_menu_disponibilite AS
SELECT m.id_menu, m.id_plat, p.nom_plat, p.prix AS prix_actuel_plat, m.date_menu, m.quantite_initiale,
       COALESCE(vqc.quantite_commandee, 0) AS quantite_commandee,
       (m.quantite_initiale - COALESCE(vqc.quantite_commandee, 0)) AS quantite_restante
FROM menu_jour m
INNER JOIN plat p ON p.id_plat = m.id_plat
LEFT JOIN v_quantite_commandee_par_menu vqc ON vqc.id_menu = m.id_menu;
