#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QIcon>
#include <QLocale>

#include "CardDeck.h"
#include "Fonts.h"
#include "MainWindow.h"
#include "Version.h"

namespace {

// Qt s'arrête sur le thème d'icônes « hicolor » quand le bureau ne lui a pas
// dit lequel utiliser, et hicolor ne contient presque rien -- ce qui laisse les
// boutons du haut nus. On pointe donc le recours vers un thème d'icônes qui est
// réellement installé.
void ensureIconTheme() {
    const QString configured = QIcon::themeName();
    if (!configured.isEmpty() && configured != QLatin1String("hicolor")) return;

    const QStringList candidates = {QStringLiteral("Adwaita"), QStringLiteral("breeze"),
                                    QStringLiteral("oxygen"), QStringLiteral("Papirus"),
                                    QStringLiteral("gnome")};
    for (const QString &name : candidates) {
        for (const QString &dir : QIcon::themeSearchPaths()) {
            if (!QFileInfo::exists(dir + QLatin1Char('/') + name +
                                   QLatin1String("/index.theme"))) {
                continue;
            }
            QIcon::setFallbackThemeName(name);
            return;
        }
    }
}

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("pdcartes-belote");
    QApplication::setApplicationName("pdcartes-belote");
    QApplication::setApplicationVersion(fullVersion());
    // Ce sous quoi le bureau connaît cette application. Sous Wayland c'est
    // l'app_id de la fenêtre, et un shell le rapproche du fichier .desktop du
    // même nom pour savoir à quelle application une fenêtre appartient -- ce
    // qui est ce qui permet de l'épingler.
    QApplication::setDesktopFileName(QStringLiteral("pdcartes-belote"));

    // Le jeu est en français, donc ses nombres et ses dates le sont aussi,
    // quoi que la machine soit réglée.
    QLocale::setDefault(QLocale(QLocale::French, QLocale::France));

    ensureIconTheme();
    // La fonte de tout ce qui se lit dans la fenêtre. Posée avant la première
    // fenêtre : un widget garde la fonte qu'il avait en naissant.
    dressApplicationFont();

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QCoreApplication::translate("main", "pdcartes-belote — la belote sur tapis vert."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    // Les jeux de cartes installés, lus une fois au démarrage.
    CardDecks::instance().refresh();

    MainWindow window;
    window.show();
    return app.exec();
}
