#include <QCoreApplication>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QTextStream>
#include <QDate>
#include <cstdio>

#include "database.h"
#include "clientmanager.h"
#include "platmanager.h"
#include "menumanager.h"
#include "commandemanager.h"

namespace {

int echecs = 0;
QTextStream sortie(stdout);

void verifier(bool condition, const QString &description)
{
    if (condition) {
        sortie << "[OK]   " << description << "\n";
    } else {
        sortie << "[ECHEC] " << description << "\n";
        ++echecs;
    }
    sortie.flush();
}

int dernierId(const QString &table, const QString &colonne)
{
    QSqlQuery query(Database::instance()->getDatabase());
    if (!query.exec(QStringLiteral("SELECT MAX(%1) FROM %2").arg(colonne, table))
        || !query.next()) {
        return 0;
    }
    return query.value(0).toInt();
}

QString statutCommande(int idCommande)
{
    QSqlQuery query(Database::instance()->getDatabase());
    query.prepare(QStringLiteral("SELECT statut FROM commande WHERE id_commande = ?"));
    query.addBindValue(idCommande);
    if (!query.exec() || !query.next()) {
        return QString();
    }
    return query.value(0).toString();
}

int restanteMenu(int idMenu)
{
    MenuManager manager;
    const QList<MenuData> menus = manager.getMenusParDate(QDate::currentDate());
    for (const MenuData &menu : menus) {
        if (menu.idMenu == idMenu) {
            return menu.quantiteRestante;
        }
    }
    return -1;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    sortie << "=== Tests métier — Cantine Universitaire ===\n";

    if (!Database::instance()->connectToDatabase("127.0.0.1", 3306,
                                                 "cantine_universitaire", "root", "")) {
        sortie << "Connexion impossible : " << Database::instance()->lastError() << "\n";
        return 2;
    }

    // Base de test propre
    {
        QSqlQuery query(Database::instance()->getDatabase());
        query.exec(QStringLiteral("SET FOREIGN_KEY_CHECKS = 0"));
        for (const QString &table : {QStringLiteral("detail_commande"), QStringLiteral("commande"),
                                     QStringLiteral("menu_jour"), QStringLiteral("plat"),
                                     QStringLiteral("client")}) {
            query.exec(QStringLiteral("DELETE FROM %1").arg(table));
        }
        query.exec(QStringLiteral("SET FOREIGN_KEY_CHECKS = 1"));
    }

    ClientManager clients;
    PlatManager plats;
    MenuManager menus;
    CommandeManager commandes;

    QString erreur;

    verifier(clients.ajouter(QStringLiteral("Étudiant Test"), QStringLiteral("0340000000"), &erreur),
             QStringLiteral("Ajout d'un client"));
    const int idClient = dernierId(QStringLiteral("client"), QStringLiteral("id_client"));

    verifier(plats.ajouter(QStringLiteral("Riz Poulet"), 5000.0, &erreur),
             QStringLiteral("Ajout d'un plat (prix 5 000 Ar)"));
    const int idPlat = dernierId(QStringLiteral("plat"), QStringLiteral("id_plat"));

    verifier(menus.ajouter(idPlat, QDate::currentDate(), 5, &erreur),
             QStringLiteral("Création du menu du jour (quantité 5)"));

    QList<MenuData> menusJour = menus.getMenusParDate(QDate::currentDate());
    const int idMenu = menusJour.isEmpty() ? 0 : menusJour.first().idMenu;
    verifier(!menusJour.isEmpty() && menusJour.first().quantiteRestante == 5,
             QStringLiteral("Quantité restante initiale = 5"));

    verifier(!menus.ajouter(idPlat, QDate::currentDate(), 10, &erreur),
             QStringLiteral("Refus d'un doublon (même plat / même date)"));
    verifier(erreur.contains(QStringLiteral("déjà présent")),
             QStringLiteral("Message explicite sur le doublon de menu"));

    LigneCommande ligne;
    ligne.idMenu = idMenu;
    ligne.idPlat = idPlat;
    ligne.nomPlat = QStringLiteral("Riz Poulet");
    ligne.quantite = 3;
    ligne.prixUnitaire = 5000.0;
    ligne.sousTotal = 15000.0;

    erreur.clear();
    const int idCommande1 = commandes.enregistrerCommande(
        idClient, QDateTime::currentDateTime(), {ligne}, true, &erreur);
    verifier(idCommande1 > 0, QStringLiteral("Commande validée de 3 unités"));
    verifier(restanteMenu(idMenu) == 2,
             QStringLiteral("Quantité restante après validation = 2"));

    const QList<LigneCommande> details = commandes.getDetails(idCommande1, &erreur);
    verifier(details.size() == 1 && qFuzzyCompare(details.first().prixUnitaire, 5000.0),
             QStringLiteral("Prix unitaire historisé = 5 000 Ar"));

    // Changement de prix du plat : l'historique doit rester inchangé
    verifier(plats.modifier(idPlat, QStringLiteral("Riz Poulet"), 7000.0, &erreur),
             QStringLiteral("Modification du prix actuel du plat à 7 000 Ar"));
    const QList<LigneCommande> detailsApres = commandes.getDetails(idCommande1, &erreur);
    verifier(detailsApres.size() == 1 && qFuzzyCompare(detailsApres.first().prixUnitaire, 5000.0),
             QStringLiteral("Le détail conserve l'ancien prix (5 000 Ar)"));

    // Dépassement de stock
    ligne.quantite = 3;
    ligne.sousTotal = 21000.0;
    erreur.clear();
    const int idRefusee = commandes.enregistrerCommande(
        idClient, QDateTime::currentDateTime(), {ligne}, true, &erreur);
    verifier(idRefusee < 0, QStringLiteral("Commande de 3 unités refusée (stock insuffisant)"));
    verifier(erreur.contains(QStringLiteral("quantité insuffisante")),
             QStringLiteral("Message explicite de quantité insuffisante"));
    verifier(restanteMenu(idMenu) == 2,
             QStringLiteral("Aucune quantité consommée par la commande refusée"));

    QSqlQuery compte(Database::instance()->getDatabase());
    compte.exec(QStringLiteral("SELECT COUNT(*) FROM commande"));
    compte.next();
    verifier(compte.value(0).toInt() == 1,
             QStringLiteral("Aucune commande partielle enregistrée après le refus"));

    // Commande en attente : ne réserve pas
    ligne.quantite = 10;
    ligne.sousTotal = 70000.0;
    erreur.clear();
    const int idAttente = commandes.enregistrerCommande(
        idClient, QDateTime::currentDateTime(), {ligne}, false, &erreur);
    verifier(idAttente > 0, QStringLiteral("Commande en attente enregistrée"));
    verifier(restanteMenu(idMenu) == 2,
             QStringLiteral("Une commande en attente ne réserve pas de quantité"));

    erreur.clear();
    verifier(!commandes.validerCommande(idAttente, &erreur),
             QStringLiteral("Validation refusée pour la commande en attente (10 > 2)"));
    verifier(statutCommande(idAttente) == QStringLiteral("EN_ATTENTE"),
             QStringLiteral("La commande refusée reste EN_ATTENTE"));

    verifier(commandes.annulerCommande(idAttente, &erreur),
             QStringLiteral("Annulation de la commande en attente"));
    verifier(commandes.annulerCommande(idCommande1, &erreur),
             QStringLiteral("Annulation de la commande validée"));
    verifier(restanteMenu(idMenu) == 5,
             QStringLiteral("Une commande annulée libère sa quantité (restante = 5)"));

    LigneCommande ligneZero = ligne;
    ligneZero.quantite = 0;
    erreur.clear();
    verifier(commandes.enregistrerCommande(idClient, QDateTime::currentDateTime(),
                                           {ligneZero}, true, &erreur) < 0,
             QStringLiteral("Refus d'une quantité nulle"));

    // Plat d'un autre jour
    verifier(menus.ajouter(idPlat, QDate::currentDate().addDays(1), 10, &erreur),
             QStringLiteral("Création d'un menu pour demain"));
    QList<MenuData> menusDemain = menus.getMenusParDate(QDate::currentDate().addDays(1));
    LigneCommande ligneDemain = ligne;
    ligneDemain.idMenu = menusDemain.isEmpty() ? 0 : menusDemain.first().idMenu;
    ligneDemain.quantite = 1;
    erreur.clear();
    verifier(commandes.enregistrerCommande(idClient, QDateTime::currentDateTime(),
                                           {ligneDemain}, true, &erreur) < 0,
             QStringLiteral("Refus d'un plat qui n'appartient pas au menu du jour"));
    verifier(erreur.contains(QStringLiteral("n'appartient pas")),
             QStringLiteral("Message explicite sur la date du menu"));

    // --- Régressions ---
    erreur.clear();
    verifier(clients.modifier(idClient, QStringLiteral("Étudiant Test"),
                              QStringLiteral("0340000000"), &erreur),
             QStringLiteral("Modification identique d'un client acceptée"));
    verifier(plats.modifier(idPlat, QStringLiteral("Riz Poulet"), 7000.0, &erreur),
             QStringLiteral("Modification identique d'un plat acceptée"));
    verifier(menus.modifier(idMenu, 5, &erreur),
             QStringLiteral("Modification identique d'un menu acceptée"));

    erreur.clear();
    verifier(!plats.ajouter(QStringLiteral("Plat Hors Limite"), 100000000.0, &erreur),
             QStringLiteral("Prix dépassant la capacité DECIMAL(10,2) refusé"));

    erreur.clear();
    verifier(qAbs(commandes.montantCommande(idCommande1, &erreur) - 15000.0) < 0.01,
             QStringLiteral("Montant de la commande relu depuis la base (15 000 Ar)"));

    // Lignes identiques dans la même commande : une seule ligne en base.
    LigneCommande ligneDouble = ligne;
    ligneDouble.quantite = 1;
    ligneDouble.sousTotal = 7000.0;
    erreur.clear();
    const int idDoublon = commandes.enregistrerCommande(
        idClient, QDateTime::currentDateTime(), {ligneDouble, ligneDouble}, false, &erreur);

    QSqlQuery detailDoublon(Database::instance()->getDatabase());
    detailDoublon.prepare(QStringLiteral(
        "SELECT COUNT(*), COALESCE(SUM(quantite), 0) FROM detail_commande "
        "WHERE id_commande = ?"));
    detailDoublon.addBindValue(idDoublon);
    detailDoublon.exec();
    detailDoublon.next();
    verifier(idDoublon > 0 && detailDoublon.value(0).toInt() == 1
                 && detailDoublon.value(1).toInt() == 2,
             QStringLiteral("Lignes identiques fusionnées (1 ligne, quantité 2)"));
    if (idDoublon > 0) {
        commandes.annulerCommande(idDoublon, &erreur);
    }

    // Le test de connexion ne doit pas perturber la connexion active.
    erreur.clear();
    verifier(Database::testerParametres(QStringLiteral("127.0.0.1"), 3306,
                                        QStringLiteral("cantine_universitaire"),
                                        QStringLiteral("root"), QString(), &erreur),
             QStringLiteral("Test de connexion avec les bons paramètres réussi"));
    erreur.clear();
    verifier(!Database::testerParametres(QStringLiteral("127.0.0.1"), 3306,
                                         QStringLiteral("cantine_universitaire"),
                                         QStringLiteral("root"),
                                         QStringLiteral("mot_de_passe_invalide"), &erreur),
             QStringLiteral("Test de connexion avec un mot de passe erroné refusé"));
    verifier(Database::instance()->isConnected(),
             QStringLiteral("La connexion active survit à un test de connexion échoué"));

    erreur.clear();
    verifier(qAbs(commandes.chiffreAffaires(QDate::currentDate(), &erreur)) < 0.01,
             QStringLiteral("Chiffre d'affaires du jour nul après annulations"));

    verifier(!plats.supprimer(idPlat, &erreur),
             QStringLiteral("Suppression d'un plat référencé refusée"));
    verifier(!clients.supprimer(idClient, &erreur),
             QStringLiteral("Suppression d'un client avec commandes refusée"));

    sortie << "\nRésultat : " << (echecs == 0 ? "TOUS LES TESTS SONT PASSÉS"
                                               : QStringLiteral("%1 échec(s)").arg(echecs))
           << "\n";
    sortie.flush();
    return echecs == 0 ? 0 : 1;
}
