#include "platspage.h"
#include "platmanager.h"
#include "utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDoubleSpinBox>
#include <QLocale>

PlatsPage::PlatsPage(QWidget *parent)
    : QWidget(parent)
{
    m_manager = new PlatManager(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto *titre = new QLabel(QStringLiteral("Gestion des plats"), this);
    titre->setObjectName(QStringLiteral("titrePage"));
    layout->addWidget(titre);

    auto *sousTitre = new QLabel(
        QStringLiteral("Gérez le catalogue des plats et leurs prix (en Ariary)."), this);
    sousTitre->setObjectName(QStringLiteral("sousTitre"));
    layout->addWidget(sousTitre);

    auto *rechercheLayout = new QHBoxLayout;
    m_txtRecherche = new QLineEdit(this);
    m_txtRecherche->setPlaceholderText(QStringLiteral("Rechercher un plat par nom…"));
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
        {QStringLiteral("ID"), QStringLiteral("Nom du plat"), QStringLiteral("Prix")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    contenuLayout->addWidget(m_table, 3);

    auto *groupeForm = new QGroupBox(QStringLiteral("Fiche plat"), this);
    auto *formLayout = new QVBoxLayout(groupeForm);

    auto *champs = new QFormLayout;
    m_txtNom = new QLineEdit(groupeForm);
    m_txtNom->setPlaceholderText(QStringLiteral("Nom du plat"));
    m_spinPrix = new QDoubleSpinBox(groupeForm);
    m_spinPrix->setRange(0.0, 99999999.99);
    m_spinPrix->setDecimals(2);
    m_spinPrix->setSingleStep(500.0);
    m_spinPrix->setSuffix(QStringLiteral(" Ar"));
    m_spinPrix->setGroupSeparatorShown(true);
    champs->addRow(QStringLiteral("Nom *"), m_txtNom);
    champs->addRow(QStringLiteral("Prix *"), m_spinPrix);
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

    connect(btnRechercher, &QPushButton::clicked, this, &PlatsPage::refresh);
    connect(m_txtRecherche, &QLineEdit::returnPressed, this, &PlatsPage::refresh);
    connect(btnActualiser, &QPushButton::clicked, this, &PlatsPage::refresh);
    connect(btnAjouter, &QPushButton::clicked, this, &PlatsPage::onAjouter);
    connect(m_btnModifier, &QPushButton::clicked, this, &PlatsPage::onModifier);
    connect(m_btnSupprimer, &QPushButton::clicked, this, &PlatsPage::onSupprimer);
    connect(btnVider, &QPushButton::clicked, this, &PlatsPage::onVider);
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &PlatsPage::onSelectionChanged);

    viderFormulaire();
}

void PlatsPage::refresh()
{
    QString error;
    const QList<PlatData> plats = m_manager->getAll(m_txtRecherche->text(), &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Erreur"), error);
        return;
    }

    m_table->setRowCount(plats.size());
    for (int row = 0; row < plats.size(); ++row) {
        const PlatData &plat = plats.at(row);

        auto *itemId = new QTableWidgetItem(QString::number(plat.id));
        itemId->setData(Qt::UserRole, plat.id);
        itemId->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 0, itemId);
        m_table->setItem(row, 1, new QTableWidgetItem(plat.nom));

        auto *itemPrix = new QTableWidgetItem(formatAr(plat.prix));
        itemPrix->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itemPrix->setData(Qt::UserRole + 1, plat.prix);
        m_table->setItem(row, 2, itemPrix);
    }

    m_btnModifier->setEnabled(false);
    m_btnSupprimer->setEnabled(false);
}

int PlatsPage::idSelectionne() const
{
    const QList<QTableWidgetItem *> selection = m_table->selectedItems();
    if (selection.isEmpty()) {
        return -1;
    }
    const int row = selection.first()->row();
    QTableWidgetItem *item = m_table->item(row, 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void PlatsPage::onSelectionChanged()
{
    const QList<QTableWidgetItem *> selection = m_table->selectedItems();
    if (selection.isEmpty()) {
        viderFormulaire();
        return;
    }

    const int row = selection.first()->row();
    m_txtNom->setText(m_table->item(row, 1) ? m_table->item(row, 1)->text() : QString());
    if (m_table->item(row, 2)) {
        m_spinPrix->setValue(m_table->item(row, 2)->data(Qt::UserRole + 1).toDouble());
    }
    m_btnModifier->setEnabled(true);
    m_btnSupprimer->setEnabled(true);
}

void PlatsPage::onAjouter()
{
    QString error;
    if (!m_manager->ajouter(m_txtNom->text(), m_spinPrix->value(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Ajout impossible"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Succès"),
                             QStringLiteral("Le plat a été ajouté avec succès."));
    viderFormulaire();
    refresh();
}

void PlatsPage::onModifier()
{
    const int id = idSelectionne();
    if (id <= 0) {
        QMessageBox::information(this, QStringLiteral("Information"),
                                 QStringLiteral("Sélectionnez un plat à modifier."));
        return;
    }

    QString error;
    if (!m_manager->modifier(id, m_txtNom->text(), m_spinPrix->value(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Modification impossible"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Succès"),
                             QStringLiteral("Le plat a été mis à jour."));
    viderFormulaire();
    refresh();
}

void PlatsPage::onSupprimer()
{
    const int id = idSelectionne();
    if (id <= 0) {
        QMessageBox::information(this, QStringLiteral("Information"),
                                 QStringLiteral("Sélectionnez un plat à supprimer."));
        return;
    }

    const int row = m_table->selectedItems().first()->row();
    const QString nom = m_table->item(row, 1) ? m_table->item(row, 1)->text() : QString();

    const QMessageBox::StandardButton reponse = QMessageBox::question(
        this, QStringLiteral("Confirmer la suppression"),
        QStringLiteral("Supprimer définitivement le plat « %1 » ?").arg(nom),
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
                             QStringLiteral("Le plat a été supprimé."));
    viderFormulaire();
    refresh();
}

void PlatsPage::onVider()
{
    viderFormulaire();
    m_table->clearSelection();
}

void PlatsPage::viderFormulaire()
{
    m_txtNom->clear();
    m_spinPrix->setValue(0.0);
    m_btnModifier->setEnabled(false);
    m_btnSupprimer->setEnabled(false);
    m_txtNom->setFocus();
}
