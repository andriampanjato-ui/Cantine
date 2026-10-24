USE cantine_universitaire;

-- 1) Ajouter colonnes snapshot
ALTER TABLE commande 
  ADD COLUMN IF NOT EXISTS nom_client_snapshot VARCHAR(100) NULL AFTER id_client,
  ADD COLUMN IF NOT EXISTS telephone_client_snapshot VARCHAR(20) NULL AFTER nom_client_snapshot;

-- 2) Remplir snapshots depuis client existant
UPDATE commande c
JOIN client cl ON cl.id_client = c.id_client
SET c.nom_client_snapshot = cl.nom,
    c.telephone_client_snapshot = cl.telephone;

-- 3) Pour commandes sans client existant, créer un snapshot générique
UPDATE commande 
SET nom_client_snapshot = COALESCE(nom_client_snapshot, CONCAT('Client #', id_client)),
    telephone_client_snapshot = COALESCE(telephone_client_snapshot, '')
WHERE nom_client_snapshot IS NULL AND id_client IS NOT NULL;

-- 4) Supprimer la contrainte FK existante
SET @fk_name = (
  SELECT CONSTRAINT_NAME 
  FROM information_schema.KEY_COLUMN_USAGE 
  WHERE TABLE_SCHEMA = 'cantine_universitaire' 
    AND TABLE_NAME = 'commande' 
    AND REFERENCED_TABLE_NAME = 'client' 
  LIMIT 1
);
SET @sql = IF(@fk_name IS NOT NULL, CONCAT('ALTER TABLE commande DROP FOREIGN KEY ', @fk_name), 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- 5) Rendre id_client nullable et ajouter contrainte SET NULL
ALTER TABLE commande MODIFY id_client INT NULL;
ALTER TABLE commande ADD CONSTRAINT fk_commande_client 
  FOREIGN KEY (id_client) REFERENCES client(id_client) 
  ON UPDATE RESTRICT ON DELETE SET NULL;

-- 6) Mettre à NULL les id_client orphelins
UPDATE commande c
LEFT JOIN client cl ON cl.id_client = c.id_client
SET c.id_client = NULL
WHERE cl.id_client IS NULL AND c.id_client IS NOT NULL;

-- Vérification
SELECT COUNT(*) AS total_commandes, 
       COUNT(id_client) AS avec_client, 
       COUNT(nom_client_snapshot) AS avec_snapshot 
FROM commande;
