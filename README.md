# Cantine Universitaire — Gestion des commandes

Application desktop **C++ / Qt Widgets / MySQL** pour gérer les clients, les plats, les menus
journaliers et les commandes d'une cantine universitaire.

- Interface moderne bleu foncé / blanc / vert (QSS), menu latéral et pages empilées (`QStackedWidget`).
- Toutes les requêtes SQL sont **préparées** (aucune concaténation de saisie utilisateur).
- Règle de stock garantie **côté C++ et côté MySQL** (contraintes, transactions, verrous).
- Montants affichés en **Ariary (Ar)**.

---

## 1. Contenu du livrable

```
CantineUniversitaire/
├── cantine_universitaire.pro      Projet qmake (Qt Widgets + SQL)
├── main.cpp                       Point d'entrée, chargement du thème QSS
├── mainwindow.*                   Fenêtre principale, navigation, page Paramètres
├── database.*                     Connexion MySQL (QMYSQL) et messages d'erreur
├── clientmanager.*                CRUD clients
├── platmanager.*                  CRUD plats
├── menumanager.*                  Menus du jour, doublons, disponibilité
├── commandemanager.*              Commandes, transactions, contrôle de stock
├── dashboard.*                    Tableau de bord (indicateurs)
├── clientspage.*                  Page Clients
├── platspage.*                    Page Plats
├── menupage.*                     Page Menus du jour
├── neworderpage.*                 Page Nouvelle commande
├── historypage.*                  Page Historique + détails/validation/annulation
├── utils.h                        Formatage des montants et des dates
├── qss/style.qss                  Thème (aussi embarqué via resources.qrc)
├── resources.qrc                  Ressources Qt
├── sql/cantine_universitaire.sql  Création complète de la base + vues
├── tests/test_metier.cpp|.pro     Tests automatisés de la règle métier
├── scripts/build_qmysql_plugin.ps1 Compilation/installation du pilote QMYSQL
├── build/release/                 Compilation qmake
└── deploy/                        Version autonome (DLL Qt + pilotes + libmysql.dll)
```

---

## 2. Prérequis

| Élément | Version conseillée |
|---|---|
| Windows | 10 / 11 64 bits |
| Qt | 6.5.3 MinGW 64 bits (Qt Creator + kit `Desktop Qt 6.5.3 MinGW 64-bit`) |
| Compilateur | MinGW 11.2.0 64 bits fourni avec Qt |
| MySQL | XAMPP (MariaDB/MySQL, port 3306) |
| Pilote Qt | `QMYSQL` (`qsqlmysql.dll`) + `libmysql.dll` |

> **Le pilote QMYSQL n'est pas fourni par défaut avec Qt.** Voir la section 4.

---

## 3. Base de données (XAMPP)

1. Lancer **XAMPP Control Panel** → démarrer **MySQL** (port 3306).
   En ligne de commande si besoin :
   ```powershell
   C:\xampp\mysql\bin\mysqld.exe --defaults-file=C:\xampp\mysql\bin\my.ini --standalone
   ```
2. Ouvrir **phpMyAdmin** (`http://localhost/phpmyadmin`) → onglet **Importer** →
   sélectionner `sql/cantine_universitaire.sql` → **Exécuter**.
   Ou en ligne de commande :
   ```powershell
   C:\xampp\mysql\bin\mysql.exe -u root < C:\CantineUniversitaire\sql\cantine_universitaire.sql
   ```

Le script :

- supprime puis recrée la base `cantine_universitaire` (UTF-8 `utf8mb4`) ;
- crée les tables **InnoDB** : `client`, `plat`, `menu_jour`, `commande`, `detail_commande` ;
- crée les contraintes : clés étrangères `RESTRICT` (historique préservé),
  `UNIQUE (id_plat, date_menu)`, `prix >= 0`, `quantite > 0`, statuts limités à
  `EN_ATTENTE` / `VALIDEE` / `ANNULEE` ;
- crée les vues `v_quantite_commandee_par_menu` et `v_menu_disponibilite`.

> ⚠️ Le script **efface** la base existante : à n'exécuter qu'à l'installation ou pour repartir de zéro.

---

## 4. Pilote QMYSQL

### 4.1 Vérifier sa présence

Lancer l'application : si le pilote manque, la page **Paramètres** affiche
« Le pilote QMYSQL n'est pas disponible » avec la liste des pilotes trouvés.

Vérification rapide en ligne de commande :

```powershell
C:\Qt\6.5.3\mingw_64\bin\qmake.exe -query QT_INSTALL_PLUGINS
Get-ChildItem C:\Qt\6.5.3\mingw_64\plugins\sqldrivers
```

`qsqlmysql.dll` doit être présent.

### 4.2 Compiler le pilote (si absent)

Le script fourni automatise l'opération (adapte les chemins si votre installation diffère) :

```powershell
powershell -ExecutionPolicy Bypass -File C:\CantineUniversitaire\scripts\build_qmysql_plugin.ps1
```

Il compile `qsqlmysql.dll` avec les sources du module SQL de Qt 6.5.3 et
**MySQL Connector/C 6.1.11**, puis l'installe dans :
`C:\Qt\6.5.3\mingw_64\plugins\sqldrivers\`.

Ensuite, copier `libmysql.dll` (issu de MySQL Connector/C 6.1.11) dans :
`C:\Qt\6.5.3\mingw_64\bin\`.

Redémarrer Qt Creator pour que le pilote soit pris en compte.

---

## 5. Compilation

### 5.1 Avec Qt Creator

1. **Fichier → Ouvrir un fichier ou un projet** → `cantine_universitaire.pro`.
2. Choisir le kit **Desktop Qt 6.5.3 MinGW 64-bit**.
3. **Projet → Exécuter qmake**, puis **Construire** (Ctrl+B).
4. **Exécuter** (Ctrl+R).

### 5.2 En ligne de commande

```powershell
$env:Path = "C:\Qt\6.5.3\mingw_64\bin;C:\Qt\Tools\mingw1120_64\bin;" + $env:Path
Set-Location C:\CantineUniversitaire\build
qmake ..\cantine_universitaire.pro -spec win32-g++ "CONFIG+=release"
mingw32-make -f Makefile.Release -j4
```

Binaire produit : `build\release\CantineUniversitaire.exe`.

---

## 6. Lancement et connexion

Au premier démarrage, l'application se connecte avec les paramètres par défaut :

| Paramètre | Valeur |
|---|---|
| Hôte | `127.0.0.1` |
| Port | `3306` |
| Base | `cantine_universitaire` |
| Utilisateur | `root` |
| Mot de passe | *(vide sous XAMPP par défaut)* |

Ces valeurs sont modifiables dans **Paramètres** (bouton **Tester** puis **Se connecter**).
**Tester** ouvre une connexion temporaire et ne touche pas à la connexion en cours :
seul **Se connecter** applique et enregistre les paramètres.
Elles sont mémorisées via `QSettings` :

```
HKEY_CURRENT_USER\Software\CantineUniversitaire\GestionCantine
```

Si la connexion échoue, l'application reste utilisable sur la page Paramètres et affiche
la cause exacte (MySQL arrêté, base absente, identifiants incorrects, pilote manquant).

---

## 7. Règles métier implémentées

### Clients / plats
- Nom obligatoire (non vide, espaces ignorés).
- Téléphone facultatif.
- Prix du plat >= 0 et <= 99 999 999,99 Ar (capacité `DECIMAL(10,2)` de la base).
- Suppression **refusée** si l'élément est référencé par un menu ou une commande
  (l'historique est conservé).
- Confirmation avant toute suppression.

### Menus du jour
- Un plat ne peut figurer qu'une seule fois par date
  (`UNIQUE (id_plat, date_menu)` + contrôle applicatif avec message explicite).
- Quantité initiale >= 0.
- Quantité **restante** = `quantite_initiale - somme(quantités des commandes VALIDEE)`.
- Les commandes `EN_ATTENTE` et `ANNULEE` ne réservent **aucun** stock.
- Modification refusée si la nouvelle quantité initiale est inférieure à la quantité déjà validée.

### Commandes
- Une commande comporte **au moins une ligne**, quantités entières > 0.
- Chaque plat commandé doit appartenir au **menu de la date choisie**.
- **Prix unitaire figé** depuis la base au moment de la commande (pas depuis l'interface) :
  modifier le prix d'un plat ne change pas l'historique.
- Total recalculé côté serveur (`SUM(quantite * prix_unitaire)`), jamais fourni par l'interface.
- **Aucune commande partielle** : si une ligne est en rupture, toute l'opération est annulée.
- **Recherche dans l'historique** : filtrage automatique dès la saisie (pause de 250 ms),
  touche Entrée ou bouton Rechercher. Chaque mot est un critère ; plusieurs mots se
  combinent (ET) : `Fenitra 23/10/2026`.
  - `5` ou `#5` : n° de commande exact (comparaison numérique, donc `5` ne renvoie
    pas la commande 15) ;
  - nom du client : partiel, insensible à la casse et aux accents (`fenitra`) ;
  - date : `jj/mm/aaaa`, `aaaa-mm-jj`, `jj-mm-aaaa` ou `jj.mm.aaaa` ;
  - 4 chiffres et plus : téléphone ou année (`2026`).
  Champ vide = toutes les commandes ; sans résultat, le message
  « Aucune commande trouvée » s'affiche au-dessus du tableau.

Déroulement d'une validation (`commandemanager.cpp`) :

```
SET TRANSACTION ISOLATION LEVEL READ COMMITTED
START TRANSACTION
  SELECT ... FROM menu_jour WHERE id_menu IN (...) ORDER BY id_menu FOR UPDATE
  vérification : appartenance au menu de la date + quantités > 0 + stock restant suffisant
  INSERT INTO commande (statut = 'VALIDEE' ou 'EN_ATTENTE')
  INSERT INTO detail_commande (prix_unitaire lu en base)
COMMIT            -- ou ROLLBACK en cas d'erreur
```

- `FOR UPDATE` verrouille les lignes de menu : deux caisses simultanées ne peuvent pas
  survendre le même plat.
- Les menus sont verrouillés **dans l'ordre croissant de `id_menu`** pour éviter les interblocages.
- `READ COMMITTED` évite de relire un stock obsolète sous le niveau `REPEATABLE READ` par défaut.

### Statuts
| Statut | Effet sur le stock |
|---|---|
| `EN_ATTENTE` | aucune réservation |
| `VALIDEE` | réserve les quantités |
| `ANNULEE` | ne réserve plus (stock libéré) |

L'annulation est confirmée par une boîte de dialogue ; une commande déjà `ANNULEE`
ne peut pas être ré-annulée, et une commande `VALIDEE` ne redevient jamais `EN_ATTENTE`.

---

## 8. Guide de test (scénario complet)

> Base vierge : réimporter `sql/cantine_universitaire.sql` avant de commencer.

1. **Connexion** — ouvrir l'application : la barre d'état indique « Connecté à MySQL ».
2. **Clients** — ajouter `Rakoto Jean` / `034 12 345 67`.
   Vérifier qu'un nom vide est refusé.
3. **Plats** — ajouter `Riz au poulet` à `8 000 Ar` et `Haricots` à `3 000 Ar`.
4. **Menus du jour** — pour la date du jour, ajouter `Riz au poulet` (qté 5) et `Haricots` (qté 10).
   - Réessayer d'ajouter `Riz au poulet` à la même date → message « déjà présent ».
   - Vérifier que la quantité restante = quantité initiale.
5. **Nouvelle commande**
   - Choisir `Rakoto Jean` et la date du jour.
   - Ajouter 2 × `Riz au poulet` et 3 × `Haricots` → total `25 000 Ar`.
   - Valider → statut `VALIDEE`.
6. **Stock** — retourner aux Menus : restant = 3 pour `Riz au poulet`, 7 pour `Haricots`.
7. **Dépassement** — commander 4 × `Riz au poulet` → refus explicite
   (« stock insuffisant : 3 restant(s) »), **aucune** commande créée.
8. **Multi-lignes** — commander 1 × `Riz au poulet` + 100 × `Haricots` → refus global,
   vérifier dans l'historique qu'aucune ligne n'a été enregistrée (rollback complet).
9. **Historique** — taper un nom, un téléphone, une date ou un n° de commande :
   la liste se filtre automatiquement pendant la saisie (ou avec Entrée / Rechercher).
   Tester `fenitra`, `23/10/2026`, `5` et un terme inexistant : le message
   « Aucune commande trouvée » doit apparaître ; vider le champ pour tout réafficher.
   Ouvrir le détail d'une commande (lignes, prix unitaires, total).
10. **Annulation** — annuler la commande de l'étape 5 → statut `ANNULEE`, et le stock
    remonte à 5 / 10.
11. **Prix historisé** — modifier le prix de `Riz au poulet` (ex. `9 500 Ar`) :
    l'ancienne commande conserve `8 000 Ar`.
12. **Suppressions** — tenter de supprimer `Riz au poulet` : refusé car référencé.
    Supprimer un plat jamais utilisé : accepté après confirmation.
13. **Tableau de bord** — le nombre de commandes validées du jour, le chiffre d'affaires
    du jour, les commandes récentes et les plats à stock faible sont cohérents avec les
    étapes précédentes.

### Tests automatisés

Ils vérifient la règle métier directement contre MySQL (stock, refus, rollback,
prix historisé, doublons, annulation, contraintes de suppression) :

```powershell
$env:Path = "C:\Qt\6.5.3\mingw_64\bin;C:\Qt\Tools\mingw1120_64\bin;" + $env:Path
Set-Location C:\CantineUniversitaire\tests\build
qmake ..\test_metier.pro -spec win32-g++ "CONFIG+=release"
mingw32-make -f Makefile.Release -j4
.\release\test_metier.exe
```

Résultat attendu : une suite de `[OK]` puis `TOUS LES TESTS SONT PASSÉS`.
Réimporter le script SQL ensuite pour retrouver une base propre.

---

## 9. Déploiement autonome

Le dossier `deploy/` contient une version exécutable sans Qt Creator :

```
deploy/
├── CantineUniversitaire.exe
├── Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll, Qt6Sql.dll, Qt6Network.dll
├── libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll
├── libmysql.dll
├── platforms/qwindows.dll
├── styles/qwindowsvistastyle.dll
├── sqldrivers/qsqlmysql.dll, qsqlite.dll
├── imageformats/…
├── qss/style.qss
└── sql/cantine_universitaire.sql
```

Lancer `deploy\CantineUniversitaire.exe` : MySQL (XAMPP) doit être démarré.
`windeployqt` peut être utilisé pour régénérer les DLL Qt, mais **il ne copie pas**
`qsqlmysql.dll` ni `libmysql.dll` : les copier manuellement comme ci-dessus.

---

## 10. Dépannage

| Symptôme | Cause probable | Solution |
|---|---|---|
| « Le pilote QMYSQL n'est pas disponible » | plugin absent ou DLL MySQL manquante | section 4 ; vérifier `plugins\sqldrivers\qsqlmysql.dll` et `libmysql.dll` dans `bin\` |
| « Échec de connexion à MySQL » | MySQL arrêté / base absente / identifiants | démarrer MySQL dans XAMPP, importer le script, vérifier Paramètres |
| `libmysql.dll` introuvable au lancement | DLL absente du dossier de l'exe | copier `libmysql.dll` à côté de l'exécutable |
| Erreur `mysql_native_password` / `caching_sha2_password` | pilote compilé avec un connecteur trop ancien | recompiler QMYSQL avec Connector/C 6.1.11 (script fourni) |
| Quantité restante incohérente | commandes de test laissées en base | réimporter `sql/cantine_universitaire.sql` |
| « Duplicate entry » sur un menu | plat déjà programmé à cette date | modifier le menu existant au lieu d'en créer un second |
| Suppression refusée | élément utilisé par un menu/commande | conserver l'historique : modifier au lieu de supprimer |
| L'application ne se lance pas (aucune fenêtre) | DLL Qt manquante | utiliser `deploy/` ou ajouter `C:\Qt\6.5.3\mingw_64\bin` au PATH |
| Apache XAMPP ne démarre pas, « Port 443 in use » | VMware Workstation (`vmware-hostd.exe`) occupe le 443 | dans `C:\xampp\apache\conf\extra\httpd-ssl.conf`, remplacer `443` par `4433` (`Listen`, `VirtualHost`, `ServerName`) puis démarrer Apache. phpMyAdmin reste sur `http://localhost/phpmyadmin` |

---

## 11. Notes techniques

- **Requêtes préparées** partout : `QSqlQuery::prepare()` + `addBindValue()`.
  Les rares `QString::arg()` présents dans le code ne servent qu'à des messages
  d'interface ou à des noms de tables internes constants.
- **Transactions** : `START TRANSACTION` / `COMMIT` / `ROLLBACK` avec vérification
  du retour de chaque `exec()` ; toute erreur déclenche un `ROLLBACK` et un message
  explicite, jamais une commande partielle.
- **Verrous** : `SELECT … FOR UPDATE` sur `menu_jour`, tri par `id_menu`.
- **Historique** : clés étrangères `ON DELETE RESTRICT` + `prix_unitaire` stocké
  dans `detail_commande`.
- **Disponibilité** : vues `v_quantite_commandee_par_menu` (somme des quantités
  `VALIDEE` par menu) et `v_menu_disponibilite` (initiale − commandée).
- **Thème** : feuille QSS embarquée dans les ressources (`:/qss/style.qss`) avec
  repli sur `qss/style.qss` à côté de l'exécutable.
- **Test de connexion isolé** : `Database::testerParametres()` ouvre une connexion
  temporaire (`cantine_test_N`) puis la supprime ; une configuration erronée ne coupe
  donc jamais la connexion active.
- **Modifications sans changement** : MySQL renvoyant 0 ligne affectée lorsque les
  valeurs sont identiques, les managers vérifient l'existence de l'enregistrement
  pour ne pas afficher une fausse erreur « introuvable ».
- **Tableau de bord** : les indicateurs « commandes validées » et « chiffre d'affaires »
  sont calculés pour la journée en cours.
