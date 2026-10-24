-- phpMyAdmin SQL Dump
-- version 5.2.1
-- https://www.phpmyadmin.net/
--
-- Hôte : 127.0.0.1
-- Généré le : ven. 23 oct. 2026 à 08:11
-- Version du serveur : 10.4.32-MariaDB
-- Version de PHP : 8.2.12

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Base de données : `cantine_universitaire`
--

-- --------------------------------------------------------

--
-- Structure de la table `client`
--

CREATE TABLE `client` (
  `id_client` int(11) NOT NULL,
  `nom` varchar(100) NOT NULL,
  `telephone` varchar(20) DEFAULT NULL
) ;

--
-- Déchargement des données de la table `client`
--

INSERT INTO `client` (`id_client`, `nom`, `telephone`) VALUES
(1, 'Manampy', '0380894821'),
(2, 'Ism', '0389753305'),
(3, 'Fenitra', '0343302816'),
(4, 'Toky', '0389632154');

-- --------------------------------------------------------

--
-- Structure de la table `commande`
--

CREATE TABLE `commande` (
  `id_commande` int(11) NOT NULL,
  `id_client` int(11) NOT NULL,
  `date_commande` datetime NOT NULL,
  `statut` varchar(20) NOT NULL
) ;

--
-- Déchargement des données de la table `commande`
--

INSERT INTO `commande` (`id_commande`, `id_client`, `date_commande`, `statut`) VALUES
(1, 3, '2026-10-22 22:07:05', 'VALIDEE'),
(2, 3, '2026-10-22 22:07:05', 'EN_ATTENTE'),
(3, 3, '2026-10-22 22:07:05', 'ANNULEE');

-- --------------------------------------------------------

--
-- Structure de la table `detail_commande`
--

CREATE TABLE `detail_commande` (
  `id_detail` int(11) NOT NULL,
  `id_commande` int(11) NOT NULL,
  `id_menu` int(11) NOT NULL,
  `quantite` int(11) NOT NULL,
  `prix_unitaire` decimal(10,2) NOT NULL
) ;

--
-- Déchargement des données de la table `detail_commande`
--

INSERT INTO `detail_commande` (`id_detail`, `id_commande`, `id_menu`, `quantite`, `prix_unitaire`) VALUES
(1, 1, 1, 4, 20000.00),
(2, 2, 2, 2, 1500000.00),
(3, 3, 3, 1, 100000.00);

-- --------------------------------------------------------

--
-- Structure de la table `menu_jour`
--

CREATE TABLE `menu_jour` (
  `id_menu` int(11) NOT NULL,
  `id_plat` int(11) NOT NULL,
  `date_menu` date NOT NULL,
  `quantite_initiale` int(11) NOT NULL
) ;

--
-- Déchargement des données de la table `menu_jour`
--

INSERT INTO `menu_jour` (`id_menu`, `id_plat`, `date_menu`, `quantite_initiale`) VALUES
(1, 1, '2026-10-22', 20),
(2, 2, '2026-10-22', 20),
(3, 4, '2026-10-22', 20),
(4, 3, '2026-10-22', 20);

-- --------------------------------------------------------

--
-- Structure de la table `plat`
--

CREATE TABLE `plat` (
  `id_plat` int(11) NOT NULL,
  `nom_plat` varchar(100) NOT NULL,
  `prix` decimal(10,2) NOT NULL
) ;

--
-- Déchargement des données de la table `plat`
--

INSERT INTO `plat` (`id_plat`, `nom_plat`, `prix`) VALUES
(1, 'Akoho gasy ronony', 20000.00),
(2, 'Gana sauce', 1500000.00),
(3, 'kotileta', 9000.00),
(4, 'Hena omby ritra', 100000.00);

-- --------------------------------------------------------

--
-- Doublure de structure pour la vue `v_menu_disponibilite`
-- (Voir ci-dessous la vue réelle)
--
CREATE TABLE `v_menu_disponibilite` (
`id_menu` int(11)
,`id_plat` int(11)
,`nom_plat` varchar(100)
,`prix_actuel_plat` decimal(10,2)
,`date_menu` date
,`quantite_initiale` int(11)
,`quantite_commandee` decimal(32,0)
,`quantite_restante` decimal(33,0)
);

-- --------------------------------------------------------

--
-- Doublure de structure pour la vue `v_quantite_commandee_par_menu`
-- (Voir ci-dessous la vue réelle)
--
CREATE TABLE `v_quantite_commandee_par_menu` (
`id_menu` int(11)
,`quantite_commandee` decimal(32,0)
);

-- --------------------------------------------------------

--
-- Structure de la vue `v_menu_disponibilite`
--
DROP TABLE IF EXISTS `v_menu_disponibilite`;

CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `v_menu_disponibilite`  AS SELECT `m`.`id_menu` AS `id_menu`, `m`.`id_plat` AS `id_plat`, `p`.`nom_plat` AS `nom_plat`, `p`.`prix` AS `prix_actuel_plat`, `m`.`date_menu` AS `date_menu`, `m`.`quantite_initiale` AS `quantite_initiale`, coalesce(`vqc`.`quantite_commandee`,0) AS `quantite_commandee`, `m`.`quantite_initiale`- coalesce(`vqc`.`quantite_commandee`,0) AS `quantite_restante` FROM ((`menu_jour` `m` join `plat` `p` on(`p`.`id_plat` = `m`.`id_plat`)) left join `v_quantite_commandee_par_menu` `vqc` on(`vqc`.`id_menu` = `m`.`id_menu`)) ;

-- --------------------------------------------------------

--
-- Structure de la vue `v_quantite_commandee_par_menu`
--
DROP TABLE IF EXISTS `v_quantite_commandee_par_menu`;

CREATE ALGORITHM=UNDEFINED DEFINER=`root`@`localhost` SQL SECURITY DEFINER VIEW `v_quantite_commandee_par_menu`  AS SELECT `dc`.`id_menu` AS `id_menu`, coalesce(sum(`dc`.`quantite`),0) AS `quantite_commandee` FROM (`detail_commande` `dc` join `commande` `c` on(`c`.`id_commande` = `dc`.`id_commande`)) WHERE `c`.`statut` = 'VALIDEE' GROUP BY `dc`.`id_menu` ;

--
-- Index pour les tables déchargées
--

--
-- Index pour la table `client`
--
ALTER TABLE `client`
  ADD PRIMARY KEY (`id_client`);

--
-- Index pour la table `commande`
--
ALTER TABLE `commande`
  ADD PRIMARY KEY (`id_commande`),
  ADD KEY `idx_commande_client` (`id_client`),
  ADD KEY `idx_commande_date` (`date_commande`),
  ADD KEY `idx_commande_statut` (`statut`);

--
-- Index pour la table `detail_commande`
--
ALTER TABLE `detail_commande`
  ADD PRIMARY KEY (`id_detail`),
  ADD KEY `idx_detail_commande` (`id_commande`),
  ADD KEY `idx_detail_menu` (`id_menu`);

--
-- Index pour la table `menu_jour`
--
ALTER TABLE `menu_jour`
  ADD PRIMARY KEY (`id_menu`),
  ADD UNIQUE KEY `uq_menu_plat_date` (`id_plat`,`date_menu`),
  ADD KEY `idx_menu_date` (`date_menu`),
  ADD KEY `idx_menu_plat` (`id_plat`);

--
-- Index pour la table `plat`
--
ALTER TABLE `plat`
  ADD PRIMARY KEY (`id_plat`);

--
-- AUTO_INCREMENT pour les tables déchargées
--

--
-- AUTO_INCREMENT pour la table `client`
--
ALTER TABLE `client`
  MODIFY `id_client` int(11) NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT pour la table `commande`
--
ALTER TABLE `commande`
  MODIFY `id_commande` int(11) NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT pour la table `detail_commande`
--
ALTER TABLE `detail_commande`
  MODIFY `id_detail` int(11) NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT pour la table `menu_jour`
--
ALTER TABLE `menu_jour`
  MODIFY `id_menu` int(11) NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT pour la table `plat`
--
ALTER TABLE `plat`
  MODIFY `id_plat` int(11) NOT NULL AUTO_INCREMENT;

--
-- Contraintes pour les tables déchargées
--

--
-- Contraintes pour la table `commande`
--
ALTER TABLE `commande`
  ADD CONSTRAINT `fk_commande_client` FOREIGN KEY (`id_client`) REFERENCES `client` (`id_client`);

--
-- Contraintes pour la table `detail_commande`
--
ALTER TABLE `detail_commande`
  ADD CONSTRAINT `fk_detail_commande` FOREIGN KEY (`id_commande`) REFERENCES `commande` (`id_commande`),
  ADD CONSTRAINT `fk_detail_menu` FOREIGN KEY (`id_menu`) REFERENCES `menu_jour` (`id_menu`);

--
-- Contraintes pour la table `menu_jour`
--
ALTER TABLE `menu_jour`
  ADD CONSTRAINT `fk_menu_plat` FOREIGN KEY (`id_plat`) REFERENCES `plat` (`id_plat`);
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
