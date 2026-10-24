#include "historypage.h"
#include "commandemanager.h"
#include "utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QTimer>
#include <QDate>
#include <QFrame>

HistoryPage::HistoryPage(QWidget *parent)
    : QWidget(parent)
{
    m_manager = new CommandeManager(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);

    auto *titre = new QLabel(QStringLiteral("Historique des commandes"), this);
    titre->setObjectName(QStringLiteral("titrePage"));
    layout->addWidget(titre);

    auto *sousTitre = new QLabel(
        QStringLiteral("Consultez, validez ou annulez les commandes. "
                       "Seules les commandes validées consomment les quantités."), this);
    sousTitre->setObjectName(QStringLiteral("sousTitrePage"));
    layout->addWidget(sousTitre);

    auto *filtres = new QGroupBox(QStringLiteral("Filtres"), this);
    auto *filtresLayout = new QHBoxLayout(filtres);
    filtresLayout->setContentsMargins(16, 12, 16, 12);
    filtresLayout->setSpacing(10);

    auto *lblRecherche = new QLabel(QStringLiteral("Rechercher :"), filtres);
    m_txtRecherche = new QLineEdit(filtres);
    m_txtRecherche->setPlaceholderText(QStringLiteral("Nom du client ou n° de commande..."));

    auto *lblStatut = new QLabel(QStringLiteral("Statut :"), filtres);
    m_cboStatut = new QComboBox(filtres);
    m_cboStatut->addItem(QStringLiteral("Tous"), QString());
    m_cboStatut->addItem(QStringLiteral("En attente"), QStringLiteral("EN_ATTENTE"));
    m_cboStatut->addItem(QStringLiteral("Validée"), QStringLiteral("VALIDEE"));
    m_cboStatut->addItem(QStringLiteral("Annulée"), QStringLiteral("ANNULEE"));

    filtresLayout->addWidget(lblRecherche);
    filtresLayout->addWidget(m_txtRecherche, 1);
    filtresLayout->addSpacing(16);
    filtresLayout->addWidget(lblStatut);
    filtresLayout->addWidget(m_cboStatut);
    filtresLayout->addStretch();

    layout->addWidget(filtres);

    auto *contenu = new QHBoxLayout();
    contenu->setSpacing(16);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("N°"), QStringLiteral("Client"), QStringLiteral("Date"),
         QStringLiteral("Statut"), QStringLiteral("Lignes"), QStringLiteral("Montant total")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);

    contenu->addWidget(m_table, 3);

    auto *droite = new QVBoxLayout();
    droite->setSpacing(10);

    auto *groupeDetails = new QGroupBox(QStringLiteral("Détails de la commande"), this);
    auto *detailsLayout = new QVBoxLayout(groupeDetails);
    detailsLayout->setContentsMargins(16, 12, 16, 16);
    detailsLayout->setSpacing(10);

    m_lblDetails = new QLabel(QStringLiteral("Sélectionnez une commande pour afficher ses détails."), groupeDetails);
    m_lblDetails->setWordWrap(true);
    detailsLayout->addWidget(m_lblDetails);

    m_tableDetails = new QTableWidget(groupeDetails);
    m_tableDetails->setColumnCount(4);
    m_tableDetails->setHorizontalHeaderLabels(
        {QStringLiteral("Plat"), QStringLiteral("Quantité"), QStringLiteral("Prix unitaire"), QStringLiteral("Sous-total")});
    m_tableDetails->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableDetails->setSelectionMode(QAbstractItemView::NoSelection);
    m_tableDetails->setAlternatingRowColors(true);
    m_tableDetails->horizontalHeader()->setStretchLastSection(true);
    m_tableDetails->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tableDetails->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tableDetails->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableDetails->verticalHeader()->setVisible(false);
    detailsLayout->addWidget(m_tableDetails);

    m_lblAucunResultat = new QLabel(QStringLiteral("Aucun détail à afficher."), groupeDetails);
    m_lblAucunResultat->setAlignment(Qt::AlignCenter);
    m_lblAucunResultat->setVisible(false);
    detailsLayout->addWidget(m_lblAucunResultat);

    auto *actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(8);
    m_btnValider = new QPushButton(QStringLiteral("✔ Valider la commande"), groupeDetails);
    m_btnValider->setObjectName(QStringLiteral("btnValider"));
    m_btnAnnuler = new QPushButton(QStringLiteral("✖ Annuler la commande"), groupeDetails);
    m_btnAnnuler->setObjectName(QStringLiteral("btnAnnuler"));
    m_btnSupprimer = new QPushButton(QStringLiteral("🗑 Supprimer"), groupeDetails);
    m_btnSupprimer->setObjectName(QStringLiteral("btnSupprimer"));
    actionsLayout->addStretch();
    actionsLayout->addWidget(m_btnValider);
    actionsLayout->addWidget(m_btnAnnuler);
    actionsLayout->addWidget(m_btnSupprimer);
    detailsLayout->addLayout(actionsLayout);

    layout->addWidget(groupeDetails, 2);

    m_timerRecherche = new QTimer(this);
    m_timerRecherche->setSingleShot(true);
    m_timerRecherche->setInterval(250);

    connect(m_txtRecherche, &QLineEdit::textChanged, m_timerRecherche, QOverload<>::of(&QTimer::start));
    connect(m_cboStatut, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HistoryPage::refresh);
    connect(m_timerRecherche, &QTimer::timeout, this, &HistoryPage::refresh);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &HistoryPage::onSelectionChanged);
    connect(m_btnValider, &QPushButton::clicked, this, &HistoryPage::onValider);
    connect(m_btnAnnuler, &QPushButton::clicked, this, &HistoryPage::onAnnuler);
    connect(m_btnSupprimer, &QPushButton::clicked, this, &HistoryPage::onSupprimer);

    refresh();
}

int HistoryPage::idSelectionne() const
{
    const auto selection = m_table->selectedItems();
    if (selection.isEmpty()) {
        return 0;
    }
    const int row = selection.first()->row();
    if (m_table->item(row, 0)) {
        return m_table->item(row, 0)->data(Qt::UserRole).toInt();
    }
    return 0;
}

QString HistoryPage::statutSelectionne() const
{
    const auto selection = m_table->selectedItems();
    if (selection.isEmpty()) {
        return QString();
    }
    const int row = selection.first()->row();
    if (m_table->item(row, 3)) {
        return m_table->item(row, 3)->data(Qt::UserRole).toString();
    }
    return QString();
}

void HistoryPage::refresh()
{
    QString error;
    const QList<CommandeData> commandes = m_manager->getHistorique(
        m_txtRecherche->text(), m_cboStatut->currentData().toString(), &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Erreur"), error);
        return;
    }

    {
        const auto selection = m_table->selectedItems();
        int idCourant = 0;
        if (!selection.isEmpty()) {
            idCourant = idSelectionne();
        }

        m_table->setRowCount(0);
        int ligneAffichee = 0;
        for (const CommandeData &commande : commandes) {
            const int row = m_table->rowCount();
            m_table->insertRow(row);
            ligneAffichee += 1;

            auto *itemNum = new QTableWidgetItem(QString::number(ligneAffichee));
            itemNum->setData(Qt::UserRole, commande.id);
            itemNum->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, 0, itemNum);

            m_table->setItem(row, 1, new QTableWidgetItem(commande.nomClient));
            m_table->setItem(row, 2, new QTableWidgetItem(
                commande.dateCommande.toString(QStringLiteral("dd/MM/yyyy HH:mm"))));
            auto *itemStatut = new QTableWidgetItem(statutVersLibelle(commande.statut));
            itemStatut->setData(Qt::UserRole, commande.statut);
            itemStatut->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, 3, itemStatut);
            auto *itemLignes = new QTableWidgetItem(QString::number(commande.nbLignes));
            itemLignes->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, 4, itemLignes);
            auto *itemMontant = new QTableWidgetItem(formaterMontant(commande.montantTotal));
            itemMontant->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_table->setItem(row, 5, itemMontant);
        }

        if (idCourant > 0) {
            for (int r = 0; r < m_table->rowCount(); ++r) {
                if (m_table->item(r, 0) && m_table->item(r, 0)->data(Qt::UserRole).toInt() == idCourant) {
                    m_table->selectRow(r);
                    break;
                }
            }
        }
    }

    if (m_table->selectedItems().isEmpty() && m_table->rowCount() > 0) {
        m_table->selectRow(0);
    } else if (m_table->rowCount() == 0) {
        m_table->clearSelection();
        m_lblDetails->setText(QStringLiteral("Aucune commande à afficher."));
        m_tableDetails->setRowCount(0);
        m_lblAucunResultat->setVisible(true);
        m_btnValider->setEnabled(false);
        m_btnAnnuler->setEnabled(false);
        if (m_btnSupprimer) {
            m_btnSupprimer->setEnabled(false);
        }
    }
}

void HistoryPage::onSelectionChanged()
{
    const int id = idSelectionne();
    if (id <= 0) {
        m_lblDetails->setText(QStringLiteral("Sélectionnez une commande pour afficher ses détails."));
        m_tableDetails->setRowCount(0);
        m_lblAucunResultat->setVisible(true);
        m_btnValider->setEnabled(false);
        m_btnAnnuler->setEnabled(false);
        if (m_btnSupprimer) {
            m_btnSupprimer->setEnabled(false);
        }
        return;
    }

    const int row = m_table->selectedItems().first()->row();
    const QString nomClient = m_table->item(row, 1) ? m_table->item(row, 1)->text() : QString();
    const QString statutLibelle = m_table->item(row, 3) ? m_table->item(row, 3)->text() : QString();
    const QString statut = statutSelectionne();
    const QString montant = m_table->item(row, 5) ? m_table->item(row, 5)->text() : QStringLiteral("0 Ar");
    QString montantNet = montant;
    montantNet = montantNet.replace(QStringLiteral(" Ar"), QString());
    montantNet = montantNet.replace(QStringLiteral(" "), QString());
    afficherDetails(id, nomClient, statutLibelle, montantNet.toDouble());

    m_btnValider->setEnabled(statut == QStringLiteral("EN_ATTENTE"));
    m_btnAnnuler->setEnabled(statut == QStringLiteral("EN_ATTENTE"));
    if (m_btnSupprimer) {
        m_btnSupprimer->setEnabled(id > 0);
    }
}

void HistoryPage::afficherDetails(int idCommande, const QString &nomClient, const QString &statut, double montant)
{
    m_lblDetails->setText(QStringLiteral("Commande n°%1 — %2 — %3 — Total : %4")
                              .arg(idCommande)
                              .arg(nomClient)
                              .arg(statut)
                              .arg(formaterMontant(montant)));

    QString error;
    const QList<LigneCommande> details = m_manager->getDetails(idCommande, &error);
    m_tableDetails->setRowCount(0);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Erreur"), error);
        m_lblAucunResultat->setVisible(true);
        return;
    }

    if (details.isEmpty()) {
        m_lblAucunResultat->setVisible(true);
        return;
    }

    m_lblAucunResultat->setVisible(false);
    for (const LigneCommande &ligne : details) {
        const int row = m_tableDetails->rowCount();
        m_tableDetails->insertRow(row);
        m_tableDetails->setItem(row, 0, new QTableWidgetItem(ligne.nomPlat));
        auto *itemQte = new QTableWidgetItem(QString::number(ligne.quantite));
        itemQte->setTextAlignment(Qt::AlignCenter);
        m_tableDetails->setItem(row, 1, itemQte);
        auto *itemPu = new QTableWidgetItem(formaterMontant(ligne.prixUnitaire));
        itemPu->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_tableDetails->setItem(row, 2, itemPu);
        const double sousTotal = ligne.quantite * ligne.prixUnitaire;
        auto *itemSt = new QTableWidgetItem(formaterMontant(sousTotal));
        itemSt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_tableDetails->setItem(row, 3, itemSt);
    }
}

void HistoryPage::onValider()
{
    const int id = idSelectionne();
    if (id <= 0) {
        return;
    }

    const QMessageBox::StandardButton reponse = QMessageBox::question(
        this, QStringLiteral("Confirmer la validation"),
        QStringLiteral("Valider la commande n°%1 ?\n"
                       "Cette action déduira les quantités du stock du jour.").arg(id),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reponse != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!m_manager->validerCommande(id, &error)) {
        QMessageBox::warning(this, QStringLiteral("Erreur"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Succès"),
                             QStringLiteral("La commande n°%1 a été validée.").arg(id));
    refresh();
}

void HistoryPage::onAnnuler()
{
    const int id = idSelectionne();
    if (id <= 0) {
        return;
    }

    const QMessageBox::StandardButton reponse = QMessageBox::question(
        this, QStringLiteral("Confirmer l'annulation"),
        QStringLiteral("Annuler la commande n°%1 ?\n"
                       "Les quantités réservées seront libérées. Cette action est irréversible.").arg(id),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reponse != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!m_manager->annulerCommande(id, &error)) {
        QMessageBox::warning(this, QStringLiteral("Erreur"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Succès"),
                             QStringLiteral("La commande n°%1 a été annulée.").arg(id));
    refresh();
}

void HistoryPage::onSupprimer()
{
    const int id = idSelectionne();
    if (id <= 0) {
        QMessageBox::information(this, QStringLiteral("Information"), QStringLiteral("Sélectionnez une commande à supprimer."));
        return;
    }
    const QMessageBox::StandardButton reponse = QMessageBox::question(
        this, QStringLiteral("Confirmer la suppression (Administrateur)"),
        QStringLiteral("Supprimer définitivement la commande n° %1 ?\n\nLes détails associés seront supprimés.").arg(id),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reponse != QMessageBox::Yes) return;
    QString error;
    if (!m_manager->supprimerCommande(id, &error)) {
        QMessageBox::warning(this, QStringLiteral("Suppression impossible"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Succès"), QStringLiteral("Commande supprimée."));
    refresh();
}
