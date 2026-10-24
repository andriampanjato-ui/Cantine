-- Migration : permettre la suppression d'un client tout en conservant l'historique
USE cantine_universitaire;

-- 1) Ajouter des colonnes de snapshot dans commande
ALTER TABLE commande 
  ADD COLUMN nom_client_snapshot VARCHAR(100) NULL AFTER id_client,
  ADD COLUMN telephone_client_snapshot VARCHAR(20) NULL AFTER nom_client_snapshot;

-- 2) Remplir les snapshots pour les commandes existantes
UPDATE commande c
JOIN client cl ON cl.id_client = c.id_client
SET c.nom_client_snapshot = cl.nom,
    c.telephone_client_snapshot = cl.telephone;

-- 3) Rendre id_client nullable
ALTER TABLE commande MODIFY id_client INT NULL;

-- 4) Changer la contrainte FK pour autoriser SET NULL
ALTER TABLE commande DROP FOREIGN KEY fk_commande_client;
ALTER TABLE commande ADD CONSTRAINT fk_commande_client 
  FOREIGN KEY (id_client) REFERENCES client(id_client) 
  ON UPDATE RESTRICT ON DELETE SET NULL;

-- 5) Vérification
SELECT COUNT(*) AS commandes_avec_snapshot FROM commande WHERE nom_client_snapshot IS NOT NULL;
SELECT COUNT(*) AS commandes_sans_client FROM commande WHERE id_client IS NULL;
