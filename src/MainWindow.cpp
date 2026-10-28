#include "MainWindow.h"

#include "Belote.h"
#include "DialogChrome.h"
#include "HelpDialog.h"
#include "MenuStyle.h"
#include "NameDialog.h"
#include "Palette.h"
#include "ProfileChip.h"
#include "Profiles.h"
#include "SettingsDialog.h"
#include "TableLook.h"
#include "TablePicker.h"
#include "Tables.h"

#include <QAction>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int kMenuPage = 0;
constexpr int kTablePage = 1;
constexpr int kTopBarHeight = 50;

QToolButton *barButton(QWidget *parent, const QString &text, const QStringList &iconNames,
                       const QString &tip) {
    auto *button = new QToolButton(parent);
    button->setText(text);
    button->setToolTip(tip);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setCursor(Qt::PointingHandCursor);
    button->setAutoRaise(true);
    button->setIconSize(QSize(18, 18));
    button->setProperty("iconNames", iconNames);
    return button;
}

// De quelle couleur une étiquette de la bande est écrite.
//
// Dit par une feuille de style à elle plutôt que par une palette, ce qui est la
// façon évidente et ne marche pas : la bande porte une feuille de style, et un
// widget sous une feuille est repoli chaque fois que quelque chose change
// autour de lui -- chaque polissage lui rendant la palette qu'il avait quand la
// feuille est arrivée. Une règle sur le widget lui-même passe devant tout cela.
void inkLabel(QLabel *label, const QColor &colour) {
    const QString rule = QStringLiteral("color: %1;").arg(colour.name());
    if (label->styleSheet() == rule) return;
    label->setStyleSheet(rule);
}

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("Belotesol"));

    readSettings();
    // Le drap du profil, et ses cartes : tous deux lui appartiennent.
    setCurrentPalette(feltHue());
    applyProfileLook();

    m_picker = new TablePicker(this);
    connect(m_picker, &TablePicker::tableChosen, this, &MainWindow::startGame);

    m_table = new TablePage(this);
    connect(m_table, &TablePage::stateChanged, this, &MainWindow::updateTopBar);
    connect(m_table, &TablePage::wantsMenu, this, &MainWindow::showMenu);
    // La molette sur le drap change la taille des cartes : écrite tout de
    // suite, comme tout le reste de ce que la fenêtre retient.
    connect(m_table, &TablePage::zoomChanged, this, [this](int percent) {
        m_settings.zoom = percent;
        writeSettings();
    });

    m_pages = new QStackedWidget(this);
    m_pages->addWidget(m_picker);
    m_pages->addWidget(m_table);

    auto *centre = new QWidget(this);
    auto *rows = new QVBoxLayout(centre);
    rows->setContentsMargins(0, 0, 0, 0);
    rows->setSpacing(0);
    rows->addWidget(buildTopBar());
    rows->addWidget(m_pages, 1);
    setCentralWidget(centre);

    applyColours();
    updateTopBar();
    resize(1220, 850);
}

QWidget *MainWindow::buildTopBar() {
    m_topBar = new QWidget(this);
    m_topBar->setObjectName(QStringLiteral("topBar"));
    m_topBar->setFixedHeight(kTopBarHeight);

    m_title = new QLabel(tr("Belotesol"), m_topBar);
    QFont name = m_title->font();
    name.setBold(true);
    if (name.pointSizeF() > 0) name.setPointSizeF(name.pointSizeF() * 1.22);
    name.setLetterSpacing(QFont::PercentageSpacing, 106);
    m_title->setFont(name);

    m_subtitle = new QLabel(m_topBar);
    // Elle absorbe tout l'espace qui reste et rend ce qu'il faut quand il n'y
    // en a plus : sans cela, c'est la marque à droite qui se fait couper, et
    // une marque coupée se lit comme une autre marque.
    m_subtitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_profileChip = new ProfileChip(m_topBar);
    connect(m_profileChip, &ProfileChip::clicked, this, &MainWindow::showProfileMenu);

    m_score = new QLabel(m_topBar);
    QFont counter = m_score->font();
    counter.setBold(true);
    if (counter.pointSizeF() > 0) counter.setPointSizeF(counter.pointSizeF() * 1.16);
    counter.setStyleHint(QFont::Monospace);
    m_score->setFont(counter);
    m_score->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_againButton = barButton(m_topBar, tr("Nouvelle partie"),
                              {QStringLiteral("view-refresh-symbolic"),
                               QStringLiteral("view-refresh")},
                              tr("Une autre partie à la même table (F5)"));
    m_menuButton = barButton(m_topBar, tr("Menu"),
                             {QStringLiteral("go-home-symbolic"),
                              QStringLiteral("user-home-symbolic"),
                              QStringLiteral("go-previous-symbolic")},
                             tr("Revenir au choix de la table"));
    m_helpButton = barButton(m_topBar, tr("Règles"),
                             {QStringLiteral("help-contents-symbolic"),
                              QStringLiteral("help-about-symbolic"),
                              QStringLiteral("help-browser")},
                             tr("Les règles de la belote, et comment on joue à la souris"));
    m_settingsButton = barButton(m_topBar, tr("Réglages"),
                                 {QStringLiteral("emblem-system-symbolic"),
                                  QStringLiteral("preferences-system-symbolic")},
                                 tr("Les commodités, les cartes et la couleur du drap"));
    m_darkButton = barButton(m_topBar, QString(), {}, QString());

    auto *across = new QHBoxLayout(m_topBar);
    across->setContentsMargins(16, 0, 10, 0);
    across->setSpacing(6);
    across->addWidget(m_title);
    across->addSpacing(12);
    across->addWidget(m_subtitle, 1);
    across->addWidget(m_profileChip);
    across->addSpacing(14);
    across->addWidget(m_score);
    across->addSpacing(16);
    across->addWidget(m_againButton);
    across->addWidget(m_menuButton);
    across->addSpacing(10);
    across->addWidget(m_helpButton);
    across->addWidget(m_settingsButton);
    across->addWidget(m_darkButton);

    connect(m_menuButton, &QToolButton::clicked, this, &MainWindow::askLeaveMatch);
    connect(m_againButton, &QToolButton::clicked, this, &MainWindow::startSameTableAgain);
    connect(m_helpButton, &QToolButton::clicked, this, &MainWindow::openHelp);
    connect(m_settingsButton, &QToolButton::clicked, this, &MainWindow::openSettings);
    connect(m_darkButton, &QToolButton::clicked, this, &MainWindow::toggleDark);

    return m_topBar;
}

void MainWindow::startGame(const QString &tableId) {
    const TableKind &kind = tableKindById(tableId);
    m_settings.tableId = kind.id;
    m_settings.invited = m_picker->invited();

    m_table->startMatch(m_settings);
    m_pages->setCurrentIndex(kTablePage);
    m_table->setFocus();
    updateTopBar();
    writeSettings();
}

void MainWindow::startSameTableAgain() {
    if (m_settings.tableId.isEmpty()) return;
    startGame(m_settings.tableId);
}

void MainWindow::showMenu() {
    m_picker->reread();
    m_pages->setCurrentIndex(kMenuPage);
    updateTopBar();
}

void MainWindow::askLeaveMatch() {
    // La feuille de marque, elle, a déjà posé la question : elle appelle
    // `showMenu()` directement. Ici c'est le joueur qui s'en va de lui-même,
    // au milieu d'une donne -- et une partie quittée ne se reprend pas.
    const Phase phase = m_table->game()->phase();
    if (m_table->playing() && phase != Phase::MatchOver) {
        const auto answer =
            QMessageBox::question(this, tr("Belotesol"),
                                  tr("La partie en cours sera perdue. Revenir au menu ?"),
                                  QMessageBox::Cancel | QMessageBox::Yes);
        if (answer != QMessageBox::Yes) return;
    }
    showMenu();
}

void MainWindow::openSettings() {
    SettingsDialog dialog(m_settings, this);
    const bool kept = dialog.exec() == QDialog::Accepted;
    // La teinte s'essaie en direct dans la boîte : acceptée elle reste, et
    // annulée c'est celle du profil qui revient. Dans les deux cas la fenêtre
    // se repeint, puisque dans les deux cas elle a pu changer en chemin.
    setCurrentPalette(kept ? dialog.settings().hue : feltHue());
    applyColours();
    if (!kept) return;

    m_settings = dialog.settings();
    setFeltHue(m_settings.hue);
    writeSettings();
    m_table->applySettings(m_settings);
    m_picker->reread();
    updateTopBar();
}

void MainWindow::showProfileMenu() {
    QMenu menu(this);
    dressMenu(&menu);

    // Rien à éteindre pour cause de partie en cours : la plaque qui ouvre ce
    // menu n'est à l'écran qu'au menu d'ouverture.
    const int here = currentProfile();
    QVector<QAction *> switches;
    for (int slot = 1; slot <= kProfileCount; ++slot) {
        const ProfileRecord record = profileRecord(slot);
        QString line = record.name;
        if (record.matchesPlayed > 0) {
            line += tr("  —  %1 parties, %2 gagnées")
                        .arg(record.matchesPlayed)
                        .arg(record.matchesWon);
        } else {
            line += tr("  —  aucune partie encore");
        }
        QAction *pick = menu.addAction(line);
        pick->setCheckable(true);
        pick->setChecked(slot == here);
        pick->setEnabled(slot != here);
        switches.append(pick);
    }

    menu.addSeparator();
    QAction *rename = menu.addAction(tr("Renommer…"));
    QAction *forget = menu.addAction(tr("Tout oublier sur ce profil"));

    // Sous la plaque et alignée sur son bord gauche, comme un menu déroulant.
    const QAction *chosen =
        menu.exec(m_profileChip->mapToGlobal(QPoint(0, m_profileChip->height() + 4)));
    if (!chosen) return;

    const int picked = switches.indexOf(const_cast<QAction *>(chosen));
    if (picked >= 0) {
        setCurrentProfile(picked + 1);
        // Le drap, le jeu et le dos appartiennent au profil : ils changent avec
        // lui, sans quoi on jouerait la partie suivante avec le goût du
        // précédent.
        m_settings.hue = feltHue();
        setCurrentPalette(m_settings.hue);
        applyProfileLook();
        applyColours();
        profileChanged();
        m_picker->reread();
        return;
    }
    if (chosen == rename) {
        renameProfile(here);
    } else if (chosen == forget) {
        askForgetProfile(here);
    }
}

void MainWindow::renameProfile(int slot) {
    NameDialog dialog(tr("Renommer"), tr("Le nom de ce profil :"),
                      tr("Chaque profil garde ses parties et sa propre expérience des six "
                         "joueurs, et son propre drap."),
                      profileName(slot), kProfileNameLength, this);
    if (dialog.exec() != QDialog::Accepted) return;
    // Un nom vide rendrait la plaque muette : on garde alors l'ancien.
    if (dialog.name().isEmpty()) return;
    setProfileName(slot, dialog.name());
    profileChanged();
}

void MainWindow::askForgetProfile(int slot) {
    const ProfileRecord record = profileRecord(slot);
    const auto answer = QMessageBox::question(
        this, tr("Belotesol"),
        tr("Effacer les parties de « %1 » et remettre les six joueurs au niveau avec lequel "
           "ils arrivent ? Les deux autres profils ne sont pas touchés.")
            .arg(record.name),
        QMessageBox::Cancel | QMessageBox::Yes);
    if (answer != QMessageBox::Yes) return;
    forgetProfile(slot);
    profileChanged();
    m_picker->reread();
}

void MainWindow::profileChanged() {
    m_profileChip->reread();
    updateTopBar();
}

void MainWindow::openHelp() {
    HelpDialog dialog(this);
    dialog.exec();
}

void MainWindow::toggleDark() {
    setDarkMode(!darkMode());
    applyColours();
    writeSettings();
}

void MainWindow::applyColours() {
    const Palette &scheme = currentPalette();

    // La feuille de style de la bande d'abord, avant qu'une seule palette soit
    // distribuée. En poser une fait dépolir tout le sous-arbre par le style, et
    // rend à chaque widget la palette qu'il avait au premier polissage -- donc
    // une palette posée avant cet appel est une palette jetée.
    m_topBar->setStyleSheet(
        QStringLiteral("QWidget#topBar { background: %5; border-bottom: 1px solid %6; }"
                       "QToolButton { border: none; border-radius: 7px;"
                       " padding: 6px 10px; color: %1; background: transparent; }"
                       "QToolButton:hover { background: %2; color: %3; }"
                       "QToolButton:pressed { background: %4; }")
            .arg(scheme.faceText.name(), blendColours(scheme.face, scheme.accent, 0.22).name(),
                 scheme.faceText.name(),
                 blendColours(scheme.face, scheme.accent, 0.36).name(), scheme.face.name(),
                 scheme.edge.name()));

    QPalette window = palette();
    window.setColor(QPalette::Window, scheme.desk);
    window.setColor(QPalette::WindowText, scheme.surfaceText);
    window.setColor(QPalette::Base, scheme.field);
    window.setColor(QPalette::Text, scheme.surfaceText);
    window.setColor(QPalette::Button, scheme.face);
    window.setColor(QPalette::ButtonText, scheme.faceText);
    window.setColor(QPalette::Highlight, scheme.accent);
    window.setColor(QPalette::HighlightedText, scheme.accentText);
    setPalette(window);

    QPalette bar = m_topBar->palette();
    bar.setColor(QPalette::Window, scheme.face);
    bar.setColor(QPalette::WindowText, scheme.faceText);
    m_topBar->setPalette(bar);

    inkLabel(m_title, scheme.accent);
    inkLabel(m_subtitle, scheme.hint);
    m_profileChip->update();
    inkLabel(m_score, scheme.accent);

    m_darkButton->setToolTip(darkMode() ? tr("Couleurs claires") : tr("Couleurs sombres"));
    m_darkButton->setProperty(
        "iconNames", darkMode() ? QStringList{QStringLiteral("weather-clear-symbolic"),
                                              QStringLiteral("display-brightness-symbolic")}
                                : QStringList{QStringLiteral("weather-clear-night-symbolic"),
                                              QStringLiteral("night-light-symbolic")});

    for (QToolButton *button :
         {m_menuButton, m_againButton, m_helpButton, m_settingsButton, m_darkButton}) {
        const QStringList names = button->property("iconNames").toStringList();
        const QIcon icon = tintedIcon(names, scheme.faceText);
        button->setIcon(icon);
        if (button == m_darkButton) {
            button->setToolButtonStyle(icon.isNull() ? Qt::ToolButtonTextOnly
                                                     : Qt::ToolButtonIconOnly);
            button->setText(darkMode() ? tr("Clair") : tr("Sombre"));
        }
    }

    m_picker->refresh();
    m_table->refresh();
    update();
}

void MainWindow::updateTopBar() {
    const bool playing = m_pages->currentIndex() == kTablePage;
    m_menuButton->setVisible(playing);
    m_againButton->setVisible(playing);
    m_score->setVisible(playing);
    // La plaque du profil s'efface le temps de la partie : on n'en change pas
    // en cours de route, et la place est mieux employée par la marque.
    m_profileChip->setVisible(!playing);
    m_profileChip->reread();

    if (!playing) {
        setSubtitle(tr("La belote, à quatre et sur tapis vert"));
        return;
    }

    setSubtitle(m_table->subtitle());

    const Belote *game = m_table->game();
    if (!game || game->seatCount() < kSeatCount) {
        m_score->setText(QString());
        return;
    }
    int me = -1;
    for (int at = 0; at < game->seatCount(); ++at) {
        if (game->seat(at).human()) me = at;
    }
    if (me < 0) {
        m_score->setText(QString());
        return;
    }
    // Les deux marques, la vôtre devant. Ce sont des points gagnés et non des
    // pénalités : le plus possible.
    const int mine = game->teamScore(Belote::teamOf(me));
    const int theirs = game->teamScore(Belote::otherTeam(Belote::teamOf(me)));
    if (mine > theirs) {
        m_score->setText(tr("%1 – %2 — vous menez").arg(mine).arg(theirs));
    } else if (mine < theirs) {
        m_score->setText(tr("%1 – %2 — ils mènent").arg(mine).arg(theirs));
    } else {
        m_score->setText(tr("%1 – %2 — à égalité").arg(mine).arg(theirs));
    }
}

void MainWindow::setSubtitle(const QString &text) {
    m_subtitleText = text;
    elideSubtitle();
}

void MainWindow::elideSubtitle() {
    if (!m_subtitle) return;
    const QFontMetrics metrics(m_subtitle->font());
    const int room = std::max(60, m_subtitle->width());
    m_subtitle->setText(metrics.elidedText(m_subtitleText, Qt::ElideRight, room));
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    elideSubtitle();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Escape:
        if (m_pages->currentIndex() == kTablePage) {
            askLeaveMatch();
            return;
        }
        break;
    case Qt::Key_F5:
    case Qt::Key_F2:
        if (m_pages->currentIndex() == kTablePage) {
            startSameTableAgain();
            return;
        }
        break;
    case Qt::Key_F1:
        openHelp();
        return;
    default:
        break;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::readSettings() {
    QSettings settings;
    m_settings.tableId =
        settings.value(QStringLiteral("table/longueur"), defaultTableId()).toString();
    // Une table venue d'un fichier de réglages qui n'est pas une des tables
    // offertes -- écrite par une version plus tardive, ou éditée à la main --
    // n'est pas une table où l'on s'assoit.
    if (tableKindById(m_settings.tableId).id != m_settings.tableId) {
        m_settings.tableId = defaultTableId();
    }
    m_settings.variant = settings.value(QStringLiteral("regles/coinche"), false).toBool()
                             ? Variant::Coinche
                             : Variant::Classique;
    m_settings.annonces = settings.value(QStringLiteral("regles/annonces"), true).toBool();
    m_settings.holdByStrength = settings.value(QStringLiteral("aide/parForce"), false).toBool();
    m_settings.offerHints = settings.value(QStringLiteral("aide/conseil"), true).toBool();
    m_settings.celebrate = settings.value(QStringLiteral("aide/feu"), true).toBool();
    m_settings.pace = qBound(0, settings.value(QStringLiteral("aide/rythme"), 1).toInt(), 2);
    m_settings.zoom = qBound(80, settings.value(QStringLiteral("aide/zoom"), 100).toInt(), 200);
    setDarkMode(settings.value(QStringLiteral("look/sombre"), false).toBool());
    // La teinte du drap n'est pas rangée là : elle appartient au profil, et
    // c'est TableLook qui la garde sous lui.
    m_settings.hue = feltHue();
}

void MainWindow::writeSettings() {
    QSettings settings;
    settings.setValue(QStringLiteral("table/longueur"), m_settings.tableId);
    settings.setValue(QStringLiteral("regles/coinche"),
                      m_settings.variant == Variant::Coinche);
    settings.setValue(QStringLiteral("regles/annonces"), m_settings.annonces);
    settings.setValue(QStringLiteral("aide/parForce"), m_settings.holdByStrength);
    settings.setValue(QStringLiteral("aide/conseil"), m_settings.offerHints);
    settings.setValue(QStringLiteral("aide/feu"), m_settings.celebrate);
    settings.setValue(QStringLiteral("aide/rythme"), m_settings.pace);
    settings.setValue(QStringLiteral("aide/zoom"), m_settings.zoom);
    settings.setValue(QStringLiteral("look/sombre"), darkMode());
}
