#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QDir>

static QString chargerStyle()
{
    QFile styleFile(QStringLiteral(":/qss/style.qss"));
    if (styleFile.open(QFile::ReadOnly)) {
        QTextStream stream(&styleFile);
        const QString style = stream.readAll();
        styleFile.close();
        return style;
    }

    // Repli si la ressource n'est pas compilée : fichier à côté de l'exécutable.
    const QString chemin = QDir(QCoreApplication::applicationDirPath())
                               .filePath(QStringLiteral("qss/style.qss"));
    QFile fichier(chemin);
    if (fichier.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&fichier);
        return stream.readAll();
    }
    return QString();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QFont police(QStringLiteral("Segoe UI"));
    police.setPointSize(10);
    app.setFont(police);

    const QString style = chargerStyle();
    if (!style.isEmpty()) {
        app.setStyleSheet(style);
    }

    MainWindow fenetre;
    fenetre.show();

    return app.exec();
}
