#ifndef PLATSPAGE_H
#define PLATSPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QPushButton>

class QDoubleSpinBox;
class PlatManager;

class PlatsPage : public QWidget
{
    Q_OBJECT

public:
    explicit PlatsPage(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onSelectionChanged();
    void onAjouter();
    void onModifier();
    void onSupprimer();
    void onVider();

private:
    void viderFormulaire();
    int idSelectionne() const;

    PlatManager *m_manager;
    QLineEdit *m_txtRecherche;
    QTableWidget *m_table;
    QLineEdit *m_txtNom;
    QDoubleSpinBox *m_spinPrix;
    QPushButton *m_btnModifier;
    QPushButton *m_btnSupprimer;
};

#endif // PLATSPAGE_H
