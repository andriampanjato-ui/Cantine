#include "menupage.h"
#include "menumanager.h"
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
#include <QDate>
#include <QColor>
#include <QSignalBlocker>

MenuPage::MenuPage(QWidget *parent)
    : QWidget(parent)
{
    m_menuManager = new MenuManager(this);
    m_platManager = new PlatManager(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto *titre = new QLabel(QStringLiteral("Menus journaliers"), this);
    titre->setObjectName(QStringLiteral("titrePage"));
    layout->addWidget(titre);

    auto *sousTitre = new QLabel(
        QStringLiteral("Définissez les plats proposés chaque jour et les quantités disponibles."),
        this);
    sousTitre->setObjectName(QStringLiteral("sousTitre"));
    layout->addWidget(sousTitre);

    auto *filtreLayout = new QHBoxLayout;
    filtreLayout->addWidget(new QLabel(QStringLiteral("Date du menu :"), this));
    m_dateFiltre = new QDateEdit(QDate::currentDate(), this);
    m_dateFiltre->setCalendarPopup(true);
    m_dateFiltre->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    auto *btnAfficher = new QPushButton(QStringLiteral("🔍 Afficher"), this);
    auto *btnActualiser = new QPushButton(QStringLiteral("↻ Actualiser"), this);
    filtreLayout->addWidget(m_dateFiltre);
    filtreLayout->addWidget(btnAfficher);
    filtreLayout->addWidget(btnActualiser);
    filtreLayout->addStretch();
    layout->addLayout(filtreLayout);

    auto *contenuLayout = new QHBoxLayout;
    contenuLayout->setSpacing(16);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("N°"), QStringLiteral("Plat"), QStringLiteral("Prix"),
         QStringLiteral("Qté initiale"), QStringLiteral("Qté commandée"),
         QStringLiteral("Qté restante")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    for (int col = 2; col < 6; ++col) {
        m_table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
    }
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    contenuLayout->addWidget(m_table, 3);

    auto *groupeForm = new QGroupBox(QStringLiteral("Menu du jour"), this);
    auto *formLayout = new QVBoxLayout(groupeForm);

    auto *champs = new QFormLayout;
    m_cboPlat = new QComboBox(groupeForm);
    m_dateMenu = new QDateEdit(QDate::currentDate(), groupeForm);
    m_dateMenu->setCalendarPopup(true);
    m_dateMenu->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_spinQuantite = new QSpinBox(groupeForm);
    m_spinQuantite->setRange(0, 100000);
    m_spinQuantite->setValue(20);
    champs->addRow(QStringLiteral("Plat *"), m_cboPlat);
    champs->addRow(QStringLiteral("Date *"), m_dateMenu);
    champs->addRow(QStringLiteral("Quantité initiale *"), m_spinQuantite);
    formLayout->addLayout(champs);

    auto *btnAjouter = new QPushButton(QStringLiteral("➕ Ajouter au menu"), groupeForm);
    btnAjouter->setObjectName(QStringLiteral("btnAjouter"));
    m_btnModifier = new QPushButton(QStringLiteral("✎ Modifier la quantité"), groupeForm);
    m_btnModifier->setObjectName(QStringLiteral("btnModifier"));
    m_btnSupprimer = new QPushButton(QStringLiteral("🗑 Retirer du menu"), groupeForm);
    m_btnSupprimer->setObjectName(QStringLiteral("btnSupprimer"));
    auto *btnVider = new QPushButton(QStringLiteral("Vider"), groupeForm);

    formLayout->addWidget(btnAjouter);
    formLayout->addWidget(m_btnModifier);
    formLayout->addWidget(m_btnSupprimer);
    formLayout->addWidget(btnVider);
    formLayout->addStretch();

    contenuLayout->addWidget(groupeForm, 2);
    layout->addLayout(contenuLayout, 1);

    connect(btnAfficher, &QPushButton::clicked, this, &MenuPage::refresh);
    connect(btnActualiser, &QPushButton::clicked, this, &MenuPage::refresh);
    connect(m_dateFiltre, &QDateEdit::dateChanged, this, [this](const QDate &date) {
        m_dateMenu->setDate(date);
        refresh();
    });
    connect(btnAjouter, &QPushButton::clicked, this, &MenuPage::onAjouter);
    connect(m_btnModifier, &QPushButton::clicked, this, &MenuPage::onModifier);
    connect(m_btnSupprimer, &QPushButton::clicked, this, &MenuPage::onSupprimer);
    connect(btnVider, &QPushButton::clicked, this, &MenuPage::onVider);
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &MenuPage::onSelectionChanged);

    viderFormulaire();
}

void MenuPage::chargerPlats()
{
    const int selection = m_cboPlat->currentData().toInt();
    m_cboPlat->clear();

    QString error;
    const QList<PlatData> plats = m_platManager->getAll(QString(), &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Erreur"), error);
        return;
    }

    for (const PlatData &plat : plats) {
        m_cboPlat->addItem(
            QStringLiteral("%1 — %2").arg(plat.nom, formatAr(plat.prix)), plat.id);
    }

    if (selection > 0) {
        const int index = m_cboPlat->findData(selection);
        if (index >= 0) {
            m_cboPlat->setCurrentIndex(index);
        }
    }
}

void MenuPage::refresh()
{
    chargerPlats();

    QString error;
    const QList<MenuData> menus = m_menuManager->getMenusParDate(m_dateFiltre->date(), &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("Erreur"), error);
        return;
    }

    m_table->setRowCount(menus.size());
    for (int row = 0; row < menus.size(); ++row) {
        const MenuData &menu = menus.at(row);

        auto *itemNumero = new QTableWidgetItem(QString::number(row + 1));
        itemNumero->setData(Qt::UserRole, menu.idMenu);
        itemNumero->setData(Qt::UserRole + 1, menu.idPlat);
        itemNumero->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 0, itemNumero);
        m_table->setItem(row, 1, new QTableWidgetItem(menu.nomPlat));

        auto *itemPrix = new QTableWidgetItem(formatAr(menu.prix));
        itemPrix->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 2, itemPrix);

        auto *itemInitiale = new QTableWidgetItem(QString::number(menu.quantiteInitiale));
        itemInitiale->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 3, itemInitiale);

        auto *itemCommandee = new QTableWidgetItem(QString::number(menu.quantiteCommandee));
        itemCommandee->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 4, itemCommandee);

        auto *itemRestante = new QTableWidgetItem(QString::number(menu.quantiteRestante));
        itemRestante->setTextAlignment(Qt::AlignCenter);
        if (menu.quantiteRestante <= 0) {
            itemRestante->setForeground(QColor(QStringLiteral("#dc2626")));
        } else if (menu.quantiteRestante <= 5) {
            itemRestante->setForeground(QColor(QStringLiteral("#d97706")));
        }
        m_table->setItem(row, 5, itemRestante);
    }

    m_btnModifier->setEnabled(false);
    m_btnSupprimer->setEnabled(false);
}

int MenuPage::idSelectionne() const
{
    const QList<QTableWidgetItem *> selection = m_table->selectedItems();
    if (selection.isEmpty()) {
        return -1;
    }
    const int row = selection.first()->row();
    QTableWidgetItem *item = m_table->item(row, 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void MenuPage::onSelectionChanged()
{
    const QList<QTableWidgetItem *> selection = m_table->selectedItems();
    if (selection.isEmpty()) {
        viderFormulaire();
        return;
    }

    const int row = selection.first()->row();
    QTableWidgetItem *itemId = m_table->item(row, 0);
    if (!itemId) {
        return;
    }

    const int idPlat = itemId->data(Qt::UserRole + 1).toInt();
    const int index = m_cboPlat->findData(idPlat);
    if (index >= 0) {
        m_cboPlat->setCurrentIndex(index);
    }
    m_dateMenu->setDate(m_dateFiltre->date());
    if (m_table->item(row, 3)) {
        m_spinQuantite->setValue(m_table->item(row, 3)->text().toInt());
    }

    m_btnModifier->setEnabled(true);
    m_btnSupprimer->setEnabled(true);
}

void MenuPage::onAjouter()
{
    const int idPlat = m_cboPlat->currentData().toInt();
    if (idPlat <= 0) {
        QMessageBox::information(this, QStringLiteral("Information"),
                                 QStringLiteral("Aucun plat n'est disponible. Ajoutez d'abord un plat."));
        return;
    }

    QString error;
    if (!m_menuManager->ajouter(idPlat, m_dateMenu->date(),
                                m_spinQuantite->value(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Ajout impossible"), error);
        return;
    }

    QMessageBox::information(this, QStringLiteral("Succès"),
                             QStringLiteral("Le plat a été ajouté au menu du %1.")
                             .arg(formatDate(m_dateMenu->date())));
    {
        const QSignalBlocker blocker(m_dateFiltre);
        m_dateFiltre->setDate(m_dateMenu->date());
    }
    viderFormulaire();
    refresh();
}

void MenuPage::onModifier()
{
    const int id = idSelectionne();
    if (id <= 0) {
        QMessageBox::information(this, QStringLiteral("Information"),
                                 QStringLiteral("Sélectionnez un menu à modifier."));
        return;
    }

    QString error;
    if (!m_menuManager->modifier(id, m_spinQuantite->value(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Modification impossible"), error);
        return;
    }

    QMessageBox::information(this, QStringLiteral("Succès"),
                             QStringLiteral("La quantité initiale du menu a été mise à jour."));
    viderFormulaire();
    refresh();
}

void MenuPage::onSupprimer()
{
    const int id = idSelectionne();
    if (id <= 0) {
        QMessageBox::information(this, QStringLiteral("Information"),
                                 QStringLiteral("Sélectionnez un menu à retirer."));
        return;
    }

    const int row = m_table->selectedItems().first()->row();
    const QString nom = m_table->item(row, 1) ? m_table->item(row, 1)->text() : QString();

    const QMessageBox::StandardButton reponse = QMessageBox::question(
        this, QStringLiteral("Confirmer la suppression"),
        QStringLiteral("Retirer le plat « %1 » du menu du %2 ?")
            .arg(nom, formatDate(m_dateFiltre->date())),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reponse != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!m_menuManager->supprimer(id, &error)) {
        QMessageBox::warning(this, QStringLiteral("Suppression impossible"), error);
        return;
    }

    QMessageBox::information(this, QStringLiteral("Succès"),
                             QStringLiteral("Le plat a été retiré du menu."));
    viderFormulaire();
    refresh();
}

void MenuPage::onVider()
{
    viderFormulaire();
    m_table->clearSelection();
}

void MenuPage::viderFormulaire()
{
    m_cboPlat->setCurrentIndex(m_cboPlat->count() > 0 ? 0 : -1);
    m_dateMenu->setDate(m_dateFiltre ? m_dateFiltre->date() : QDate::currentDate());
    m_spinQuantite->setValue(20);
    m_btnModifier->setEnabled(false);
    m_btnSupprimer->setEnabled(false);
}
