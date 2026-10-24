#include "dashboard.h"
#include "database.h"
#include "clientmanager.h"
#include "platmanager.h"
#include "menumanager.h"
#include "commandemanager.h"
#include "utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QDate>
#include <QColor>
#include <QTableWidgetItem>

Dashboard::Dashboard(QWidget *parent)
    : QWidget(parent)
{
    m_clientManager = new ClientManager(this);
    m_platManager = new PlatManager(this);
    m_menuManager = new MenuManager(this);
    m_commandeManager = new CommandeManager(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    auto *headerLayout = new QHBoxLayout;
    auto *titre = new QLabel(QStringLiteral("Tableau de bord"), this);
    titre->setObjectName(QStringLiteral("titrePage"));
    m_lblDate = new QLabel(this);
    m_lblDate->setObjectName(QStringLiteral("sousTitre"));
    headerLayout->addWidget(titre);
    headerLayout->addStretch();
    headerLayout->addWidget(m_lblDate);
    layout->addLayout(headerLayout);

    auto *cardsLayout = new QHBoxLayout;
    cardsLayout->setSpacing(12);
    cardsLayout->addWidget(creerCarte(QStringLiteral("Clients"), QStringLiteral("👥"), &m_lblClients));
    cardsLayout->addWidget(creerCarte(QStringLiteral("Plats enregistrés"), QStringLiteral("🍽"), &m_lblPlats));
    cardsLayout->addWidget(creerCarte(QStringLiteral("Plats au menu du jour"), QStringLiteral("📅"), &m_lblMenusJour));
    cardsLayout->addWidget(creerCarte(QStringLiteral("Commandes validées aujourd'hui"), QStringLiteral("✅"), &m_lblCommandesValidees));
    cardsLayout->addWidget(creerCarte(QStringLiteral("Chiffre d'affaires du jour"), QStringLiteral("💰"), &m_lblChiffreAffaires));
    layout->addLayout(cardsLayout);

    auto *tablesLayout = new QHBoxLayout;
    tablesLayout->setSpacing(16);

    auto *groupeRecentes = new QGroupBox(QStringLiteral("Commandes récentes"), this);
    auto *recentesLayout = new QVBoxLayout(groupeRecentes);
    m_tableRecentes = new QTableWidget(groupeRecentes);
    m_tableRecentes->setColumnCount(5);
    m_tableRecentes->setHorizontalHeaderLabels(
        {QStringLiteral("N°"), QStringLiteral("Client"), QStringLiteral("Date"),
         QStringLiteral("Montant"), QStringLiteral("Statut")});
    m_tableRecentes->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableRecentes->verticalHeader()->setVisible(false);
    m_tableRecentes->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableRecentes->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableRecentes->setAlternatingRowColors(true);
    recentesLayout->addWidget(m_tableRecentes);
    tablesLayout->addWidget(groupeRecentes, 3);

    auto *groupeStock = new QGroupBox(QStringLiteral("Plats à stock faible (aujourd'hui)"), this);
    auto *stockLayout = new QVBoxLayout(groupeStock);
    m_tableStockFaible = new QTableWidget(groupeStock);
    m_tableStockFaible->setColumnCount(3);
    m_tableStockFaible->setHorizontalHeaderLabels(
        {QStringLiteral("Plat"), QStringLiteral("Restante"), QStringLiteral("Initiale")});
    m_tableStockFaible->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableStockFaible->verticalHeader()->setVisible(false);
    m_tableStockFaible->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableStockFaible->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableStockFaible->setAlternatingRowColors(true);
    stockLayout->addWidget(m_tableStockFaible);
    tablesLayout->addWidget(groupeStock, 2);

    layout->addLayout(tablesLayout, 1);
}

QWidget *Dashboard::creerCarte(const QString &titre, const QString &symbole, QLabel **valeurLabel)
{
    auto *carte = new QFrame(this);
    carte->setObjectName(QStringLiteral("carte"));

    auto *layout = new QVBoxLayout(carte);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(4);

    auto *entete = new QHBoxLayout;
    auto *lblSymbole = new QLabel(symbole, carte);
    lblSymbole->setObjectName(QStringLiteral("carteSymbole"));
    auto *lblTitre = new QLabel(titre, carte);
    lblTitre->setObjectName(QStringLiteral("carteTitre"));
    lblTitre->setWordWrap(true);
    entete->addWidget(lblSymbole);
    entete->addWidget(lblTitre, 1);
    layout->addLayout(entete);

    auto *valeur = new QLabel(QStringLiteral("—"), carte);
    valeur->setObjectName(QStringLiteral("carteValeur"));
    layout->addWidget(valeur);

    *valeurLabel = valeur;
    return carte;
}

void Dashboard::refresh()
{
    if (!Database::instance()->isConnected()) {
        return;
    }

    QString error;
    m_lblDate->setText(QStringLiteral("Aujourd'hui : %1").arg(formatDate(QDate::currentDate())));

    m_lblClients->setText(QString::number(m_clientManager->compter(&error)));
    m_lblPlats->setText(QString::number(m_platManager->compter(&error)));
    m_lblMenusJour->setText(QString::number(m_menuManager->compterParDate(QDate::currentDate(), &error)));
    m_lblCommandesValidees->setText(
        QString::number(m_commandeManager->compterValidees(QDate::currentDate(), &error)));
    m_lblChiffreAffaires->setText(
        formatAr(m_commandeManager->chiffreAffaires(QDate::currentDate(), &error)));

    const QList<CommandeData> recentes = m_commandeManager->getRecentes(8, &error);
    m_tableRecentes->setRowCount(recentes.size());
    for (int row = 0; row < recentes.size(); ++row) {
        const CommandeData &commande = recentes.at(row);
        auto *itemId = new QTableWidgetItem(QString::number(commande.id));
        itemId->setTextAlignment(Qt::AlignCenter);
        m_tableRecentes->setItem(row, 0, itemId);
        m_tableRecentes->setItem(row, 1, new QTableWidgetItem(commande.nomClient));
        m_tableRecentes->setItem(row, 2, new QTableWidgetItem(formatDateTime(commande.dateCommande)));

        auto *itemMontant = new QTableWidgetItem(formatAr(commande.montantTotal));
        itemMontant->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_tableRecentes->setItem(row, 3, itemMontant);

        auto *itemStatut = new QTableWidgetItem(commande.statut);
        itemStatut->setTextAlignment(Qt::AlignCenter);
        m_tableRecentes->setItem(row, 4, itemStatut);
    }
    m_tableRecentes->resizeColumnsToContents();
    m_tableRecentes->horizontalHeader()->setStretchLastSection(true);

    const QList<MenuData> stockFaible = m_menuManager->getMenusStockFaible(5, &error);
    m_tableStockFaible->setRowCount(stockFaible.size());
    for (int row = 0; row < stockFaible.size(); ++row) {
        const MenuData &menu = stockFaible.at(row);
        m_tableStockFaible->setItem(row, 0, new QTableWidgetItem(menu.nomPlat));

        auto *itemRestante = new QTableWidgetItem(QString::number(menu.quantiteRestante));
        itemRestante->setTextAlignment(Qt::AlignCenter);
        if (menu.quantiteRestante <= 2) {
            itemRestante->setForeground(QColor(QStringLiteral("#dc2626")));
        }
        m_tableStockFaible->setItem(row, 1, itemRestante);

        auto *itemInitiale = new QTableWidgetItem(QString::number(menu.quantiteInitiale));
        itemInitiale->setTextAlignment(Qt::AlignCenter);
        m_tableStockFaible->setItem(row, 2, itemInitiale);
    }
    m_tableStockFaible->horizontalHeader()->setStretchLastSection(true);
}
