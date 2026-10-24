#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QVector>

class QLineEdit;
class Dashboard;
class ClientsPage;
class PlatsPage;
class MenuPage;
class NewOrderPage;
class HistoryPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void showDashboard();
    void showClients();
    void showPlats();
    void showMenus();
    void showNouvelleCommande();
    void showHistorique();
    void showParametres();
    void testerConnexion();
    void seConnecter();

private:
    void setupUi();
    void setupSidebar();
    void setupPages();
    QPushButton *creerBoutonMenu(const QString &texte, int index);
    void setPage(int index);
    void loadSettings();
    void saveSettings();
    void checkDatabase();
    bool validerParametres();

    QWidget *m_centralWidget;
    QFrame *m_sideMenu;
    QStackedWidget *m_stackedWidget;

    QVector<QPushButton *> m_navButtons;

    Dashboard *m_dashboard;
    ClientsPage *m_clientsPage;
    PlatsPage *m_platsPage;
    MenuPage *m_menuPage;
    NewOrderPage *m_newOrderPage;
    HistoryPage *m_historyPage;

    // Paramètres de connexion
    QLineEdit *m_txtHost;
    QLineEdit *m_txtPort;
    QLineEdit *m_txtBase;
    QLineEdit *m_txtUser;
    QLineEdit *m_txtPassword;
    QLabel *m_lblEtatConnexion;
    QLabel *m_lblPilotes;
};

#endif // MAINWINDOW_H
