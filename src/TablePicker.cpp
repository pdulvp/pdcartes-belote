#include "TablePicker.h"

#include "Cards.h"
#include "DialogChrome.h"
#include "MenuStyle.h"
#include "NameDialog.h"
#include "Palette.h"
#include "Profiles.h"
#include "RivalCard.h"
#include "Rivals.h"
#include "Tables.h"
#include "TileButton.h"

#include <QAction>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

const QSize kTableTile(200, 182);

QLabel *heading(QWidget *parent, const QString &text, qreal scale, bool bold) {
    auto *label = new QLabel(text, parent);
    QFont font = label->font();
    font.setBold(bold);
    if (font.pointSizeF() > 0) font.setPointSizeF(font.pointSizeF() * scale);
    label->setFont(font);
    return label;
}

} // namespace

TablePicker::TablePicker(QWidget *parent) : QWidget(parent) {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *scroller = new QScrollArea(this);
    scroller->setWidgetResizable(true);
    scroller->setFrameShape(QFrame::NoFrame);
    scroller->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    outer->addWidget(scroller);

    auto *page = new QWidget(scroller);
    scroller->setWidget(page);

    auto *rows = new QVBoxLayout(page);
    rows->setContentsMargins(30, 24, 30, 26);
    rows->setSpacing(0);

    m_title = heading(page, tr("Belotesol"), 2.1, true);
    m_subtitle = heading(page, tr("La belote, à quatre et sur tapis vert, avec un partenaire "
                                  "qui apprend d'une partie à l'autre."),
                         1.0, false);
    rows->addWidget(m_title);
    rows->addSpacing(2);
    rows->addWidget(m_subtitle);
    rows->addSpacing(22);

    // Les tables.
    rows->addWidget(dialogSectionHeading(page, tr("La table")));
    rows->addSpacing(8);
    auto *tiles = new QHBoxLayout;
    tiles->setSpacing(10);
    for (const TableKind &kind : tableKinds()) {
        auto *tile = new TileButton(page);
        tile->setText(kind.name);
        tile->setBlurb(kind.blurb);
        tile->setTileSize(kTableTile);
        tile->setCorner(kind.points > 0 ? tr("%1 pts").arg(kind.points) : tr("1 donne"));
        const QString id = kind.id;
        connect(tile, &TileButton::clicked, this, [this, id] { Q_EMIT tableChosen(id); });
        m_tiles.append(tile);
        tiles->addWidget(tile);
    }
    tiles->addStretch();
    rows->addLayout(tiles);
    rows->addSpacing(6);
    m_footer = new QLabel(page);
    m_footer->setWordWrap(true);
    markHintLabel(m_footer);
    rows->addWidget(m_footer);
    rows->addSpacing(24);

    // Les six.
    rows->addWidget(dialogSectionHeading(page, tr("Les joueurs")));
    rows->addSpacing(8);
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);
    int at = 0;
    for (const Rival &rival : rivals()) {
        auto *card = new RivalCard(rival.id, page);
        card->setPickable(true);
        card->setToolTip(tr("Cliquer pour inviter %1 à la table ; clic droit pour le "
                            "renommer ou le remettre à son niveau de départ.")
                             .arg(rivalName(rival.id)));
        const QString id = rival.id;
        connect(card, &RivalCard::invitedChanged, this, [this, card, id] {
            // L'ordre des invitations est gardé ici : c'est lui qui décide qui
            // s'assoit en face, et une case cochée ne le dit pas.
            m_order.removeAll(id);
            if (card->invited()) m_order.append(id);
            reread();
        });
        connect(card, &RivalCard::menuWanted, this,
                [this, card](const QPoint &where) { showRivalMenu(card, where); });
        grid->addWidget(card, at / 3, at % 3);
        m_cards.append(card);
        ++at;
    }
    rows->addLayout(grid);
    rows->addSpacing(6);
    m_rivalNote = new QLabel(page);
    m_rivalNote->setWordWrap(true);
    markHintLabel(m_rivalNote);
    rows->addWidget(m_rivalNote);
    rows->addStretch(1);

    reread();
}

QVector<QString> TablePicker::invited() const { return m_order; }

void TablePicker::showRivalMenu(RivalCard *card, const QPoint &where) {
    const Rival &who = rivalById(card->rivalId());
    const Learning learning = learningFor(who.id);

    QMenu menu(this);
    dressMenu(&menu);
    const QString called = rivalName(who.id);
    QAction *invite = menu.addAction(card->invited() ? tr("Ne pas inviter %1").arg(called)
                                                     : tr("Inviter %1 à la table").arg(called));
    menu.addSeparator();
    QAction *rename = menu.addAction(tr("Renommer…"));
    menu.addSeparator();
    QAction *reset = menu.addAction(tr("Niveau de départ"));
    // Rien à remettre quand il n'a rien appris : l'entrée le dit en étant
    // éteinte, plutôt qu'en posant une question dont la réponse ne fait rien.
    reset->setEnabled(learning.xp > who.startXp);
    reset->setToolTip(
        tr("Remettre %1 au niveau avec lequel il arrive, sur ce profil seulement")
            .arg(called));

    const QAction *chosen = menu.exec(where);
    if (chosen == invite) {
        card->setInvited(!card->invited());
    } else if (chosen == rename) {
        renameRival(who.id);
    } else if (chosen == reset) {
        askResetRival(who.id);
    }
}

void TablePicker::renameRival(const QString &rivalId) {
    NameDialog dialog(tr("Renommer"), tr("Comment s'appelle ce joueur ?"),
                      tr("Son médaillon en prend l'initiale. Il garde sa couleur, son "
                         "expérience et sa façon de jouer. Laissez vide pour lui rendre "
                         "son nom."),
                      rivalName(rivalId), kRivalNameLength, this);
    if (dialog.exec() != QDialog::Accepted) return;
    // Un nom vide le fait répondre au sien : c'est setRivalName qui s'en
    // charge, et c'est la façon de le débaptiser.
    setRivalName(rivalId, dialog.name());
    reread();
}

void TablePicker::askResetRival(const QString &rivalId) {
    const Rival &who = rivalById(rivalId);
    const auto answer = QMessageBox::question(
        this, tr("Belotesol"),
        tr("Remettre %1 à %2 points d'expérience (%3), sur « %4 » ?")
            .arg(rivalName(rivalId))
            .arg(who.startXp)
            .arg(levelName(levelOf(who.startXp)).toLower(), profileName(currentProfile())),
        QMessageBox::Cancel | QMessageBox::Yes);
    if (answer != QMessageBox::Yes) return;
    resetRival(rivalId);
    reread();
}

void TablePicker::reread() {
    m_footer->setText(tr("Presser une table lance la partie. Chacun reçoit cinq cartes, une "
                         "est retournée, et celui qui la prend ramasse huit cartes et un "
                         "contrat : quatre-vingt-deux des cent soixante-deux points de la "
                         "donne."));

    // Les cartes disent où chacun s'assoira, pour que le partenaire ne soit
    // pas une surprise de la première donne.
    QStringList names;
    for (int i = 0; i < m_order.size(); ++i) names.append(rivalName(m_order.at(i)));
    for (RivalCard *card : m_cards) {
        const int rank = m_order.indexOf(card->rivalId());
        card->setInvited(rank >= 0);
        if (rank < 0) {
            card->setSeatNote(QString());
        } else if (rank == 0) {
            card->setSeatNote(tr("en face de vous"));
        } else if (rank < kSeatCount - 1) {
            card->setSeatNote(tr("contre vous"));
        } else {
            card->setSeatNote(tr("pas de place"));
        }
        card->reread();
    }

    if (names.isEmpty()) {
        m_rivalNote->setText(tr("Les trois places vont aux moins joués, pour que les six "
                                "progressent ensemble. Cliquez sur une carte pour inviter "
                                "quelqu'un : le premier invité s'assoit en face de vous, et "
                                "c'est donc lui qui joue avec vous."));
        return;
    }
    m_rivalNote->setText(tr("Votre partenaire : %1. %2Les places qui restent vont aux moins "
                            "joués.")
                             .arg(names.first(),
                                  names.size() > 1
                                      ? tr("Contre vous : %1. ")
                                            .arg(QStringList(names.mid(1, kSeatCount - 2))
                                                     .join(QStringLiteral(", ")))
                                      : QString()));
}

void TablePicker::refresh() {
    const Palette &scheme = currentPalette();
    QPalette page = palette();
    page.setColor(QPalette::Window, scheme.desk);
    page.setColor(QPalette::WindowText, scheme.surfaceText);
    setPalette(page);
    setAutoFillBackground(true);

    m_title->setStyleSheet(QStringLiteral("color: %1;").arg(scheme.accent.name()));
    m_subtitle->setStyleSheet(QStringLiteral("color: %1;").arg(scheme.hint.name()));
    for (QLabel *note : {m_rivalNote, m_footer}) {
        note->setStyleSheet(QStringLiteral("color: %1;").arg(scheme.hint.name()));
    }
    recolourSectionHeadings(this);
    reread();
    update();
}
