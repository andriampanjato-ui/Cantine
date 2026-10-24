#include "mainwindow.h"
#include "database.h"
#include "dashboard.h"
#include "clientspage.h"
#include "platspage.h"
#include "menupage.h"
#include "neworderpage.h"
#include "historypage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>
#include <QStatusBar>
#include <QIntValidator>
#include <QFrame>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    loadSettings();
    checkDatabase();

    if (Database::instance()->isConnected()) {
        setPage(0);
    } else {
        setPage(6);
    }

    resize(1280, 820);
    setMinimumSize(1100, 700);
    setWindowTitle(QStringLiteral("Cantine Universitaire — Gestion des commandes"));
}

void MainWindow::setupUi()
{
    m_centralWidget = new QWidget(this);
    m_centralWidget->setObjectName(QStringLiteral("centralWidget"));
    setCentralWidget(m_centralWidget);

    auto *mainLayout = new QHBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_sideMenu = new QFrame(m_centralWidget);
    m_sideMenu->setObjectName(QStringLiteral("sideMenu"));
    m_sideMenu->setFixedWidth(235);
    mainLayout->addWidget(m_sideMenu);

    m_stackedWidget = new QStackedWidget(m_centralWidget);
    mainLayout->addWidget(m_stackedWidget, 1);

    setupSidebar();
    setupPages();

    statusBar()->showMessage(QStringLiteral("Prêt"));
}

void MainWindow::setupSidebar()
{
    auto *layout = new QVBoxLayout(m_sideMenu);
    layout->setContentsMargins(0, 0, 0, 12);
    layout->setSpacing(2);

    auto *logo = new QLabel(QStringLiteral("🍽  CANTINE"), m_sideMenu);
    logo->setObjectName(QStringLiteral("logo"));
    auto *logoSub = new QLabel(QStringLiteral("UNIVERSITAIRE"), m_sideMenu);
    logoSub->setObjectName(QStringLiteral("logoSub"));
    layout->addWidget(logo);
    layout->addWidget(logoSub);
    layout->addSpacing(14);

    m_navButtons.clear();

    m_navButtons.append(creerBoutonMenu(QStringLiteral("🏠  Tableau de bord"), 0));
    m_navButtons.append(creerBoutonMenu(QStringLiteral("👥  Clients"), 1));
    m_navButtons.append(creerBoutonMenu(QStringLiteral("🍲  Plats"), 2));
    m_navButtons.append(creerBoutonMenu(QStringLiteral("📅  Menus journaliers"), 3));
    m_navButtons.append(creerBoutonMenu(QStringLiteral("🧾  Nouvelle commande"), 4));
    m_navButtons.append(creerBoutonMenu(QStringLiteral("📜  Historique"), 5));

    for (QPushButton *bouton : m_navButtons) {
        layout->addWidget(bouton);
    }

    layout->addStretch();

    auto *parametres = creerBoutonMenu(QStringLiteral("⚙  Paramètres de connexion"), 6);
    m_navButtons.append(parametres);
    layout->addWidget(parametres);

    auto *version = new QLabel(QStringLiteral("Version 1.0 — Qt Widgets / MySQL"), m_sideMenu);
    version->setObjectName(QStringLiteral("versionLabel"));
    layout->addWidget(version);
}

QPushButton *MainWindow::creerBoutonMenu(const QString &texte, int index)
{
    auto *bouton = new QPushButton(texte, m_sideMenu);
    bouton->setCheckable(true);
    bouton->setCursor(Qt::PointingHandCursor);
    bouton->setProperty("navIndex", index);
    connect(bouton, &QPushButton::clicked, this, [this, index]() { setPage(index); });
    return bouton;
}

void MainWindow::setupPages()
{
    m_dashboard = new Dashboard(this);
    m_clientsPage = new ClientsPage(this);
    m_platsPage = new PlatsPage(this);
    m_menuPage = new MenuPage(this);
    m_newOrderPage = new NewOrderPage(this);
    m_historyPage = new HistoryPage(this);

    m_stackedWidget->addWidget(m_dashboard);      // 0
    m_stackedWidget->addWidget(m_clientsPage);    // 1
    m_stackedWidget->addWidget(m_platsPage);      // 2
    m_stackedWidget->addWidget(m_menuPage);       // 3
    m_stackedWidget->addWidget(m_newOrderPage);   // 4
    m_stackedWidget->addWidget(m_historyPage);    // 5

    // ---- Page paramètres de connexion ----
    auto *pageParametres = new QWidget(this);
    auto *layout = new QVBoxLayout(pageParametres);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto *titre = new QLabel(QStringLiteral("Paramètres de connexion"), pageParametres);
    titre->setObjectName(QStringLiteral("titrePage"));
    layout->addWidget(titre);

    auto *sousTitre = new QLabel(
        QStringLiteral("Connexion MySQL via XAMPP. La base doit être créée avec le script "
                       "sql/cantine_universitaire.sql."), pageParametres);
    sousTitre->setObjectName(QStringLiteral("sousTitre"));
    layout->addWidget(sousTitre);

    auto *groupe = new QGroupBox(QStringLiteral("Serveur MySQL"), pageParametres);
    auto *form = new QFormLayout(groupe);
    m_txtHost = new QLineEdit(QStringLiteral("127.0.0.1"), groupe);
    m_txtPort = new QLineEdit(QStringLiteral("3306"), groupe);
    m_txtPort->setValidator(new QIntValidator(1, 65535, m_txtPort));
    m_txtBase = new QLineEdit(QStringLiteral("cantine_universitaire"), groupe);
    m_txtUser = new QLineEdit(QStringLiteral("root"), groupe);
    m_txtPassword = new QLineEdit(groupe);
    m_txtPassword->setEchoMode(QLineEdit::Password);
    m_txtPassword->setPlaceholderText(QStringLiteral("Laissez vide avec XAMPP par défaut"));

    form->addRow(QStringLiteral("Hôte :"), m_txtHost);
    form->addRow(QStringLiteral("Port :"), m_txtPort);
    form->addRow(QStringLiteral("Base de données :"), m_txtBase);
    form->addRow(QStringLiteral("Utilisateur :"), m_txtUser);
    form->addRow(QStringLiteral("Mot de passe :"), m_txtPassword);

    auto *boutons = new QHBoxLayout;
    auto *btnTester = new QPushButton(QStringLiteral("🔌 Tester la connexion"), groupe);
    auto *btnConnecter = new QPushButton(QStringLiteral("✔ Se connecter"), groupe);
    btnConnecter->setObjectName(QStringLiteral("btnValider"));
    boutons->addWidget(btnTester);
    boutons->addWidget(btnConnecter);
    boutons->addStretch();
    form->addRow(boutons);

    layout->addWidget(groupe);

    auto *groupeEtat = new QGroupBox(QStringLiteral("État"), pageParametres);
    auto *etatLayout = new QVBoxLayout(groupeEtat);
    m_lblPilotes = new QLabel(groupeEtat);
    m_lblPilotes->setWordWrap(true);
    m_lblPilotes->setText(
        QStringLiteral("Pilote QMYSQL : %1\nPilotes disponibles : %2")
            .arg(Database::isMysqlDriverAvailable() ? QStringLiteral("disponible")
                                                    : QStringLiteral("indisponible"),
                 Database::availableDrivers().join(QStringLiteral(", "))));
    if (!Database::isMysqlDriverAvailable()) {
        m_lblPilotes->setStyleSheet(QStringLiteral("color: #b91c1c;"));
    }
    etatLayout->addWidget(m_lblPilotes);

    m_lblEtatConnexion = new QLabel(
        QStringLiteral("Non connecté."), groupeEtat);
    m_lblEtatConnexion->setWordWrap(true);
    m_lblEtatConnexion->setStyleSheet(QStringLiteral("color: #b91c1c; font-weight: bold;"));
    etatLayout->addWidget(m_lblEtatConnexion);
    layout->addWidget(groupeEtat);

    layout->addStretch();

    m_stackedWidget->addWidget(pageParametres);   // 6

    connect(btnTester, &QPushButton::clicked, this, &MainWindow::testerConnexion);
    connect(btnConnecter, &QPushButton::clicked, this, &MainWindow::seConnecter);
}

void MainWindow::loadSettings()
{
    QSettings settings(QStringLiteral("CantineUniversitaire"), QStringLiteral("GestionCantine"));
    m_txtHost->setText(settings.value(QStringLiteral("db/host"),
                                      QStringLiteral("127.0.0.1")).toString());
    m_txtPort->setText(settings.value(QStringLiteral("db/port"),
                                      QStringLiteral("3306")).toString());
    m_txtBase->setText(settings.value(QStringLiteral("db/name"),
                                      QStringLiteral("cantine_universitaire")).toString());
    m_txtUser->setText(settings.value(QStringLiteral("db/user"),
                                      QStringLiteral("root")).toString());
    m_txtPassword->setText(settings.value(QStringLiteral("db/password")).toString());
}

void MainWindow::saveSettings()
{
    QSettings settings(QStringLiteral("CantineUniversitaire"), QStringLiteral("GestionCantine"));
    settings.setValue(QStringLiteral("db/host"), m_txtHost->text().trimmed());
    settings.setValue(QStringLiteral("db/port"), m_txtPort->text().trimmed());
    settings.setValue(QStringLiteral("db/name"), m_txtBase->text().trimmed());
    settings.setValue(QStringLiteral("db/user"), m_txtUser->text().trimmed());
    settings.setValue(QStringLiteral("db/password"), m_txtPassword->text());
}

void MainWindow::checkDatabase()
{
    const bool ok = Database::instance()->connectToDatabase(
        m_txtHost->text().trimmed(),
        m_txtPort->text().toInt(),
        m_txtBase->text().trimmed(),
        m_txtUser->text().trimmed(),
        m_txtPassword->text());

    if (ok) {
        m_lblEtatConnexion->setText(
            QStringLiteral("Connexion réussie à la base « %1 » sur %2:%3.")
                .arg(m_txtBase->text(), m_txtHost->text(), m_txtPort->text()));
        m_lblEtatConnexion->setStyleSheet(QStringLiteral("color: #16a34a; font-weight: bold;"));
        statusBar()->showMessage(QStringLiteral("Connecté à MySQL"));
    } else {
        m_lblEtatConnexion->setText(
            QStringLiteral("Connexion impossible.\n%1").arg(Database::instance()->lastError()));
        m_lblEtatConnexion->setStyleSheet(QStringLiteral("color: #b91c1c; font-weight: bold;"));
        statusBar()->showMessage(QStringLiteral("Non connecté à MySQL"));
    }
}

bool MainWindow::validerParametres()
{
    if (m_txtHost->text().trimmed().isEmpty()
        || m_txtBase->text().trimmed().isEmpty()
        || m_txtUser->text().trimmed().isEmpty()) {
        QMessageBox::warning(
            this, QStringLiteral("Paramètres incomplets"),
            QStringLiteral("L'hôte, le nom de la base et l'utilisateur sont obligatoires."));
        return false;
    }

    bool portValide = false;
    const int port = m_txtPort->text().trimmed().toInt(&portValide);
    if (!portValide || port < 1 || port > 65535) {
        QMessageBox::warning(
            this, QStringLiteral("Port invalide"),
            QStringLiteral("Le port doit être un nombre entier compris entre 1 et 65535."));
        return false;
    }
    return true;
}

void MainWindow::testerConnexion()
{
    if (!validerParametres()) {
        return;
    }

    QString erreur;
    const bool ok = Database::testerParametres(
        m_txtHost->text().trimmed(),
        m_txtPort->text().toInt(),
        m_txtBase->text().trimmed(),
        m_txtUser->text().trimmed(),
        m_txtPassword->text(),
        &erreur);

    if (!ok) {
        statusBar()->showMessage(QStringLiteral("Test de connexion échoué"), 5000);
        QMessageBox::critical(
            this, QStringLiteral("Échec du test de connexion"),
            QStringLiteral("Les paramètres testés ne permettent pas de se connecter.\n\n"
                           "%1\n\n"
                           "La connexion actuelle n'a pas été modifiée.").arg(erreur));
        return;
    }

    QMessageBox::information(
        this, QStringLiteral("Test de connexion réussi"),
        QStringLiteral("Les paramètres de connexion sont valides.\n\n"
                       "Cliquez sur « Se connecter » pour les appliquer et enregistrer "
                       "la configuration."));
}

void MainWindow::seConnecter()
{
    if (!validerParametres()) {
        return;
    }

    const bool ok = Database::instance()->connectToDatabase(
        m_txtHost->text().trimmed(),
        m_txtPort->text().toInt(),
        m_txtBase->text().trimmed(),
        m_txtUser->text().trimmed(),
        m_txtPassword->text());

    if (!ok) {
        m_lblEtatConnexion->setText(
            QStringLiteral("Connexion impossible.\n%1").arg(Database::instance()->lastError()));
        m_lblEtatConnexion->setStyleSheet(QStringLiteral("color: #b91c1c; font-weight: bold;"));
        QMessageBox::critical(this, QStringLiteral("Échec de connexion"),
                              Database::instance()->lastError());
        return;
    }

    saveSettings();
    m_lblEtatConnexion->setText(
        QStringLiteral("Connexion réussie à la base « %1 ».").arg(m_txtBase->text()));
    m_lblEtatConnexion->setStyleSheet(QStringLiteral("color: #16a34a; font-weight: bold;"));
    statusBar()->showMessage(QStringLiteral("Connecté à MySQL"));
    QMessageBox::information(this, QStringLiteral("Connexion réussie"),
                             QStringLiteral("Connexion à la base réussie."));
    setPage(0);
}

void MainWindow::setPage(int index)
{
    if (index < 0 || index >= m_stackedWidget->count()) {
        return;
    }

    if (index != 6 && !Database::instance()->isConnected()) {
        QMessageBox::warning(
            this, QStringLiteral("Connexion requise"),
            QStringLiteral("La base de données n'est pas connectée.\n"
                           "Configurez la connexion dans « Paramètres de connexion »."));
        index = 6;
    }

    m_stackedWidget->setCurrentIndex(index);
    for (int i = 0; i < m_navButtons.size(); ++i) {
        QPushButton *bouton = m_navButtons.at(i);
        if (bouton) {
            bouton->setChecked(bouton->property("navIndex").toInt() == index);
        }
    }

    switch (index) {
    case 0:
        m_dashboard->refresh();
        break;
    case 1:
        m_clientsPage->refresh();
        break;
    case 2:
        m_platsPage->refresh();
        break;
    case 3:
        m_menuPage->refresh();
        break;
    case 4:
        m_newOrderPage->refresh();
        break;
    case 5:
        m_historyPage->refresh();
        break;
    case 6:
    default:
        break;
    }
}

void MainWindow::showDashboard() { setPage(0); }
void MainWindow::showClients() { setPage(1); }
void MainWindow::showPlats() { setPage(2); }
void MainWindow::showMenus() { setPage(3); }
void MainWindow::showNouvelleCommande() { setPage(4); }
void MainWindow::showHistorique() { setPage(5); }
void MainWindow::showParametres() { setPage(6); }
