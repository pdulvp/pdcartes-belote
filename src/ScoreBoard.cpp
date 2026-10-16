#include "ScoreBoard.h"

#include "Belote.h"
#include "CardArt.h"
#include "Palette.h"

#include <QFontMetricsF>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>

#include <algorithm>

namespace {

constexpr qreal kPad = 26.0;
constexpr qreal kRowHeight = 32.0;
constexpr qreal kNameWidth = 196.0;
constexpr qreal kNumberWidth = 74.0;
constexpr int kColumns = 4;

} // namespace

ScoreBoard::ScoreBoard(QWidget *parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NoSystemBackground);
    hide();

    m_onwards = new QPushButton(this);
    m_away = new QPushButton(this);
    m_onwards->setCursor(Qt::PointingHandCursor);
    m_away->setCursor(Qt::PointingHandCursor);
    connect(m_onwards, &QPushButton::clicked, this, &ScoreBoard::goOn);
    connect(m_away, &QPushButton::clicked, this, &ScoreBoard::leave);
}

void ScoreBoard::showDonne(const Belote *game) {
    if (!game) return;
    m_matchOver = false;
    m_youWon = false;
    m_lines.clear();
    m_notes.clear();

    const DonneResult &done = game->result();
    int me = 0;
    for (int at = 0; at < game->seatCount(); ++at) {
        if (game->seat(at).human()) me = at;
    }
    const int myTeam = Belote::teamOf(me);

    // Votre camp d'abord : une feuille de marque se lit en commençant par sa
    // propre ligne.
    for (int step = 0; step < kTeamCount; ++step) {
        const int team = step == 0 ? myTeam : Belote::otherTeam(myTeam);
        Line line;
        line.name = game->teamName(team);
        line.ours = team == myTeam;
        line.hue = line.ours ? currentPalette().hue
                             : game->seat((me + 1) % kSeatCount).hue;
        line.tricks = done.tricks[team];
        line.points = done.points[team];
        line.gained = done.gained[team];
        line.total = game->teamScore(team);
        line.winner = done.gained[team] > done.gained[Belote::otherTeam(team)];
        m_lines.append(line);
    }

    if (done.annulled) {
        m_headline = tr("Donne annulée");
        m_aside = tr("Personne n'a voulu de la retourne, aux deux tours : c'est au suivant "
                     "de donner.");
        m_lines.clear();
        m_onwards->setText(tr("Redonner"));
        m_away->setText(tr("Quitter la table"));
        dressButtons();
        show();
        raise();
        setFocus();
        placeButtons();
        update();
        return;
    }

    const int takerTeam = Belote::teamOf(done.taker);
    const bool ourContract = takerTeam == myTeam;
    const bool coinche = done.variant == Variant::Coinche;

    if (done.litige) {
        m_headline = tr("Litige");
    } else if (coinche && !done.made) {
        m_headline = ourContract ? tr("Chute") : tr("Ils chutent");
    } else if (done.capot && done.made) {
        m_headline = ourContract ? tr("Capot !") : tr("Capot contre vous");
    } else if (done.capot) {
        m_headline = ourContract ? tr("Capot contre vous") : tr("Capot !");
    } else if (done.made) {
        m_headline = ourContract ? tr("Contrat tenu") : tr("Ils tiennent leur contrat");
    } else {
        m_headline = ourContract ? tr("Dedans") : tr("Ils sont dedans");
    }

    const QString name = game->seat(done.taker).human() ? tr("vous") : game->seat(done.taker).name;
    QString took;
    if (coinche) {
        took = done.contract >= kCapotBid
                   ? tr("capot %1 par %2").arg(suitWithArticle(done.trump), name)
                   : tr("%1 %2 par %3")
                         .arg(done.contract)
                         .arg(suitWithArticle(done.trump), name);
        if (done.coinche >= 4) {
            took += tr(", surcoinché");
        } else if (done.coinche == 2) {
            took += tr(", coinché");
        }
    } else {
        took = game->seat(done.taker).human()
                   ? tr("vous avez pris %1").arg(suitWithArticle(done.trump))
                   : tr("%1 a pris %2").arg(name, suitWithArticle(done.trump));
    }
    m_aside = tr("Donne %1 — %2").arg(game->donneNumber()).arg(took);

    if (done.beloteTeam >= 0) {
        m_notes.append(done.beloteTeam == myTeam ? tr("belote et rebelote pour vous : 20 points")
                                                 : tr("belote et rebelote pour eux : 20 points"));
    }
    if (done.annonceTeam >= 0) {
        const int points = done.annonces[done.annonceTeam];
        // Dit en entier, parce que c'est la règle des annonces qui surprend :
        // le camp qui tient la plus forte prend tout, l'autre rien.
        QString said = done.annonceTeam == myTeam
                           ? tr("vos annonces l'emportent : %1 points, et les leurs ne comptent "
                                "pas")
                                 .arg(points)
                           : tr("leurs annonces l'emportent : %1 points, et les vôtres ne "
                                "comptent pas")
                                 .arg(points);
        if (done.annonceTeam == takerTeam && !done.made) {
            said += tr(" — perdues, le contrat a chuté");
        }
        m_notes.append(said);
    }
    if (coinche) {
        const int stake = done.contract * done.coinche;
        m_notes.append(done.made
                           ? tr("contrat tenu : %1 points de contrat en plus des plis")
                                 .arg(stake)
                           : tr("contrat chuté : les défenseurs prennent les %1 de la donne et "
                                "les %2 du contrat")
                                 .arg(kDonnePoints)
                                 .arg(stake));
    }
    if (done.dixDeDer >= 0) {
        m_notes.append(done.dixDeDer == myTeam ? tr("le dix de der est à vous")
                                               : tr("le dix de der est à eux"));
    }
    if (done.litige) {
        m_notes.append(tr("quatre-vingt-un chacun : les %1 points du preneur sont mis de côté "
                          "pour le camp qui réussira la donne suivante")
                           .arg(done.litigeHeld));
    }
    if (done.litigeTaken > 0) {
        m_notes.append(tr("et les %1 points du litige d'avant sont encaissés")
                           .arg(done.litigeTaken));
    }
    if (!coinche && !done.made && !done.capot && !done.litige) {
        m_notes.append(tr("il fallait %1 points : les défenseurs ramassent les %2 de la donne")
                           .arg(kContract)
                           .arg(kDonnePoints));
    }

    m_onwards->setText(tr("Donne suivante"));
    m_away->setText(tr("Quitter la table"));
    dressButtons();
    show();
    raise();
    setFocus();
    placeButtons();
    update();
}

void ScoreBoard::showMatch(const Belote *game) {
    if (!game) return;
    showDonne(game);
    m_matchOver = true;

    int me = 0;
    for (int at = 0; at < game->seatCount(); ++at) {
        if (game->seat(at).human()) me = at;
    }
    const int winner = game->matchWinner();
    m_youWon = winner >= 0 && Belote::teamOf(me) == winner;

    for (Line &line : m_lines) line.winner = false;
    if (winner >= 0 && !m_lines.isEmpty()) {
        const int which = m_youWon ? 0 : 1;
        m_lines[which].winner = true;
    }

    if (winner < 0) {
        m_headline = tr("Partie nulle");
    } else {
        m_headline = m_youWon ? tr("Partie gagnée") : tr("Partie perdue");
    }
    m_aside = tr("%1 donnes — %2 points contre %3")
                  .arg(game->donneNumber())
                  .arg(game->teamScore(Belote::teamOf(me)))
                  .arg(game->teamScore(Belote::otherTeam(Belote::teamOf(me))));

    m_onwards->setText(tr("Rejouer"));
    m_away->setText(tr("Quitter la table"));
    dressButtons();
    placeButtons();
    update();
}

void ScoreBoard::hideBoard() { hide(); }

void ScoreBoard::refresh() {
    dressButtons();
    update();
}

void ScoreBoard::dressButtons() {
    const Palette &scheme = currentPalette();
    const QString common = QStringLiteral("QPushButton { border-radius: 7px; padding: 8px 20px;"
                                          " font-weight: bold; border: 1px solid %1; }");
    m_onwards->setStyleSheet(
        common.arg(scheme.accent.name()) +
        QStringLiteral("QPushButton { background: %1; color: %2; }"
                       "QPushButton:hover { background: %3; }")
            .arg(scheme.accent.name(), scheme.accentText.name(),
                 blendColours(scheme.accent, QColor(Qt::white), 0.16).name()));
    m_away->setStyleSheet(common.arg(scheme.edge.name()) +
                          QStringLiteral("QPushButton { background: %1; color: %2; }"
                                         "QPushButton:hover { border: 1px solid %3; }")
                              .arg(scheme.surface.name(), scheme.surfaceText.name(),
                                   scheme.accent.name()));
}

QSizeF ScoreBoard::measure() const {
    const qreal rows = kRowHeight * m_lines.size();
    const qreal width = kPad * 2 + kNameWidth + kNumberWidth * kColumns;
    const qreal notes = m_notes.isEmpty() ? 0.0 : 18.0 * m_notes.size() + 8.0;
    const qreal height = kPad * 2 + 46.0 + 22.0 + 20.0 + rows + notes + 30.0 + 40.0;
    return QSizeF(width, height);
}

void ScoreBoard::placeButtons() {
    const QSizeF card = measure();
    QRectF box(0, 0, card.width(), card.height());
    box.moveCenter(QPointF(width() / 2.0, height() / 2.0));

    const qreal buttonTop = box.bottom() - kPad - 34.0;
    m_onwards->adjustSize();
    m_away->adjustSize();
    m_onwards->move(int(box.right() - kPad - m_onwards->width()), int(buttonTop));
    m_away->move(int(box.left() + kPad), int(buttonTop));
}

void ScoreBoard::resizeEvent(QResizeEvent *) { placeButtons(); }

void ScoreBoard::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        Q_EMIT goOn();
        return;
    case Qt::Key_Escape:
        Q_EMIT leave();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void ScoreBoard::paintEvent(QPaintEvent *) {
    const Palette &scheme = currentPalette();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QColor veil = scheme.feltEdge;
    veil.setAlpha(206);
    painter.fillRect(rect(), veil);

    const QSizeF size = measure();
    QRectF box(0, 0, size.width(), size.height());
    box.moveCenter(QPointF(width() / 2.0, height() / 2.0));

    QPainterPath shape;
    shape.addRoundedRect(box, 16, 16);
    painter.setPen(Qt::NoPen);
    painter.fillPath(shape, scheme.surface);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(m_matchOver && m_youWon ? scheme.gold : scheme.accent, 2.2));
    painter.drawPath(shape);

    const QRectF inner = box.adjusted(kPad, kPad, -kPad, -kPad);

    QFont headline = plainFont(&painter);
    headline.setBold(true);
    headline.setPixelSize(m_matchOver ? 26 : 21);
    painter.setFont(headline);
    painter.setPen(m_matchOver && m_youWon ? scheme.gold : scheme.accent);
    const QFontMetricsF headMetrics(headline);
    painter.drawText(QRectF(inner.left(), inner.top(), inner.width(), headMetrics.height()),
                     Qt::AlignCenter, m_headline);
    qreal y = inner.top() + headMetrics.height() + 3.0;

    QFont aside = plainFont(&painter);
    aside.setPixelSize(12);
    painter.setFont(aside);
    painter.setPen(scheme.hint);
    const QFontMetricsF asideMetrics(aside);
    painter.drawText(QRectF(inner.left(), y, inner.width(), asideMetrics.height() * 2.0),
                     Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, m_aside);
    y += asideMetrics.height() + 16.0;

    if (!m_lines.isEmpty()) {
        // L'en-tête des colonnes. Quatre chiffres par camp, et ils ne disent
        // pas la même chose : les plis pris, les points ramassés, les points
        // marqués, et le total.
        QFont heading = plainFont(&painter);
        heading.setBold(true);
        heading.setPixelSize(10);
        heading.setCapitalization(QFont::AllUppercase);
        heading.setLetterSpacing(QFont::PercentageSpacing, 120);
        painter.setFont(heading);
        painter.setPen(scheme.hint);
        const QStringList columns = {tr("plis"), tr("ramassés"), tr("marqués"), tr("total")};
        for (int i = 0; i < columns.size(); ++i) {
            painter.drawText(QRectF(inner.left() + kNameWidth + kNumberWidth * i, y,
                                    kNumberWidth, 14.0),
                             Qt::AlignRight | Qt::AlignVCenter, columns.at(i));
        }
        y += 16.0;
        painter.setPen(QPen(scheme.edge, 1.0));
        painter.drawLine(QPointF(inner.left(), y), QPointF(inner.right(), y));
        y += 3.0;

        QFont row = plainFont(&painter);
        row.setPixelSize(14);
        QFont rowBold = row;
        rowBold.setBold(true);

        for (const Line &line : m_lines) {
            const QRectF band(inner.left(), y, inner.width(), kRowHeight);
            if (line.winner) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(blendColours(scheme.surface, scheme.accent, 0.14));
                painter.drawRoundedRect(band.adjusted(-6, 1, 6, -1), 6, 6);
            }

            painter.setPen(Qt::NoPen);
            painter.setBrush(rivalColour(line.hue));
            painter.drawEllipse(QRectF(band.left(), band.center().y() - 4.5, 9.0, 9.0));

            painter.setFont(line.ours ? rowBold : row);
            painter.setPen(scheme.surfaceText);
            painter.drawText(QRectF(band.left() + 16.0, band.top(), kNameWidth - 16.0, kRowHeight),
                             Qt::AlignLeft | Qt::AlignVCenter, line.name);

            const int figures[kColumns] = {line.tricks, line.points, line.gained, line.total};
            for (int i = 0; i < kColumns; ++i) {
                const bool last = i == kColumns - 1;
                const bool marked = i == kColumns - 2;
                painter.setFont(last || marked ? rowBold : row);
                painter.setPen(marked && figures[i] == 0 ? scheme.danger
                                                         : (last && line.winner ? scheme.accent
                                                                                : scheme.surfaceText));
                painter.drawText(QRectF(band.left() + kNameWidth + kNumberWidth * i, band.top(),
                                        kNumberWidth, kRowHeight),
                                 Qt::AlignRight | Qt::AlignVCenter, QString::number(figures[i]));
            }
            y += kRowHeight;
        }
        y += 8.0;
    }

    if (m_notes.isEmpty()) return;
    QFont note = plainFont(&painter);
    note.setPixelSize(11);
    painter.setFont(note);
    painter.setPen(scheme.hint);
    for (const QString &said : m_notes) {
        painter.drawText(QRectF(inner.left(), y, inner.width(), 18.0),
                         Qt::AlignLeft | Qt::AlignVCenter, said);
        y += 18.0;
    }
}
