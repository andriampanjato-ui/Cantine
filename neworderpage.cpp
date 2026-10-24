#include "neworderpage.h"
#include "clientmanager.h"
#include "menumanager.h"
#include "database.h"
#include "utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QColor>

NewOrderPage::NewOrderPage(QWidget *parent)
    : QWidget(parent)
{
    m_clientManager = new ClientManager(this);
    m_menuManager = new MenuManager(this);
    m_commandeManager = new CommandeManager(this);
    m_selectionMenu = nullptr;
    m_dateMenusCourante = QDate::currentDate();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);

    auto *titre = new QLabel(QStringLiteral("Nouvelle commande"), this);
    titre->setObjectName(QStringLiteral("titrePage"));
    layout->addWidget(titre);

    auto *sousTitre = new QLabel(
        QStringLiteral("Sélectionnez un client, la date, puis composez la commande. "
                       "Le prix unitaire est figé au moment de l'enregistrement."), this);
    sousTitre->setObjectName(QStringLiteral("sousTitre"));
    layout->addWidget(sousTitre);

    auto *enteteLayout = new QHBoxLayout;
    enteteLayout->addWidget(new QLabel(QStringLiteral("Client *"), this));
    m_cboClient = new QComboBox(this);
    m_cboClient->setMinimumWidth(260);
    auto *btnRefreshClients = new QPushButton(QStringLiteral("↻"), this);
    btnRefreshClients->setToolTip(QStringLiteral("Recharger la liste des clients"));
    btnRefreshClients->setFixedWidth(40);
    enteteLayout->addWidget(m_cboClient);
    enteteLayout->addWidget(btnRefreshClients);

    enteteLayout->addSpacing(24);
    enteteLayout->addWidget(new QLabel(QStringLiteral("Date de commande *"), this));
    m_dateCommande = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_dateCommande->setCalendarPopup(true);
    m_dateCommande->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
    enteteLayout->addWidget(m_dateCommande);
    enteteLayout->addStretch();
    layout->addLayout(enteteLayout);

    m_lblInfo = new QLabel(this);
    m_lblInfo->setObjectName(QStringLiteral("sousTitre"));
    layout->addWidget(m_lblInfo);

    auto *contenuLayout = new QHBoxLayout;
    contenuLayout->setSpacing(16);

    // ---- Plats disponibles ----
    auto *groupeMenus = new QGroupBox(QStringLiteral("Plats disponibles au menu de la date choisie"), this);
    auto *menusLayout = new QVBoxLayout(groupeMenus);

    m_tableMenus = new QTableWidget(groupeMenus);
    m_tableMenus->setColumnCount(5);
    m_tableMenus->setHorizontalHeaderLabels(
        {QStringLiteral("Menu"), QStringLiteral("Plat"), QStringLiteral("Prix"),
         QStringLiteral("Qté restante"), QStringLiteral("")});
    m_tableMenus->setColumnHidden(0, true);
    m_tableMenus->setColumnHidden(4, true);
    m_tableMenus->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableMenus->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableMenus->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tableMenus->verticalHeader()->setVisible(false);
    m_tableMenus->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableMenus->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableMenus->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableMenus->setAlternatingRowColors(true);
    menusLayout->addWidget(m_tableMenus);

    auto *ligneAjout = new QHBoxLayout;
    ligneAjout->addWidget(new QLabel(QStringLiteral("Quantité :"), groupeMenus));
    m_spinQuantite = new QSpinBox(groupeMenus);
    m_spinQuantite->setRange(1, 100000);
    m_spinQuantite->setValue(1);
    ligneAjout->addWidget(m_spinQuantite);
    auto *btnAjouterLigne = new QPushButton(QStringLiteral("➕ Ajouter la ligne"), groupeMenus);
    btnAjouterLigne->setObjectName(QStringLiteral("btnAjouterLigne"));
    ligneAjout->addWidget(btnAjouterLigne);
    ligneAjout->addStretch();
    menusLayout->addLayout(ligneAjout);

    contenuLayout->addWidget(groupeMenus, 1);

    // ---- Lignes de la commande ----
    auto *groupeLignes = new QGroupBox(QStringLiteral("Lignes de la commande"), this);
    auto *lignesLayout = new QVBoxLayout(groupeLignes);

    m_tableLignes = new QTableWidget(groupeLignes);
    m_tableLignes->setColumnCount(5);
    m_tableLignes->setHorizontalHeaderLabels(
        {QStringLiteral("Menu"), QStringLiteral("Plat"), QStringLiteral("Quantité"),
         QStringLiteral("Prix unitaire"), QStringLiteral("Sous-total")});
    m_tableLignes->setColumnHidden(0, true);
    m_tableLignes->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableLignes->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableLignes->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tableLignes->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_tableLignes->verticalHeader()->setVisible(false);
    m_tableLignes->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableLignes->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableLignes->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableLignes->setAlternatingRowColors(true);
    lignesLayout->addWidget(m_tableLignes);

    auto *boutonsLignes = new QHBoxLayout;
    auto *btnRetirer = new QPushButton(QStringLiteral("🗑 Retirer la ligne"), groupeLignes);
    auto *btnVider = new QPushButton(QStringLiteral("Vider"), groupeLignes);
    boutonsLignes->addWidget(btnRetirer);
    boutonsLignes->addWidget(btnVider);
    boutonsLignes->addStretch();
    lignesLayout->addLayout(boutonsLignes);

    auto *totalLayout = new QHBoxLayout;
    auto *lblTotalTitre = new QLabel(QStringLiteral("Montant total :"), groupeLignes);
    lblTotalTitre->setObjectName(QStringLiteral("totalTitre"));
    m_lblTotal = new QLabel(QStringLiteral("0,00 Ar"), groupeLignes);
    m_lblTotal->setObjectName(QStringLiteral("totalValeur"));
    totalLayout->addStretch();
    totalLayout->addWidget(lblTotalTitre);
    totalLayout->addWidget(m_lblTotal);
    lignesLayout->addLayout(totalLayout);

    auto *boutonsValidation = new QHBoxLayout;
    auto *btnEnAttente = new QPushButton(QStringLiteral("💾 Enregistrer en attente"), groupeLignes);
    auto *btnValider = new QPushButton(QStringLiteral("✔ Valider la commande"), groupeLignes);
    btnValider->setObjectName(QStringLiteral("btnValider"));
    boutonsValidation->addStretch();
    boutonsValidation->addWidget(btnEnAttente);
    boutonsValidation->addWidget(btnValider);
    lignesLayout->addLayout(boutonsValidation);

    contenuLayout->addWidget(groupeLignes, 1);
    layout->addLayout(contenuLayout, 1);

    connect(btnRefreshClients, &QPushButton::clicked, this, &NewOrderPage::refreshClients);
    connect(m_dateCommande, &QDateTimeEdit::dateChanged,
            this, &NewOrderPage::onDateChanged);
    connect(btnAjouterLigne, &QPushButton::clicked, this, &NewOrderPage::onAjouterLigne);
    connect(btnRetirer, &QPushButton::clicked, this, &NewOrderPage::onRetirerLigne);
    connect(btnVider, &QPushButton::clicked, this, &NewOrderPage::onVider);
    connect(btnEnAttente, &QPushButton::clicked, this, &NewOrderPage::onEnregistrerEnAttente);
    connect(btnValider, &QPushButton::clicked, this, &NewOrderPage::onValiderCommande);
    connect(m_tableMenus, &QTableWidget::itemSelectionChanged, this, [this]() {
        const QList<QTableWidgetItem *> selection = m_tableMenus->selectedItems();
        if (!selection.isEmpty()) {
            QTableWidgetItem *item = m_tableMenus->item(selection.first()->row(), 0);
            if (item) {
                m_selectionMenu = item;
                m_spinQuantite->setValue(1);
            }
        }
    });
}

void NewOrderPage::refresh()
{
    refreshClients();
    refreshMenus();
}

void NewOrderPage::refreshClients()
{
    const int selection = m_cboClient->currentData().toInt();
    m_cboClient->clear();

    QString error;
    const QList<ClientData> clients = m_clientManager->getAll(QString(), &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Erreur"), error);
        return;
    }

    for (const ClientData &client : clients) {
        QString texte = client.nom;
        if (!client.telephone.trimmed().isEmpty()) {
            texte += QStringLiteral(" — ") + client.telephone;
        }
        m_cboClient->addItem(texte, client.id);
    }

    if (clients.isEmpty()) {
        m_lblInfo->setText(
            QStringLiteral("Aucun client enregistré. Ajoutez un client dans la rubrique « Clients »."));
    } else if (selection > 0) {
        const int index = m_cboClient->findData(selection);
        if (index >= 0) {
            m_cboClient->setCurrentIndex(index);
        }
    }
}

void NewOrderPage::refreshMenus()
{
    const QDate date = m_dateCommande->date();
    m_dateMenusCourante = date;

    QString error;
    const QList<MenuData> menus = m_menuManager->getMenusDisponibles(date, &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Erreur"), error);
        return;
    }

    m_selectionMenu = nullptr;
    m_tableMenus->setRowCount(menus.size());
    for (int row = 0; row < menus.size(); ++row) {
        const MenuData &menu = menus.at(row);

        auto *itemMenu = new QTableWidgetItem(QString::number(menu.idMenu));
        itemMenu->setData(Qt::UserRole, menu.idMenu);
        itemMenu->setData(Qt::UserRole + 1, menu.idPlat);
        itemMenu->setData(Qt::UserRole + 2, menu.prix);
        itemMenu->setData(Qt::UserRole + 3, menu.quantiteRestante);
        m_tableMenus->setItem(row, 0, itemMenu);

        m_tableMenus->setItem(row, 1, new QTableWidgetItem(menu.nomPlat));

        auto *itemPrix = new QTableWidgetItem(formatAr(menu.prix));
        itemPrix->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_tableMenus->setItem(row, 2, itemPrix);

        auto *itemRestante = new QTableWidgetItem(QString::number(menu.quantiteRestante));
        itemRestante->setTextAlignment(Qt::AlignCenter);
        if (menu.quantiteRestante <= 5) {
            itemRestante->setForeground(QColor(QStringLiteral("#d97706")));
        }
        m_tableMenus->setItem(row, 3, itemRestante);

        m_tableMenus->setItem(row, 4, new QTableWidgetItem());
    }

    if (menus.isEmpty()) {
        m_lblInfo->setText(
            QStringLiteral("Aucun plat disponible pour le %1. Ajoutez des menus journaliers.")
            .arg(formatDate(date)));
    } else {
        m_lblInfo->setText(
            QStringLiteral("Menus du %1 — sélectionnez un plat puis ajoutez-le à la commande.")
            .arg(formatDate(date)));
    }
}

int NewOrderPage::quantiteDejaAjoutee(int idMenu) const
{
    int total = 0;
    for (const LigneCommande &ligne : m_lignes) {
        if (ligne.idMenu == idMenu) {
            total += ligne.quantite;
        }
    }
    return total;
}

void NewOrderPage::onDateChanged(const QDate &date)
{
    if (date != m_dateMenusCourante) {
        const bool lignesReinitialisees = !m_lignes.isEmpty();
        if (lignesReinitialisees) {
            m_lignes.clear();
            afficherLignes();
        }
        refreshMenus();
        if (lignesReinitialisees) {
            m_lblInfo->setText(
                QStringLiteral("La date a changé : les lignes de la commande ont été réinitialisées."));
        }
    }
}

void NewOrderPage::onAjouterLigne()
{
    if (!m_selectionMenu) {
        QMessageBox::information(this, QStringLiteral("Information"),
                                 QStringLiteral("Sélectionnez d'abord un plat dans le menu du jour."));
        return;
    }

    const int idMenu = m_selectionMenu->data(Qt::UserRole).toInt();
    const int idPlat = m_selectionMenu->data(Qt::UserRole + 1).toInt();
    const double prix = m_selectionMenu->data(Qt::UserRole + 2).toDouble();
    const int restante = m_selectionMenu->data(Qt::UserRole + 3).toInt();
    const QString nomPlat = m_tableMenus->item(m_selectionMenu->row(), 1)
                                ? m_tableMenus->item(m_selectionMenu->row(), 1)->text()
                                : QString();
    const int quantite = m_spinQuantite->value();

    if (quantite <= 0) {
        QMessageBox::warning(this, QStringLiteral("Quantité invalide"),
                             QStringLiteral("La quantité doit être strictement positive."));
        return;
    }

    const int deja = quantiteDejaAjoutee(idMenu);
    if (deja + quantite > restante) {
        QMessageBox::warning(
            this, QStringLiteral("Quantité insuffisante"),
            QStringLiteral(
                "Quantité insuffisante pour le plat « %1 ».\n"
                "Quantité restante : %2 — déjà ajoutée : %3 — demandée : %4.")
                .arg(nomPlat).arg(restante).arg(deja).arg(quantite));
        return;
    }

    bool trouve = false;
    for (LigneCommande &ligne : m_lignes) {
        if (ligne.idMenu == idMenu) {
            ligne.quantite += quantite;
            ligne.sousTotal = ligne.quantite * ligne.prixUnitaire;
            trouve = true;
            break;
        }
    }

    if (!trouve) {
        LigneCommande ligne;
        ligne.idMenu = idMenu;
        ligne.idPlat = idPlat;
        ligne.nomPlat = nomPlat;
        ligne.quantite = quantite;
        ligne.prixUnitaire = prix;
        ligne.sousTotal = quantite * prix;
        m_lignes.append(ligne);
    }

    afficherLignes();
}

void NewOrderPage::onRetirerLigne()
{
    if (m_tableLignes->currentRow() < 0) {
        QMessageBox::information(this, QStringLiteral("Information"),
                                 QStringLiteral("Sélectionnez une ligne à retirer."));
        return;
    }

    QTableWidgetItem *itemMenu = m_tableLignes->item(m_tableLignes->currentRow(), 0);
    if (!itemMenu) {
        return;
    }
    const int idMenu = itemMenu->data(Qt::UserRole).toInt();

    for (int i = m_lignes.size() - 1; i >= 0; --i) {
        if (m_lignes.at(i).idMenu == idMenu) {
            m_lignes.removeAt(i);
        }
    }
    afficherLignes();
}

void NewOrderPage::onVider()
{
    if (m_lignes.isEmpty()) {
        return;
    }
    const QMessageBox::StandardButton reponse = QMessageBox::question(
        this, QStringLiteral("Confirmer"),
        QStringLiteral("Vider toutes les lignes de la commande ?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reponse == QMessageBox::Yes) {
        m_lignes.clear();
        afficherLignes();
    }
}

void NewOrderPage::afficherLignes()
{
    m_tableLignes->setRowCount(m_lignes.size());
    for (int row = 0; row < m_lignes.size(); ++row) {
        const LigneCommande &ligne = m_lignes.at(row);

        auto *itemMenu = new QTableWidgetItem(QString::number(ligne.idMenu));
        itemMenu->setData(Qt::UserRole, ligne.idMenu);
        m_tableLignes->setItem(row, 0, itemMenu);

        m_tableLignes->setItem(row, 1, new QTableWidgetItem(ligne.nomPlat));

        auto *itemQte = new QTableWidgetItem(QString::number(ligne.quantite));
        itemQte->setTextAlignment(Qt::AlignCenter);
        m_tableLignes->setItem(row, 2, itemQte);

        auto *itemPrix = new QTableWidgetItem(formatAr(ligne.prixUnitaire));
        itemPrix->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_tableLignes->setItem(row, 3, itemPrix);

        auto *itemSousTotal = new QTableWidgetItem(formatAr(ligne.sousTotal));
        itemSousTotal->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_tableLignes->setItem(row, 4, itemSousTotal);
    }
    recalculerTotal();
}

void NewOrderPage::recalculerTotal()
{
    double total = 0.0;
    for (const LigneCommande &ligne : m_lignes) {
        total += ligne.sousTotal;
    }
    m_lblTotal->setText(formatAr(total));
}

bool NewOrderPage::enregistrer(bool valider)
{
    if (m_cboClient->currentData().toInt() <= 0) {
        QMessageBox::warning(this, QStringLiteral("Commande incomplète"),
                             QStringLiteral("Sélectionnez un client."));
        return false;
    }
    if (m_lignes.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Commande incomplète"),
                             QStringLiteral("Ajoutez au moins un plat à la commande."));
        return false;
    }

    const int idClient = m_cboClient->currentData().toInt();
    const QDateTime dateCommande = m_dateCommande->dateTime();

    QString error;
    const int idCommande = m_commandeManager->enregistrerCommande(
        idClient, dateCommande, m_lignes, valider, &error);

    if (idCommande <= 0) {
        QMessageBox::critical(this, QStringLiteral("Commande refusée"), error);
        return false;
    }

    // Total relu depuis la base : le prix réellement enregistré peut différer
    // de celui affiché si le tarif a changé entre-temps.
    QString erreurMontant;
    double total = m_commandeManager->montantCommande(idCommande, &erreurMontant);
    if (total < 0.0) {
        total = 0.0;
        for (const LigneCommande &ligne : m_lignes) {
            total += ligne.sousTotal;
        }
    }

    QMessageBox::information(
        this, QStringLiteral("Succès"),
        valider
            ? QStringLiteral("Commande n°%1 validée avec succès.\nMontant total : %2")
                  .arg(idCommande).arg(formatAr(total))
            : QStringLiteral("Commande n°%1 enregistrée en attente.\nMontant total : %2\n"
                             "Elle ne réserve pas encore de quantité : validez-la dans l'historique.")
                  .arg(idCommande).arg(formatAr(total)));

    m_lignes.clear();
    afficherLignes();
    refreshMenus();
    return true;
}

void NewOrderPage::onEnregistrerEnAttente()
{
    enregistrer(false);
}

void NewOrderPage::onValiderCommande()
{
    const QMessageBox::StandardButton reponse = QMessageBox::question(
        this, QStringLiteral("Confirmer la validation"),
        QStringLiteral("Valider la commande et réserver immédiatement les quantités ?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reponse != QMessageBox::Yes) {
        return;
    }
    enregistrer(true);
}
