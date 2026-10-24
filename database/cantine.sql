-- MariaDB dump 10.19  Distrib 10.4.32-MariaDB, for Win64 (AMD64)
--
-- Host: 127.0.0.1    Database: cantine_universitaire
-- ------------------------------------------------------
-- Server version	10.4.32-MariaDB

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `client`
--

DROP TABLE IF EXISTS `client`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `client` (
  `id_client` int(11) NOT NULL AUTO_INCREMENT,
  `nom` varchar(100) NOT NULL,
  `telephone` varchar(20) DEFAULT NULL,
  PRIMARY KEY (`id_client`),
  CONSTRAINT `chk_client_nom_nonvide` CHECK (trim(`nom`) <> '')
) ENGINE=InnoDB AUTO_INCREMENT=18 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `client`
--

LOCK TABLES `client` WRITE;
/*!40000 ALTER TABLE `client` DISABLE KEYS */;
/*!40000 ALTER TABLE `client` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `commande`
--

DROP TABLE IF EXISTS `commande`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `commande` (
  `id_commande` int(11) NOT NULL AUTO_INCREMENT,
  `id_client` int(11) DEFAULT NULL,
  `nom_client_snapshot` varchar(100) DEFAULT NULL,
  `telephone_client_snapshot` varchar(20) DEFAULT NULL,
  `date_commande` datetime NOT NULL,
  `statut` varchar(20) NOT NULL,
  PRIMARY KEY (`id_commande`),
  KEY `idx_commande_client` (`id_client`),
  KEY `idx_commande_date` (`date_commande`),
  KEY `idx_commande_statut` (`statut`),
  CONSTRAINT `fk_commande_client` FOREIGN KEY (`id_client`) REFERENCES `client` (`id_client`) ON DELETE SET NULL,
  CONSTRAINT `chk_commande_statut` CHECK (`statut` in ('EN_ATTENTE','VALIDEE','ANNULEE'))
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `commande`
--

LOCK TABLES `commande` WRITE;
/*!40000 ALTER TABLE `commande` DISABLE KEYS */;
INSERT INTO `commande` VALUES (1,NULL,'Client #3','','2026-10-22 22:07:05','ANNULEE'),(2,NULL,'Client #3','','2026-10-22 22:07:05','ANNULEE'),(3,NULL,'Client #3','','2026-10-22 22:07:05','ANNULEE'),(4,NULL,'Client #3','','2026-10-23 09:12:16','ANNULEE'),(5,NULL,'Client #3','','2026-10-23 09:12:16','ANNULEE'),(6,NULL,'Client #3','','2026-10-23 09:12:16','ANNULEE'),(7,NULL,'Client #3','','2026-10-23 09:12:16','ANNULEE'),(8,NULL,'Client #4','','2026-10-23 09:12:16','ANNULEE'),(9,NULL,'Isma','0389753305','2026-10-23 18:31:14','ANNULEE');
/*!40000 ALTER TABLE `commande` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `detail_commande`
--

DROP TABLE IF EXISTS `detail_commande`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `detail_commande` (
  `id_detail` int(11) NOT NULL AUTO_INCREMENT,
  `id_commande` int(11) NOT NULL,
  `id_menu` int(11) DEFAULT NULL,
  `nom_plat_snapshot` varchar(100) DEFAULT NULL,
  `nom_menu_snapshot` varchar(100) DEFAULT NULL,
  `quantite` int(11) NOT NULL,
  `prix_unitaire` decimal(10,2) NOT NULL,
  PRIMARY KEY (`id_detail`),
  KEY `idx_detail_commande` (`id_commande`),
  KEY `idx_detail_menu` (`id_menu`),
  CONSTRAINT `fk_detail_commande` FOREIGN KEY (`id_commande`) REFERENCES `commande` (`id_commande`) ON DELETE CASCADE,
  CONSTRAINT `fk_detail_menu` FOREIGN KEY (`id_menu`) REFERENCES `menu_jour` (`id_menu`) ON DELETE SET NULL,
  CONSTRAINT `chk_detail_quantite_positif` CHECK (`quantite` > 0),
  CONSTRAINT `chk_detail_prix_unitaire_positif` CHECK (`prix_unitaire` >= 0)
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `detail_commande`
--

LOCK TABLES `detail_commande` WRITE;
/*!40000 ALTER TABLE `detail_commande` DISABLE KEYS */;
INSERT INTO `detail_commande` VALUES (1,1,1,'Akoho gasy ronony','Akoho gasy ronony',4,20000.00),(2,2,2,'Gana sauce','Gana sauce',2,1500000.00),(3,3,3,'Hena omby ritra','Hena omby ritra',1,100000.00),(4,4,5,'Gana sauce','Gana sauce',3,1500000.00),(5,5,8,'Hena omby ritra','Hena omby ritra',1,100000.00),(6,6,10,'kotileta','kotileta',2,9000.00),(7,7,8,'Hena omby ritra','Hena omby ritra',1,100000.00),(8,8,10,'kotileta','kotileta',4,9000.00),(9,9,5,'Gana sauce','Gana sauce',1,1500000.00);
/*!40000 ALTER TABLE `detail_commande` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `menu_jour`
--

DROP TABLE IF EXISTS `menu_jour`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `menu_jour` (
  `id_menu` int(11) NOT NULL AUTO_INCREMENT,
  `id_plat` int(11) DEFAULT NULL,
  `date_menu` date NOT NULL,
  `quantite_initiale` int(11) NOT NULL,
  PRIMARY KEY (`id_menu`),
  UNIQUE KEY `uq_menu_plat_date` (`id_plat`,`date_menu`),
  KEY `idx_menu_date` (`date_menu`),
  KEY `idx_menu_plat` (`id_plat`),
  CONSTRAINT `fk_menu_plat` FOREIGN KEY (`id_plat`) REFERENCES `plat` (`id_plat`) ON DELETE SET NULL,
  CONSTRAINT `chk_menu_qte_init_positif` CHECK (`quantite_initiale` >= 0)
) ENGINE=InnoDB AUTO_INCREMENT=19 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `menu_jour`
--

LOCK TABLES `menu_jour` WRITE;
/*!40000 ALTER TABLE `menu_jour` DISABLE KEYS */;
INSERT INTO `menu_jour` VALUES (1,NULL,'2026-10-22',20),(2,NULL,'2026-10-22',20),(3,NULL,'2026-10-22',20),(4,NULL,'2026-10-22',20),(5,NULL,'2026-10-23',20),(8,NULL,'2026-10-23',20),(10,NULL,'2026-10-23',20);
/*!40000 ALTER TABLE `menu_jour` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `plat`
--

DROP TABLE IF EXISTS `plat`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `plat` (
  `id_plat` int(11) NOT NULL AUTO_INCREMENT,
  `nom_plat` varchar(100) NOT NULL,
  `prix` decimal(10,2) NOT NULL,
  PRIMARY KEY (`id_plat`),
  CONSTRAINT `chk_plat_nom_nonvide` CHECK (trim(`nom_plat`) <> ''),
  CONSTRAINT `chk_plat_prix_positif` CHECK (`prix` >= 0)
) ENGINE=InnoDB AUTO_INCREMENT=11 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `plat`
--

LOCK TABLES `plat` WRITE;
/*!40000 ALTER TABLE `plat` DISABLE KEYS */;
/*!40000 ALTER TABLE `plat` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Temporary table structure for view `v_menu_disponibilite`
--

DROP TABLE IF EXISTS `v_menu_disponibilite`;
/*!50001 DROP VIEW IF EXISTS `v_menu_disponibilite`*/;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
/*!50001 CREATE VIEW `v_menu_disponibilite` AS SELECT
 1 AS `id_menu`,
  1 AS `id_plat`,
  1 AS `nom_plat`,
  1 AS `prix_actuel_plat`,
  1 AS `date_menu`,
  1 AS `quantite_initiale`,
  1 AS `quantite_commandee`,
  1 AS `quantite_restante` */;
SET character_set_client = @saved_cs_client;

--
-- Temporary table structure for view `v_quantite_commandee_par_menu`
--

DROP TABLE IF EXISTS `v_quantite_commandee_par_menu`;
/*!50001 DROP VIEW IF EXISTS `v_quantite_commandee_par_menu`*/;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
/*!50001 CREATE VIEW `v_quantite_commandee_par_menu` AS SELECT
 1 AS `id_menu`,
  1 AS `quantite_commandee` */;
SET character_set_client = @saved_cs_client;

--
-- Dumping events for database 'cantine_universitaire'
--

--
-- Dumping routines for database 'cantine_universitaire'
--

--
-- Final view structure for view `v_menu_disponibilite`
--

/*!50001 DROP VIEW IF EXISTS `v_menu_disponibilite`*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = cp850 */;
/*!50001 SET character_set_results     = cp850 */;
/*!50001 SET collation_connection      = cp850_general_ci */;
/*!50001 CREATE ALGORITHM=UNDEFINED */
/*!50013 DEFINER=`root`@`localhost` SQL SECURITY DEFINER */
/*!50001 VIEW `v_menu_disponibilite` AS select `m`.`id_menu` AS `id_menu`,`m`.`id_plat` AS `id_plat`,`p`.`nom_plat` AS `nom_plat`,`p`.`prix` AS `prix_actuel_plat`,`m`.`date_menu` AS `date_menu`,`m`.`quantite_initiale` AS `quantite_initiale`,coalesce(`vqc`.`quantite_commandee`,0) AS `quantite_commandee`,`m`.`quantite_initiale` - coalesce(`vqc`.`quantite_commandee`,0) AS `quantite_restante` from ((`menu_jour` `m` join `plat` `p` on(`p`.`id_plat` = `m`.`id_plat`)) left join `v_quantite_commandee_par_menu` `vqc` on(`vqc`.`id_menu` = `m`.`id_menu`)) */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;

--
-- Final view structure for view `v_quantite_commandee_par_menu`
--

/*!50001 DROP VIEW IF EXISTS `v_quantite_commandee_par_menu`*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = cp850 */;
/*!50001 SET character_set_results     = cp850 */;
/*!50001 SET collation_connection      = cp850_general_ci */;
/*!50001 CREATE ALGORITHM=UNDEFINED */
/*!50013 DEFINER=`root`@`localhost` SQL SECURITY DEFINER */
/*!50001 VIEW `v_quantite_commandee_par_menu` AS select `dc`.`id_menu` AS `id_menu`,coalesce(sum(`dc`.`quantite`),0) AS `quantite_commandee` from (`detail_commande` `dc` join `commande` `c` on(`c`.`id_commande` = `dc`.`id_commande`)) where `c`.`statut` = 'VALIDEE' group by `dc`.`id_menu` */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-10-24 23:03:03
