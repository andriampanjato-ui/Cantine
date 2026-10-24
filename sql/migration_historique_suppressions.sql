USE cantine_universitaire;

-- 1) Ajouter snapshots dans detail_commande pour préserver l'historique
ALTER TABLE detail_commande 
  ADD COLUMN IF NOT EXISTS nom_plat_snapshot VARCHAR(100) NULL AFTER id_menu,
  ADD COLUMN IF NOT EXISTS nom_menu_snapshot VARCHAR(100) NULL AFTER nom_plat_snapshot;

-- Remplir snapshots existants
UPDATE detail_commande dc
LEFT JOIN menu_jour m ON m.id_menu = dc.id_menu
LEFT JOIN plat p ON p.id_plat = m.id_plat
SET dc.nom_plat_snapshot = COALESCE(dc.nom_plat_snapshot, p.nom_plat, ''),
    dc.nom_menu_snapshot = COALESCE(dc.nom_menu_snapshot, p.nom_plat, '');

-- 2) Assouplir les FK sur detail_commande pour permettre suppression tout en conservant historique
-- Supprimer contrainte fk_detail_menu
SET @fk = (
  SELECT CONSTRAINT_NAME 
  FROM information_schema.KEY_COLUMN_USAGE 
  WHERE TABLE_SCHEMA = 'cantine_universitaire' 
    AND TABLE_NAME = 'detail_commande' 
    AND REFERENCED_TABLE_NAME = 'menu_jour' 
  LIMIT 1
);
SET @sql = IF(@fk IS NOT NULL, CONCAT('ALTER TABLE detail_commande DROP FOREIGN KEY ', @fk), 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- Rendre id_menu nullable
ALTER TABLE detail_commande MODIFY id_menu INT NULL;
-- Ajouter FK avec SET NULL
ALTER TABLE detail_commande ADD CONSTRAINT fk_detail_menu 
  FOREIGN KEY (id_menu) REFERENCES menu_jour(id_menu) 
  ON UPDATE RESTRICT ON DELETE SET NULL;

-- Mettre à NULL les références orphelines
UPDATE detail_commande dc
LEFT JOIN menu_jour m ON m.id_menu = dc.id_menu
SET dc.id_menu = NULL
WHERE m.id_menu IS NULL AND dc.id_menu IS NOT NULL;

-- 3) Assouplir FK menu_jour -> plat pour permettre suppression plat si nécessaire
SET @fk2 = (
  SELECT CONSTRAINT_NAME 
  FROM information_schema.KEY_COLUMN_USAGE 
  WHERE TABLE_SCHEMA = 'cantine_universitaire' 
    AND TABLE_NAME = 'menu_jour' 
    AND REFERENCED_TABLE_NAME = 'plat' 
  LIMIT 1
);
SET @sql2 = IF(@fk2 IS NOT NULL, CONCAT('ALTER TABLE menu_jour DROP FOREIGN KEY ', @fk2), 'SELECT 1');
PREPARE stmt2 FROM @sql2;
EXECUTE stmt2;
DEALLOCATE PREPARE stmt2;

ALTER TABLE menu_jour MODIFY id_plat INT NULL;
ALTER TABLE menu_jour ADD CONSTRAINT fk_menu_plat 
  FOREIGN KEY (id_plat) REFERENCES plat(id_plat) 
  ON UPDATE RESTRICT ON DELETE SET NULL;

-- 4) Assouplir FK detail_commande -> commande pour permettre suppression commande depuis historique/nouvelles commandes
SET @fk3 = (
  SELECT CONSTRAINT_NAME 
  FROM information_schema.KEY_COLUMN_USAGE 
  WHERE TABLE_SCHEMA = 'cantine_universitaire' 
    AND TABLE_NAME = 'detail_commande' 
    AND REFERENCED_TABLE_NAME = 'commande' 
  LIMIT 1
);
SET @sql3 = IF(@fk3 IS NOT NULL, CONCAT('ALTER TABLE detail_commande DROP FOREIGN KEY ', @fk3), 'SELECT 1');
PREPARE stmt3 FROM @sql3;
EXECUTE stmt3;
DEALLOCATE PREPARE stmt3;

ALTER TABLE detail_commande ADD CONSTRAINT fk_detail_commande 
  FOREIGN KEY (id_commande) REFERENCES commande(id_commande) 
  ON UPDATE RESTRICT ON DELETE CASCADE;

SELECT 'Migration historique_suppressions OK' AS status;
SHOW COLUMNS FROM detail_commande;
SHOW COLUMNS FROM menu_jour;
SHOW COLUMNS FROM commande;
