#include "commandemanager.h"
#include "database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QSqlDatabase>
#include <QMap>
#include <QRegularExpression>
#include <QStringList>

namespace {

void setError(QString *error, const QString &message)
{
    if (error) {
        *error = message;
    }
}

// Neutralise les caractères spéciaux de LIKE pour qu'ils soient recherchés
// littéralement (le caractère d'échappement explicite est « ! »).
QString echapperLike(QString valeur)
{
    valeur.replace(QLatin1Char('!'), QStringLiteral("!!"));
    valeur.replace(QLatin1Char('%'), QStringLiteral("!%"));
    valeur.replace(QLatin1Char('_'), QStringLiteral("!_"));
    return valeur;
}

bool demarrerTransaction(QString *error)
{
    QSqlQuery query(Database::instance()->getDatabase());
    // Niveau READ COMMITTED : après l'obtention des verrous, les quantités
    // déjà validées par une autre transaction sont visibles.
    if (!query.exec(QStringLiteral("SET TRANSACTION ISOLATION LEVEL READ COMMITTED"))) {
        setError(error, QStringLiteral("Impossible de configurer la transaction : %1")
                            .arg(query.lastError().text()));
        return false;
    }
    if (!query.exec(QStringLiteral("START TRANSACTION"))) {
        setError(error, QStringLiteral("Impossible de démarrer la transaction : %1")
                            .arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool validerTransaction(QString *error)
{
    QSqlQuery query(Database::instance()->getDatabase());
    if (!query.exec(QStringLiteral("COMMIT"))) {
        setError(error, QStringLiteral("Impossible de valider la transaction : %1")
                            .arg(query.lastError().text()));
        QSqlQuery rollback(Database::instance()->getDatabase());
        rollback.exec(QStringLiteral("ROLLBACK"));
        return false;
    }
    return true;
}

void annulerTransaction()
{
    QSqlQuery query(Database::instance()->getDatabase());
    query.exec(QStringLiteral("ROLLBACK"));
}

} // namespace

CommandeManager::CommandeManager(QObject *parent)
    : QObject(parent)
{
}

int CommandeManager::enregistrerCommande(int idClient,
                                         const QDateTime &dateCommande,
                                         const QList<LigneCommande> &lignes,
                                         bool valider,
                                         QString *error)
{
    if (!Database::instance()->isConnected()) {
        setError(error, QStringLiteral("Aucune connexion active à la base de données."));
        return -1;
    }
    if (idClient <= 0) {
        setError(error, QStringLiteral("Sélectionnez un client."));
        return -1;
    }
    if (!dateCommande.isValid()) {
        setError(error, QStringLiteral("La date de la commande est invalide."));
        return -1;
    }
    if (lignes.isEmpty()) {
        setError(error, QStringLiteral("Ajoutez au moins une ligne à la commande."));
        return -1;
    }

    for (const LigneCommande &ligne : lignes) {
        if (ligne.quantite <= 0) {
            setError(error, QStringLiteral(
                "La quantité doit être strictement positive pour chaque plat."));
            return -1;
        }
    }

    if (!demarrerTransaction(error)) {
        return -1;
    }

    QSqlDatabase db = Database::instance()->getDatabase();

    // Vérifier que le client existe.
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("SELECT id_client FROM client WHERE id_client = ?"));
        query.addBindValue(idClient);
        if (!query.exec()) {
            annulerTransaction();
            setError(error, QStringLiteral("Erreur SQL : %1").arg(query.lastError().text()));
            return -1;
        }
        if (!query.next()) {
            annulerTransaction();
            setError(error, QStringLiteral("Le client sélectionné n'existe plus."));
            return -1;
        }
    }

    // Insérer la commande (toujours en attente au départ).
    int idCommande = 0;
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "INSERT INTO commande (id_client, date_commande, statut) "
            "VALUES (?, ?, 'EN_ATTENTE')"));
        query.addBindValue(idClient);
        query.addBindValue(dateCommande.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        if (!query.exec()) {
            annulerTransaction();
            setError(error, QStringLiteral("Impossible de créer la commande : %1")
                                .arg(query.lastError().text()));
            return -1;
        }
        idCommande = query.lastInsertId().toInt();
    }

    // Regrouper les lignes par menu (l'interface fusionne déjà les doublons,
    // mais l'API doit rester fiable) puis verrouiller les menus dans l'ordre
    // croissant de leur identifiant pour éviter les interblocages.
    QMap<int, int> quantitesParMenu;
    for (const LigneCommande &ligne : lignes) {
        if (ligne.idMenu <= 0) {
            annulerTransaction();
            setError(error, QStringLiteral("Ligne de commande invalide (menu manquant)."));
            return -1;
        }
        quantitesParMenu[ligne.idMenu] += ligne.quantite;
    }

    for (auto it = quantitesParMenu.cbegin(); it != quantitesParMenu.cend(); ++it) {
        const int idMenu = it.key();
        const int quantite = it.value();

        QSqlQuery menuQuery(db);
        menuQuery.prepare(QStringLiteral(
            "SELECT m.date_menu, p.prix, p.nom_plat "
            "FROM menu_jour m "
            "INNER JOIN plat p ON p.id_plat = m.id_plat "
            "WHERE m.id_menu = ? FOR UPDATE"));
        menuQuery.addBindValue(idMenu);

        if (!menuQuery.exec()) {
            annulerTransaction();
            setError(error, QStringLiteral("Erreur SQL lors du verrouillage du menu : %1")
                                .arg(menuQuery.lastError().text()));
            return -1;
        }
        if (!menuQuery.next()) {
            annulerTransaction();
            setError(error, QStringLiteral("Le menu sélectionné n'existe plus."));
            return -1;
        }

        const QDate dateMenu = menuQuery.value(0).toDate();
        const double prixActuel = menuQuery.value(1).toDouble();
        const QString nomPlat = menuQuery.value(2).toString();

        if (dateMenu != dateCommande.date()) {
            annulerTransaction();
            setError(error, QStringLiteral(
                "Commande refusée : le plat « %1 » n'appartient pas au menu du %2.")
                .arg(nomPlat, dateCommande.date().toString(QStringLiteral("dd/MM/yyyy"))));
            return -1;
        }

        QSqlQuery detailQuery(db);
        detailQuery.prepare(QStringLiteral(
            "INSERT INTO detail_commande (id_commande, id_menu, quantite, prix_unitaire) "
            "VALUES (?, ?, ?, ?)"));
        detailQuery.addBindValue(idCommande);
        detailQuery.addBindValue(idMenu);
        detailQuery.addBindValue(quantite);
        // Le prix unitaire est enregistré dans le détail pour conserver l'historique.
        detailQuery.addBindValue(prixActuel);

        if (!detailQuery.exec()) {
            annulerTransaction();
            setError(error, QStringLiteral("Impossible d'ajouter une ligne de commande : %1")
                                .arg(detailQuery.lastError().text()));
            return -1;
        }
    }

    if (valider) {
        if (!validerInterne(idCommande, error)) {
            annulerTransaction();
            return -1;
        }
    }

    if (!validerTransaction(error)) {
        return -1;
    }
    return idCommande;
}

bool CommandeManager::validerInterne(int idCommande, QString *error)
{
    QSqlDatabase db = Database::instance()->getDatabase();

    QDate dateCommande;
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "SELECT DATE(date_commande), statut FROM commande WHERE id_commande = ? FOR UPDATE"));
        query.addBindValue(idCommande);
        if (!query.exec() || !query.next()) {
            setError(error, QStringLiteral("Commande introuvable."));
            return false;
        }
        dateCommande = query.value(0).toDate();
        const QString statut = query.value(1).toString();
        if (statut != QStringLiteral("EN_ATTENTE")) {
            setError(error, QStringLiteral(
                "Seules les commandes en attente peuvent être validées (statut actuel : %1).")
                .arg(statut));
            return false;
        }
    }

    QList<int> menus;
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "SELECT DISTINCT id_menu FROM detail_commande WHERE id_commande = ? ORDER BY id_menu"));
        query.addBindValue(idCommande);
        if (!query.exec()) {
            setError(error, QStringLiteral("Erreur SQL : %1").arg(query.lastError().text()));
            return false;
        }
        while (query.next()) {
            menus.append(query.value(0).toInt());
        }
    }

    if (menus.isEmpty()) {
        setError(error, QStringLiteral("La commande ne contient aucune ligne."));
        return false;
    }

    for (int idMenu : menus) {
        QSqlQuery menuQuery(db);
        menuQuery.prepare(QStringLiteral(
            "SELECT m.date_menu, m.quantite_initiale, p.nom_plat "
            "FROM menu_jour m "
            "INNER JOIN plat p ON p.id_plat = m.id_plat "
            "WHERE m.id_menu = ? FOR UPDATE"));
        menuQuery.addBindValue(idMenu);

        if (!menuQuery.exec()) {
            setError(error, QStringLiteral("Erreur SQL lors du verrouillage du menu : %1")
                                .arg(menuQuery.lastError().text()));
            return false;
        }
        if (!menuQuery.next()) {
            setError(error, QStringLiteral("Un plat de la commande n'existe plus dans les menus."));
            return false;
        }

        const QDate dateMenu = menuQuery.value(0).toDate();
        const int quantiteInitiale = menuQuery.value(1).toInt();
        const QString nomPlat = menuQuery.value(2).toString();

        if (dateMenu != dateCommande) {
            setError(error, QStringLiteral(
                "Commande refusée : le plat « %1 » n'appartient pas au menu du %2.")
                .arg(nomPlat, dateCommande.toString(QStringLiteral("dd/MM/yyyy"))));
            return false;
        }

        int quantiteDemandee = 0;
        {
            QSqlQuery query(db);
            query.prepare(QStringLiteral(
                "SELECT COALESCE(SUM(quantite), 0) FROM detail_commande "
                "WHERE id_commande = ? AND id_menu = ?"));
            query.addBindValue(idCommande);
            query.addBindValue(idMenu);
            if (!query.exec() || !query.next()) {
                setError(error, QStringLiteral("Erreur SQL : %1").arg(query.lastError().text()));
                return false;
            }
            quantiteDemandee = query.value(0).toInt();
        }

        int quantiteDejaValidee = 0;
        {
            QSqlQuery query(db);
            query.prepare(QStringLiteral(
                "SELECT COALESCE(SUM(dc.quantite), 0) "
                "FROM detail_commande dc "
                "INNER JOIN commande c ON c.id_commande = dc.id_commande "
                "WHERE dc.id_menu = ? AND c.statut = 'VALIDEE'"));
            query.addBindValue(idMenu);
            if (!query.exec() || !query.next()) {
                setError(error, QStringLiteral("Erreur SQL : %1").arg(query.lastError().text()));
                return false;
            }
            quantiteDejaValidee = query.value(0).toInt();
        }

        const int quantiteRestante = quantiteInitiale - quantiteDejaValidee;
        if (quantiteDemandee > quantiteRestante) {
            setError(error, QStringLiteral(
                "Commande refusée : quantité insuffisante pour le plat « %1 ».\n"
                "Quantité demandée : %2 — quantité restante : %3.")
                .arg(nomPlat).arg(quantiteDemandee).arg(qMax(0, quantiteRestante)));
            return false;
        }
    }

    QSqlQuery updateQuery(db);
    updateQuery.prepare(QStringLiteral(
        "UPDATE commande SET statut = 'VALIDEE' WHERE id_commande = ?"));
    updateQuery.addBindValue(idCommande);
    if (!updateQuery.exec()) {
        setError(error, QStringLiteral("Impossible de valider la commande : %1")
                            .arg(updateQuery.lastError().text()));
        return false;
    }
    return true;
}

bool CommandeManager::validerCommande(int idCommande, QString *error)
{
    if (!Database::instance()->isConnected()) {
        setError(error, QStringLiteral("Aucune connexion active à la base de données."));
        return false;
    }
    if (idCommande <= 0) {
        setError(error, QStringLiteral("Aucune commande sélectionnée."));
        return false;
    }

    if (!demarrerTransaction(error)) {
        return false;
    }

    if (!validerInterne(idCommande, error)) {
        annulerTransaction();
        return false;
    }

    return validerTransaction(error);
}

bool CommandeManager::annulerCommande(int idCommande, QString *error)
{
    if (!Database::instance()->isConnected()) {
        setError(error, QStringLiteral("Aucune connexion active à la base de données."));
        return false;
    }
    if (idCommande <= 0) {
        setError(error, QStringLiteral("Aucune commande sélectionnée."));
        return false;
    }

    if (!demarrerTransaction(error)) {
        return false;
    }

    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral(
        "UPDATE commande SET statut = 'ANNULEE' "
        "WHERE id_commande = ? AND statut IN ('EN_ATTENTE', 'VALIDEE')"));
    query.addBindValue(idCommande);

    if (!query.exec()) {
        annulerTransaction();
        setError(error, QStringLiteral("Impossible d'annuler la commande : %1")
                            .arg(query.lastError().text()));
        return false;
    }
    if (query.numRowsAffected() == 0) {
        annulerTransaction();
        setError(error, QStringLiteral(
            "Cette commande ne peut pas être annulée (déjà annulée ou introuvable).\n"
            "Une commande annulée ne réserve plus de quantité."));
        return false;
    }

    return validerTransaction(error);
}

QList<CommandeData> CommandeManager::getHistorique(const QString &recherche,
                                                   const QString &statut,
                                                   QString *error) const
{
    QList<CommandeData> result;

    QString sql = QStringLiteral(
        "SELECT c.id_commande, c.id_client, cl.nom, c.date_commande, c.statut, "
        "       COALESCE(SUM(dc.quantite * dc.prix_unitaire), 0) AS montant, "
        "       COUNT(dc.id_detail) AS nb_lignes "
        "FROM commande c "
        "INNER JOIN client cl ON cl.id_client = c.id_client "
        "LEFT JOIN detail_commande dc ON dc.id_commande = c.id_commande "
        "WHERE 1 = 1 ");

    QStringList conditions;
    QVariantList params;

    // Chaque mot saisi doit correspondre à au moins un champ : « Fenitra 23/10 »
    // fonctionne donc comme un filtre combiné.
    const QString term = recherche.trimmed();
    if (!term.isEmpty()) {
        const QStringList mots = term.split(QRegularExpression(QStringLiteral("\\s+")),
                                            Qt::SkipEmptyParts);
        for (QString mot : mots) {
            // « #12 » ne recherche que par identifiant exact.
            const bool idSeul = mot.startsWith(QLatin1Char('#'));
            if (idSeul) {
                mot.remove(0, 1);
            }
            mot = mot.trimmed();
            if (mot.isEmpty()) {
                continue;
            }

            QStringList champs;
            QVariantList valeurs;

            // id_commande est un INT : on le compare numériquement (et non avec
            // un CAST ... LIKE, qui ferait aussi correspondre 15 pour « 5 »).
            bool estEntier = false;
            const int idMot = mot.toInt(&estEntier);
            const bool numerique = estEntier && idMot > 0;
            const bool commenceParChiffre = !mot.isEmpty() && mot.at(0).isDigit();
            const bool formatDate = commenceParChiffre
                                    && (mot.contains(QLatin1Char('/'))
                                        || mot.contains(QLatin1Char('-'))
                                        || mot.contains(QLatin1Char('.')));

            const QString pattern = QStringLiteral("%") + echapperLike(mot)
                                    + QStringLiteral("%");

            if (idSeul) {
                // « #5 » cible uniquement la commande n° 5.
                if (!numerique) {
                    continue;
                }
                champs << QStringLiteral("c.id_commande = ?");
                valeurs << idMot;
            } else if (numerique && mot.size() <= 3) {
                // « 5 » cible la commande n° 5 (sans confondre avec un
                // téléphone ou une date contenant 5).
                champs << QStringLiteral("c.id_commande = ?");
                valeurs << idMot;
            } else if (formatDate) {
                // date_commande est un DATETIME : on compare les formats usuels.
                champs << QStringLiteral(
                    "DATE_FORMAT(c.date_commande, '%d/%m/%Y') LIKE ? ESCAPE '!'");
                valeurs << pattern;
                champs << QStringLiteral(
                    "DATE_FORMAT(c.date_commande, '%Y-%m-%d') LIKE ? ESCAPE '!'");
                valeurs << pattern;
                champs << QStringLiteral(
                    "DATE_FORMAT(c.date_commande, '%d-%m-%Y') LIKE ? ESCAPE '!'");
                valeurs << pattern;
                champs << QStringLiteral(
                    "DATE_FORMAT(c.date_commande, '%d.%m.%Y') LIKE ? ESCAPE '!'");
                valeurs << pattern;
            } else if (numerique) {
                // 4 chiffres et plus : téléphone ou année (« 2026 »).
                champs << QStringLiteral("COALESCE(cl.telephone, '') LIKE ? ESCAPE '!'");
                valeurs << pattern;
                champs << QStringLiteral(
                    "DATE_FORMAT(c.date_commande, '%d/%m/%Y') LIKE ? ESCAPE '!'");
                valeurs << pattern;
                champs << QStringLiteral(
                    "DATE_FORMAT(c.date_commande, '%Y-%m-%d') LIKE ? ESCAPE '!'");
                valeurs << pattern;
                champs << QStringLiteral(
                    "DATE_FORMAT(c.date_commande, '%d-%m-%Y') LIKE ? ESCAPE '!'");
                valeurs << pattern;
                champs << QStringLiteral(
                    "DATE_FORMAT(c.date_commande, '%d.%m.%Y') LIKE ? ESCAPE '!'");
                valeurs << pattern;
            } else {
                // Nom : collation utf8mb4_unicode_ci de la colonne,
                // donc insensible à la casse et aux accents.
                champs << QStringLiteral("cl.nom LIKE ? ESCAPE '!'");
                valeurs << pattern;
            }

            if (champs.isEmpty()) {
                continue;
            }

            conditions << QStringLiteral("(") + champs.join(QStringLiteral(" OR "))
                              + QStringLiteral(")");
            params += valeurs;
        }
    }

    const QString statutFiltre = statut.trimmed();
    if (!statutFiltre.isEmpty() && statutFiltre != QStringLiteral("TOUS")) {
        conditions << QStringLiteral("c.statut = ?");
        params << statutFiltre;
    }

    if (!conditions.isEmpty()) {
        sql += QStringLiteral("AND ") + conditions.join(QStringLiteral(" AND "))
               + QStringLiteral(" ");
    }

    sql += QStringLiteral(
        "GROUP BY c.id_commande, c.id_client, cl.nom, c.date_commande, c.statut "
        "ORDER BY c.date_commande DESC, c.id_commande DESC");

    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(sql);
    for (const QVariant &value : params) {
        query.addBindValue(value);
    }

    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de charger les commandes : %1")
                            .arg(query.lastError().text()));
        return result;
    }

    while (query.next()) {
        CommandeData commande;
        commande.id = query.value(0).toInt();
        commande.idClient = query.value(1).toInt();
        commande.nomClient = query.value(2).toString();
        commande.dateCommande = query.value(3).toDateTime();
        commande.statut = query.value(4).toString();
        commande.montantTotal = query.value(5).toDouble();
        commande.nbLignes = query.value(6).toInt();
        result.append(commande);
    }
    return result;
}

QList<LigneCommande> CommandeManager::getDetails(int idCommande, QString *error) const
{
    QList<LigneCommande> result;

    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral(
        "SELECT dc.id_menu, m.id_plat, p.nom_plat, dc.quantite, dc.prix_unitaire "
        "FROM detail_commande dc "
        "INNER JOIN menu_jour m ON m.id_menu = dc.id_menu "
        "INNER JOIN plat p ON p.id_plat = m.id_plat "
        "WHERE dc.id_commande = ? "
        "ORDER BY p.nom_plat ASC"));
    query.addBindValue(idCommande);

    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de charger les détails : %1")
                            .arg(query.lastError().text()));
        return result;
    }

    while (query.next()) {
        LigneCommande ligne;
        ligne.idMenu = query.value(0).toInt();
        ligne.idPlat = query.value(1).toInt();
        ligne.nomPlat = query.value(2).toString();
        ligne.quantite = query.value(3).toInt();
        ligne.prixUnitaire = query.value(4).toDouble();
        ligne.sousTotal = ligne.quantite * ligne.prixUnitaire;
        result.append(ligne);
    }
    return result;
}

int CommandeManager::compterValidees(const QDate &jour, QString *error) const
{
    QSqlQuery query(Database::instance()->getDatabase());
    if (jour.isValid()) {
        query.prepare(QStringLiteral(
            "SELECT COUNT(*) FROM commande "
            "WHERE statut = 'VALIDEE' AND DATE(date_commande) = ?"));
        query.addBindValue(jour.toString(QStringLiteral("yyyy-MM-dd")));
        if (!query.exec() || !query.next()) {
            setError(error, QStringLiteral("Impossible de compter les commandes validées : %1")
                                .arg(query.lastError().text()));
            return 0;
        }
        return query.value(0).toInt();
    }

    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM commande WHERE statut = 'VALIDEE'"))
        || !query.next()) {
        setError(error, QStringLiteral("Impossible de compter les commandes validées : %1")
                            .arg(query.lastError().text()));
        return 0;
    }
    return query.value(0).toInt();
}

double CommandeManager::chiffreAffaires(const QDate &jour, QString *error) const
{
    QSqlQuery query(Database::instance()->getDatabase());
    if (jour.isValid()) {
        query.prepare(QStringLiteral(
            "SELECT COALESCE(SUM(dc.quantite * dc.prix_unitaire), 0) "
            "FROM detail_commande dc "
            "INNER JOIN commande c ON c.id_commande = dc.id_commande "
            "WHERE c.statut = 'VALIDEE' AND DATE(c.date_commande) = ?"));
        query.addBindValue(jour.toString(QStringLiteral("yyyy-MM-dd")));
        if (!query.exec() || !query.next()) {
            setError(error, QStringLiteral("Impossible de calculer le chiffre d'affaires : %1")
                                .arg(query.lastError().text()));
            return 0.0;
        }
        return query.value(0).toDouble();
    }

    if (!query.exec(QStringLiteral(
            "SELECT COALESCE(SUM(dc.quantite * dc.prix_unitaire), 0) "
            "FROM detail_commande dc "
            "INNER JOIN commande c ON c.id_commande = dc.id_commande "
            "WHERE c.statut = 'VALIDEE'"))
        || !query.next()) {
        setError(error, QStringLiteral("Impossible de calculer le chiffre d'affaires : %1")
                            .arg(query.lastError().text()));
        return 0.0;
    }
    return query.value(0).toDouble();
}

double CommandeManager::montantCommande(int idCommande, QString *error) const
{
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(quantite * prix_unitaire), 0) "
        "FROM detail_commande WHERE id_commande = ?"));
    query.addBindValue(idCommande);
    if (!query.exec() || !query.next()) {
        setError(error, QStringLiteral("Impossible de calculer le montant de la commande : %1")
                            .arg(query.lastError().text()));
        return -1.0;
    }
    return query.value(0).toDouble();
}

QList<CommandeData> CommandeManager::getRecentes(int limite, QString *error) const
{
    QList<CommandeData> result;

    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral(
        "SELECT c.id_commande, c.id_client, cl.nom, c.date_commande, c.statut, "
        "       COALESCE(SUM(dc.quantite * dc.prix_unitaire), 0) AS montant, "
        "       COUNT(dc.id_detail) AS nb_lignes "
        "FROM commande c "
        "INNER JOIN client cl ON cl.id_client = c.id_client "
        "LEFT JOIN detail_commande dc ON dc.id_commande = c.id_commande "
        "GROUP BY c.id_commande, c.id_client, cl.nom, c.date_commande, c.statut "
        "ORDER BY c.date_commande DESC, c.id_commande DESC "
        "LIMIT ?"));
    query.addBindValue(qMax(1, limite));

    if (!query.exec()) {
        setError(error, QStringLiteral("Impossible de charger les commandes récentes : %1")
                            .arg(query.lastError().text()));
        return result;
    }

    while (query.next()) {
        CommandeData commande;
        commande.id = query.value(0).toInt();
        commande.idClient = query.value(1).toInt();
        commande.nomClient = query.value(2).toString();
        commande.dateCommande = query.value(3).toDateTime();
        commande.statut = query.value(4).toString();
        commande.montantTotal = query.value(5).toDouble();
        commande.nbLignes = query.value(6).toInt();
        result.append(commande);
    }
    return result;
}
