#include "clientspage.h"
#include "clientmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>

ClientsPage::ClientsPage(QWidget *parent)
    : QWidget(parent)
{
    m_manager = new ClientManager(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto *titre = new QLabel(QStringLiteral("Gestion des clients"), this);
    titre->setObjectName(QStringLiteral("titrePage"));
    layout->addWidget(titre);

    auto *sousTitre = new QLabel(
        QStringLiteral("Ajoutez, modifiez ou supprimez les clients de la cantine."), this);
    sousTitre->setObjectName(QStringLiteral("sousTitre"));
    layout->addWidget(sousTitre);

    auto *rechercheLayout = new QHBoxLayout;
    m_txtRecherche = new QLineEdit(this);
    m_txtRecherche->setPlaceholderText(QStringLiteral("Rechercher par nom ou téléphone…"));
    m_txtRecherche->setClearButtonEnabled(true);
    auto *btnRechercher = new QPushButton(QStringLiteral("🔍 Rechercher"), this);
    auto *btnActualiser = new QPushButton(QStringLiteral("↻ Actualiser"), this);
    rechercheLayout->addWidget(m_txtRecherche, 1);
    rechercheLayout->addWidget(btnRechercher);
    rechercheLayout->addWidget(btnActualiser);
    layout->addLayout(rechercheLayout);

    auto *contenuLayout = new QHBoxLayout;
    contenuLayout->setSpacing(16);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("N°"), QStringLiteral("Nom"), QStringLiteral("Téléphone")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    contenuLayout->addWidget(m_table, 3);

    auto *groupeForm = new QGroupBox(QStringLiteral("Fiche client"), this);
    auto *formLayout = new QVBoxLayout(groupeForm);

    auto *champs = new QFormLayout;
    m_txtNom = new QLineEdit(groupeForm);
    m_txtNom->setPlaceholderText(QStringLiteral("Nom complet"));
    m_txtTelephone = new QLineEdit(groupeForm);
    m_txtTelephone->setPlaceholderText(QStringLiteral("Ex : 034 00 000 00"));
    champs->addRow(QStringLiteral("Nom *"), m_txtNom);
    champs->addRow(QStringLiteral("Téléphone"), m_txtTelephone);
    formLayout->addLayout(champs);

    auto *btnAjouter = new QPushButton(QStringLiteral("➕ Ajouter"), groupeForm);
    btnAjouter->setObjectName(QStringLiteral("btnAjouter"));
    m_btnModifier = new QPushButton(QStringLiteral("✎ Modifier"), groupeForm);
    m_btnModifier->setObjectName(QStringLiteral("btnModifier"));
    m_btnSupprimer = new QPushButton(QStringLiteral("🗑 Supprimer"), groupeForm);
    m_btnSupprimer->setObjectName(QStringLiteral("btnSupprimer"));
    auto *btnVider = new QPushButton(QStringLiteral("Vider"), groupeForm);

    formLayout->addWidget(btnAjouter);
    formLayout->addWidget(m_btnModifier);
    formLayout->addWidget(m_btnSupprimer);
    formLayout->addWidget(btnVider);
    formLayout->addStretch();

    contenuLayout->addWidget(groupeForm, 2);
    layout->addLayout(contenuLayout, 1);

    connect(btnRechercher, &QPushButton::clicked, this, &ClientsPage::refresh);
    connect(m_txtRecherche, &QLineEdit::returnPressed, this, &ClientsPage::refresh);
    connect(btnActualiser, &QPushButton::clicked, this, &ClientsPage::refresh);
    connect(btnAjouter, &QPushButton::clicked, this, &ClientsPage::onAjouter);
    connect(m_btnModifier, &QPushButton::clicked, this, &ClientsPage::onModifier);
    connect(m_btnSupprimer, &QPushButton::clicked, this, &ClientsPage::onSupprimer);
    connect(btnVider, &QPushButton::clicked, this, &ClientsPage::onVider);
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &ClientsPage::onSelectionChanged);

    viderFormulaire();
}

void ClientsPage::refresh()
{
    QString error;
    const QList<ClientData> clients = m_manager->getAll(m_txtRecherche->text(), &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Erreur"), error);
        return;
    }

    m_table->setRowCount(clients.size());
    for (int row = 0; row < clients.size(); ++row) {
        const ClientData &client = clients.at(row);

        auto *itemNumero = new QTableWidgetItem(QString::number(row + 1));
        itemNumero->setData(Qt::UserRole, client.id);
        itemNumero->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 0, itemNumero);
        m_table->setItem(row, 1, new QTableWidgetItem(client.nom));
        m_table->setItem(row, 2, new QTableWidgetItem(client.telephone));
    }

    m_btnModifier->setEnabled(false);
    m_btnSupprimer->setEnabled(false);
}

int ClientsPage::idSelectionne() const
{
    const QList<QTableWidgetItem *> selection = m_table->selectedItems();
    if (selection.isEmpty()) {
        return -1;
    }
    const int row = selection.first()->row();
    QTableWidgetItem *item = m_table->item(row, 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void ClientsPage::onSelectionChanged()
{
    const QList<QTableWidgetItem *> selection = m_table->selectedItems();
    if (selection.isEmpty()) {
        viderFormulaire();
        return;
    }

    const int row = selection.first()->row();
    m_txtNom->setText(m_table->item(row, 1) ? m_table->item(row, 1)->text() : QString());
    m_txtTelephone->setText(m_table->item(row, 2) ? m_table->item(row, 2)->text() : QString());
    m_btnModifier->setEnabled(true);
    m_btnSupprimer->setEnabled(true);
}

void ClientsPage::onAjouter()
{
    QString error;
    if (!m_manager->ajouter(m_txtNom->text(), m_txtTelephone->text(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Ajout impossible"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Succès"),
                             QStringLiteral("Le client a été ajouté avec succès."));
    viderFormulaire();
    refresh();
}

void ClientsPage::onModifier()
{
    const int id = idSelectionne();
    if (id <= 0) {
        QMessageBox::information(this, QStringLiteral("Information"),
                                 QStringLiteral("Sélectionnez un client à modifier."));
        return;
    }

    QString error;
    if (!m_manager->modifier(id, m_txtNom->text(), m_txtTelephone->text(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Modification impossible"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Succès"),
                             QStringLiteral("Les informations du client ont été mises à jour."));
    viderFormulaire();
    refresh();
}

void ClientsPage::onSupprimer()
{
    const int id = idSelectionne();
    if (id <= 0) {
        QMessageBox::information(this, QStringLiteral("Information"),
                                 QStringLiteral("Sélectionnez un client à supprimer."));
        return;
    }

    const int row = m_table->selectedItems().first()->row();
    const QString nom = m_table->item(row, 1) ? m_table->item(row, 1)->text() : QString();

    const QMessageBox::StandardButton reponse = QMessageBox::question(
        this, QStringLiteral("Confirmer la suppression (Administrateur)"),
        QStringLiteral("Supprimer définitivement le client « %1 » ?\n\n"
                       "Les commandes et l'historique seront conservés avec un aperçu du client."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reponse != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!m_manager->supprimer(id, &error)) {
        QMessageBox::warning(this, QStringLiteral("Suppression impossible"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Succès"),
                             QStringLiteral("Le client a été supprimé. Les commandes historiques ont été conservées."));
    viderFormulaire();
    refresh();
}

void ClientsPage::onVider()
{
    viderFormulaire();
    m_table->clearSelection();
}

void ClientsPage::viderFormulaire()
{
    m_txtNom->clear();
    m_txtTelephone->clear();
    m_btnModifier->setEnabled(false);
    m_btnSupprimer->setEnabled(false);
    m_txtNom->setFocus();
}
