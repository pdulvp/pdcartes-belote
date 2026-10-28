#include "TablePage.h"

#include "FeltArt.h"
#include "Palette.h"
#include "Profiles.h"
#include "Rivals.h"
#include "ScoreBoard.h"
#include "TableView.h"
#include "VictoryEffect.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int kActionsHeight = 50;

// Combien de temps un adversaire a l'air de réfléchir. Pas pour faire semblant
// d'être lent : un coup qui arrive dans la même milliseconde que le précédent
// est un coup que personne n'a vu passer, et la moitié de la belote est de
// regarder ce que font les trois autres.
constexpr int kThinkMs = 620;

// Et combien de temps le pli reste sur la table avant d'être ramassé. Plus
// long que le reste, parce que c'est le moment où l'on compte : quatre cartes,
// qui les a jouées, et ce que ça fait.
constexpr int kSweepMs = 1150;

QPushButton *actionButton(QWidget *parent, const QString &text, const QString &tip,
                          const QString &keys) {
    auto *button = new QPushButton(text, parent);
    button->setToolTip(keys.isEmpty() ? tip : QStringLiteral("%1 (%2)").arg(tip, keys));
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

} // namespace

TablePage::TablePage(QWidget *parent) : QWidget(parent) {
    m_game = new Belote(this);
    m_view = new TableView(this);
    m_view->watch(m_game);

    auto *rows = new QVBoxLayout(this);
    rows->setContentsMargins(0, 0, 0, 0);
    rows->setSpacing(0);
    rows->addWidget(m_view, 1);
    rows->addWidget(buildActions());

    // Par-dessus la table, et seulement quand il y a lieu.
    m_board = new ScoreBoard(this);
    m_victory = new VictoryEffect(this);

    m_beat = new QTimer(this);
    m_beat->setSingleShot(true);
    connect(m_beat, &QTimer::timeout, this, &TablePage::playRival);

    m_sweep = new QTimer(this);
    m_sweep->setSingleShot(true);
    connect(m_sweep, &QTimer::timeout, this, &TablePage::sweep);

    connect(m_game, &Belote::changed, this, [this] {
        m_view->refresh();
        updateActions();
        Q_EMIT stateChanged();
    });
    connect(m_game, &Belote::turnBegan, this, &TablePage::onTurnBegan);
    connect(m_game, &Belote::acted, this, &TablePage::onActed);
    connect(m_game, &Belote::trickFinished, this, &TablePage::onTrickFinished);
    connect(m_game, &Belote::donneFinished, this, &TablePage::onDonneFinished);
    connect(m_game, &Belote::matchFinished, this, &TablePage::onMatchFinished);

    connect(m_view, &TableView::cardPressed, this, &TablePage::onCardPressed);
    connect(m_view, &TableView::cardDoubleClicked, this, &TablePage::onCardDoubleClicked);
    connect(m_view, &TableView::zoomChanged, this, [this](int percent) {
        m_settings.zoom = percent;
        Q_EMIT zoomChanged(percent);
    });

    connect(m_board, &ScoreBoard::goOn, this, [this] {
        m_board->hideBoard();
        m_victory->stop();
        if (m_game->phase() == Phase::MatchOver) {
            startMatch(m_settings);
        } else {
            m_donneCounted = false;
            m_game->startDonne();
        }
        setFocus();
    });
    connect(m_board, &ScoreBoard::leave, this, [this] {
        m_board->hideBoard();
        m_victory->stop();
        Q_EMIT wantsMenu();
    });
    connect(m_victory, &VictoryEffect::finished, this, [this] {
        m_victory->stop();
        m_board->raise();
        m_board->show();
    });

    setFocusPolicy(Qt::StrongFocus);
}

QWidget *TablePage::buildActions() {
    m_actions = new QWidget(this);
    m_actions->setObjectName(QStringLiteral("actionBar"));
    m_actions->setFixedHeight(kActionsHeight);

    m_state = new QLabel(m_actions);
    // Elle absorbe la place qui reste et rend ce qu'il faut quand il n'y en a
    // plus : à la coinche la rangée porte onze boutons, et c'est la phrase qui
    // doit céder, pas eux.
    m_state->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    m_sortSuit = actionButton(m_actions, tr("Par couleur"),
                              tr("Ranger la main par couleur, l'atout en tête"), tr("T"));
    m_sortStrength = actionButton(m_actions, tr("Par force"),
                                  tr("Ranger la main par force, les maîtresses en avant"),
                                  tr("F"));

    // Les quatre couleurs, dont une seule ou trois sont offertes selon le tour
    // d'enchère. Quatre boutons plutôt qu'un bouton et une liste : prendre est
    // la seule décision de l'enchère, et elle se prend d'un clic.
    m_less = actionButton(m_actions, tr("−"), tr("Annoncer moins"), tr("←"));
    m_more = actionButton(m_actions, tr("+"), tr("Annoncer plus"), tr("→"));
    for (int s = 0; s < 4; ++s) {
        const Suit suit = Suit(s);
        m_takeSuit[s] = actionButton(
            m_actions, tr("Prendre %1").arg(suitPip(suit)),
            tr("Prendre la donne, l'atout %1").arg(suitWithArticle(suit)),
            QString::number(s + 1));
        connect(m_takeSuit[s], &QPushButton::clicked, this, [this, suit] { doSuit(suit); });
    }
    m_capot = actionButton(m_actions, tr("Capot"),
                           tr("Annoncer les huit plis : deux cent cinquante points"), tr("0"));
    m_coinche = actionButton(m_actions, tr("Coincher"),
                             tr("Parier que le contrat ne passera pas : tout est doublé"),
                             tr("C"));
    m_pass = actionButton(m_actions, tr("Passer"), tr("Laisser parler le suivant"), tr("P"));
    m_play = actionButton(m_actions, tr("Jouer"), tr("Jouer la carte choisie"), tr("Entrée"));
    m_hint = actionButton(m_actions, tr("Conseil"), tr("Ce que le meilleur joueur ferait"),
                          tr("H"));

    auto *across = new QHBoxLayout(m_actions);
    across->setContentsMargins(14, 0, 14, 0);
    across->setSpacing(7);
    // L'étiquette prend toute la place qui reste plutôt qu'un espace vide :
    // c'est elle qui doit céder quand la rangée se remplit, mais tant qu'il y a
    // de la place elle la prend.
    across->addWidget(m_state, 1);
    across->addWidget(m_sortSuit);
    across->addWidget(m_sortStrength);
    across->addSpacing(12);
    across->addWidget(m_less);
    across->addWidget(m_more);
    for (QPushButton *button : m_takeSuit) across->addWidget(button);
    across->addWidget(m_capot);
    across->addWidget(m_coinche);
    across->addWidget(m_pass);
    across->addSpacing(12);
    across->addWidget(m_hint);
    across->addWidget(m_play);

    connect(m_sortSuit, &QPushButton::clicked, this, [this] { sort(false); });
    connect(m_sortStrength, &QPushButton::clicked, this, [this] { sort(true); });
    connect(m_less, &QPushButton::clicked, this, [this] { stepBid(-kBidStep); });
    connect(m_more, &QPushButton::clicked, this, [this] { stepBid(kBidStep); });
    connect(m_capot, &QPushButton::clicked, this, [this] { doBid(kCapotBid, m_game->bidSuit()); });
    connect(m_coinche, &QPushButton::clicked, this, &TablePage::doCoinche);
    connect(m_pass, &QPushButton::clicked, this, &TablePage::doPass);
    connect(m_play, &QPushButton::clicked, this, &TablePage::doPlay);
    connect(m_hint, &QPushButton::clicked, this, &TablePage::doHint);

    return m_actions;
}

// ---------------------------------------------------------------------------
// Qui est à table.

QVector<SeatSpec> TablePage::chooseSeats() const {
    // Vous à la place zéro, de sorte que la place deux -- celle d'en face --
    // est celle de votre partenaire, et que les places impaires sont le camp
    // d'en face.
    SeatSpec you;
    you.name = tr("Vous");
    // Votre couleur est celle du drap : elle appartient au profil et se choisit
    // dans les réglages, la table n'ayant plus de couleur à elle.
    you.hue = currentPalette().hue;

    // Ceux qu'on a demandés d'abord, puis les moins joués : comme ça les six
    // progressent à peu près ensemble au lieu de laisser les trois derniers
    // débutants pour toujours.
    QVector<QString> order;
    for (const QString &id : m_settings.invited) {
        if (order.size() >= kSeatCount - 1) break;
        if (!order.contains(id)) order.append(id);
    }

    QVector<Rival> rest;
    for (const Rival &rival : rivals()) {
        if (!order.contains(rival.id)) rest.append(rival);
    }
    std::stable_sort(rest.begin(), rest.end(), [](const Rival &a, const Rival &b) {
        return learningFor(a.id).donnesPlayed < learningFor(b.id).donnesPlayed;
    });
    for (const Rival &rival : rest) {
        if (order.size() >= kSeatCount - 1) break;
        order.append(rival.id);
    }

    // Le premier invité s'assoit en face de vous. C'est la seule chose qu'un
    // joueur de belote veut vraiment choisir : avec qui il joue, et non
    // contre qui.
    const QVector<int> places = {2, 1, 3};
    QVector<SeatSpec> round(kSeatCount);
    round[0] = you;
    for (int i = 0; i < order.size() && i < places.size(); ++i) {
        const Rival &rival = rivalById(order.at(i));
        SeatSpec spec;
        spec.rivalId = rival.id;
        // Le nom que ce profil-là lui donne, et non celui avec lequel il est
        // arrivé : c'est celui qu'on lit sur sa plaque tout au long de la
        // partie.
        spec.name = rivalName(rival.id);
        spec.hue = rival.hue;
        round[places.at(i)] = spec;
    }
    return round;
}

void TablePage::startMatch(const GameSettings &settings) {
    m_settings = settings;
    const TableKind &table = tableKindById(settings.tableId);

    m_board->hideBoard();
    m_victory->stop();
    m_beat->stop();
    m_sweep->stop();
    m_view->stopAnimations();
    m_view->setZoom(settings.zoom);
    m_donneCounted = false;
    m_dealShown = 0;

    const QVector<SeatSpec> seats = chooseSeats();
    m_skills.clear();
    for (const SeatSpec &spec : seats) {
        m_skills.append(spec.rivalId.isEmpty() ? bestSkill()
                                               : skillFor(learningFor(spec.rivalId).xp));
    }

    // La vue regarde la partie avant que la première donne soit distribuée :
    // startMatch() donne tout de suite, et une vue qui ne regarde encore rien
    // n'aurait pas de quoi montrer la distribution.
    m_view->watch(m_game);
    m_game->startMatch(seats, table.points, settings.variant, settings.annonces);
    m_view->clearChosen();
    updateActions();
    setFocus();
}

void TablePage::applySettings(const GameSettings &settings) {
    const QString table = m_settings.tableId;
    const Variant variant = m_settings.variant;
    const bool annonces = m_settings.annonces;
    m_settings = settings;
    // Ni la table ni les règles ne changent une partie en cours : jeter une
    // donne à moitié jouée parce qu'un réglage a bougé serait un mauvais
    // échange. Elles valent pour la prochaine.
    m_settings.tableId = table;
    m_settings.variant = variant;
    m_settings.annonces = annonces;
    m_hint->setVisible(m_settings.offerHints);
    m_view->setZoom(m_settings.zoom);
    // L'ordre de la main, lui, change tout de suite : on l'a changé en
    // regardant sa main, et attendre la donne suivante pour le poser serait
    // répondre à côté.
    if (m_game->trumpChosen()) sort(m_settings.holdByStrength);
    updateActions();
}

void TablePage::refresh() {
    const Palette &scheme = currentPalette();
    // La bande du bas porte les couleurs de celle du haut : ce sont les deux
    // bords de la même fenêtre, et une bande pâle sous un drap vert sombre
    // ressemblait à un panneau posé par une autre application.
    const QColor barFace = feltBarFace(scheme);
    const QColor barInk = scheme.feltText;
    QColor barAside = barInk;
    barAside.setAlphaF(0.62);
    m_actions->setStyleSheet(
        QStringLiteral("QWidget#actionBar { background: %1; border-top: 1px solid %2; }"
                       "QPushButton { background: %3; color: %4; border: 1px solid %2;"
                       " border-radius: 7px; padding: 5px 13px 7px 13px; }"
                       "QPushButton:hover:enabled { border: 1px solid %5; background: %6; }"
                       "QPushButton:pressed:enabled { background: %7; }"
                       "QPushButton:disabled { color: %2; background: %1; border-color: %1; }"
                       "QPushButton[strong=\"true\"]:enabled { background: %5; color: %8;"
                       " border: 1px solid %5; font-weight: bold; }"
                       "QLabel { color: %9; }")
            .arg(barFace.name(), feltGold(scheme, 0.30).name(QColor::HexArgb),
                 feltPlaqueFill(scheme).name(QColor::HexArgb), barInk.name(),
                 scheme.gold.name(), feltPlaqueFill(scheme, 0.14).name(QColor::HexArgb),
                 feltPlaqueFill(scheme, 0.26).name(QColor::HexArgb),
                 readableTextOn(scheme.gold).name(), barAside.name(QColor::HexArgb)));

    QPalette bar = m_actions->palette();
    bar.setColor(QPalette::Window, barFace);
    bar.setColor(QPalette::WindowText, barInk);
    m_actions->setPalette(bar);
    m_actions->setAutoFillBackground(true);

    m_board->refresh();
    m_view->refresh();
    updateActions();
}

int TablePage::mySeat() const {
    for (int at = 0; at < m_game->seatCount(); ++at) {
        if (m_game->seat(at).human()) return at;
    }
    return -1;
}

bool TablePage::playing() const {
    return m_game->seatCount() == kSeatCount && m_game->phase() != Phase::Idle;
}

QString TablePage::subtitle() const {
    if (!playing()) return QString();
    const TableKind &table = tableKindById(m_settings.tableId);
    if (m_game->phase() == Phase::MatchOver) {
        const int winner = m_game->matchWinner();
        if (winner < 0) return tr("%1 — partie nulle").arg(table.name);
        const int me = mySeat();
        return me >= 0 && Belote::teamOf(me) == winner
                   ? tr("%1 — vous gagnez la partie").arg(table.name)
                   : tr("%1 — %2 gagne la partie").arg(table.name, m_game->teamName(winner));
    }
    if (table.points > 0) {
        return tr("%1 — donne %2, en %3 points")
            .arg(table.name)
            .arg(m_game->donneNumber())
            .arg(table.points);
    }
    return tr("%1 — une seule donne").arg(table.name);
}

int TablePage::thinkingMs(bool first) const {
    const qreal part = m_settings.pace == 0 ? 1.7 : (m_settings.pace == 2 ? 0.42 : 1.0);
    return std::max(40, int((first ? kThinkMs : kThinkMs * 0.7) * part));
}

int TablePage::sweepMs() const {
    const qreal part = m_settings.pace == 0 ? 1.5 : (m_settings.pace == 2 ? 0.50 : 1.0);
    return std::max(120, int(kSweepMs * part));
}

// ---------------------------------------------------------------------------
// Le tour des autres.

void TablePage::onTurnBegan(int seat) {
    m_view->clearChosen();
    showDealIfNew();
    if (m_game->phase() == Phase::Bidding) {
        m_sortedDonne = false;
        // Le contrat a pu monter depuis la dernière fois qu'on a parlé.
        m_bidValue = qBound(kLowestBid, std::max(m_bidValue, m_game->lowestBid()), kHighestBid);
    } else if (!m_sortedDonne && m_game->phase() == Phase::Playing) {
        // L'atout vient d'être connu, de quelque façon qu'il l'ait été.
        m_sortedDonne = true;
        sortAsPreferred();
    }
    if (m_game->seat(seat).human()) {
        sayWhatToDo();
        updateActions();
        return;
    }
    m_view->say(m_game->phase() == Phase::Bidding
                    ? tr("%1 réfléchit à la retourne…").arg(m_game->seat(seat).name)
                    : tr("À %1 de jouer…").arg(m_game->seat(seat).name));
    // Le premier pli d'une donne mérite qu'on attende ; une parole d'enchère
    // et les plis suivants vont plus vite, parce qu'il y a huit fois moins à
    // regarder.
    m_beat->start(thinkingMs(m_game->phase() == Phase::Playing && m_game->trickNumber() <= 1));
}

void TablePage::onActed(int seat, const QString &tale) {
    Q_UNUSED(seat);
    if (!tale.isEmpty()) m_view->say(tale);
}

void TablePage::playRival() {
    const int at = m_game->turn();
    if (at < 0 || at >= m_skills.size()) return;
    if (m_game->seat(at).human()) return;

    if (m_game->phase() == Phase::Bidding) {
        const Bid bid = chooseBid(*m_game, at, m_skills.at(at));
        if (bid.coinche && m_game->coincher()) return;
        if (bid.take) {
            if (m_game->variant() == Variant::Coinche) {
                if (m_game->makeBid(bid.value, bid.suit)) return;
            } else {
                const QVector<int> before = cardsInHands();
                const int retourneId = m_game->retourne().id;
                if (m_game->take(bid.suit)) {
                    noteTakeDeal(before, retourneId);
                    return;
                }
            }
        }
        m_game->pass();
        return;
    }
    if (m_game->phase() != Phase::Playing) return;

    const int card = chooseCard(*m_game, at, m_skills.at(at));
    const Card *held = findCard(m_game->seat(at).hand, card);
    const Card played = held ? *held : Card{};
    if (card >= 0 && m_game->play(card)) {
        m_view->notePlayed(at, played);
        return;
    }

    // Un coup refusé, ce qui veut dire que le cerveau a demandé quelque chose
    // que les règles ne permettent pas. Jamais vu arriver, et c'est
    // précisément pourquoi il y a un filet : une partie bloquée est bien pire
    // qu'un coup mal joué.
    const QVector<int> allowed = m_game->legalCards(at);
    if (allowed.isEmpty()) return;
    const Card *fallback = findCard(m_game->seat(at).hand, allowed.first());
    const Card thrown = fallback ? *fallback : Card{};
    if (m_game->play(allowed.first())) m_view->notePlayed(at, thrown);
}

void TablePage::onTrickFinished(int winner) {
    const int points = m_game->trickValue();
    m_view->say(m_game->seat(winner).human()
                    ? tr("Vous ramassez le pli — %1 points.").arg(points)
                    : tr("%1 ramasse le pli — %2 points.")
                          .arg(m_game->seat(winner).name)
                          .arg(points));
    m_sweep->start(sweepMs());
}

void TablePage::sweep() {
    if (m_game->phase() != Phase::TrickOver) return;
    const int winner = m_game->trickWinner();
    m_game->sweepTrick();
    m_view->noteTrickTaken(winner);
}

// ---------------------------------------------------------------------------
// La fin d'une donne.

void TablePage::onDonneFinished() {
    m_beat->stop();
    m_sweep->stop();
    m_view->clearChosen();
    if (m_donneCounted) return;
    m_donneCounted = true;

    const DonneResult &done = m_game->result();
    if (done.played) {
        // Qui a gagné la donne : celui qui a marqué le plus. C'est ce qui
        // compte pour l'expérience, et ce n'est pas la même chose que le
        // contrat -- un camp dedans peut avoir marqué sa belote.
        const int best = done.gained[0] == done.gained[1] ? -1
                                                          : (done.gained[0] > done.gained[1] ? 0 : 1);
        for (int at = 0; at < m_game->seatCount(); ++at) {
            const Seat &seat = m_game->seat(at);
            const bool won = best >= 0 && Belote::teamOf(at) == best;
            if (seat.human()) {
                noteDonnePlayed(won);
                continue;
            }
            noteRivalDonne(seat.rivalId, won, at == done.taker, done.made);
        }

        // Ce qu'ils viennent d'apprendre compte pour la donne suivante, pas
        // pour la prochaine fois qu'on lance le jeu.
        for (int at = 0; at < m_game->seatCount() && at < m_skills.size(); ++at) {
            const Seat &seat = m_game->seat(at);
            if (seat.human()) continue;
            m_skills[at] = skillFor(learningFor(seat.rivalId).xp);
        }
    }

    if (m_game->phase() == Phase::MatchOver) return;   // la feuille suit
    m_board->setGeometry(rect());
    m_board->showDonne(m_game);
    Q_EMIT stateChanged();
}

void TablePage::onMatchFinished() {
    m_beat->stop();
    m_sweep->stop();
    const int winner = m_game->matchWinner();
    const int me = mySeat();
    const bool youWon = winner >= 0 && me >= 0 && Belote::teamOf(me) == winner;

    for (int at = 0; at < m_game->seatCount(); ++at) {
        const Seat &seat = m_game->seat(at);
        if (seat.human()) {
            noteMatchPlayed(youWon, m_game->donneNumber());
            continue;
        }
        noteRivalMatch(seat.rivalId, winner >= 0 && Belote::teamOf(at) == winner);
    }

    m_board->setGeometry(rect());
    m_victory->setGeometry(rect());

    // La feuille est remplie dans tous les cas. Quand il y a un feu
    // d'artifice elle attend derrière, et elle se montre quand il s'éteint.
    m_board->showMatch(m_game);
    if (youWon && m_settings.celebrate) {
        m_board->hide();
        QVector<Card> cards;
        QVector<QPointF> at;
        m_view->cardsOnScreen(&cards, &at);
        m_victory->start(cards, at, m_view->trickCardHeight() * 0.62, tr("Partie gagnée"),
                         tr("%1 contre %2")
                             .arg(m_game->teamScore(winner))
                             .arg(m_game->teamScore(Belote::otherTeam(winner))));
    }
    Q_EMIT stateChanged();
}

// ---------------------------------------------------------------------------
// Votre tour.

void TablePage::sort(bool byStrength) {
    const int me = mySeat();
    if (me < 0) return;
    // Le moteur garde la main dans son ordre ; trier est une affaire d'écran,
    // mais la main est au moteur, alors c'est lui qui la rend triée.
    m_game->sortSeatHand(me, byStrength);
    m_view->refresh();
}

void TablePage::sortAsPreferred() {
    if (!m_settings.holdByStrength) return;
    // Le moteur rend les mains rangées par couleur, l'atout en tête : c'est
    // l'ordre par défaut, et il n'y a que l'autre à poser.
    sort(true);
}

void TablePage::stepBid(int by) {
    if (m_game->variant() != Variant::Coinche) return;
    const int lowest = std::max(kLowestBid, m_game->lowestBid());
    m_bidValue = qBound(lowest, m_bidValue + by, kHighestBid);
    updateActions();
}

// Les quatre boutons de couleur font deux choses selon la façon de jouer :
// prendre la retourne, ou annoncer le contrat affiché. C'est le même geste --
// « je joue à cette couleur-là » -- et c'est pour cela que c'est le même
// bouton.
void TablePage::doSuit(Suit suit) {
    if (m_game->variant() == Variant::Coinche) {
        doBid(m_bidValue, suit);
        return;
    }
    doTake(suit);
}

QVector<int> TablePage::cardsInHands() const {
    QVector<int> ids;
    for (int step = 1; step <= kSeatCount; ++step) {
        const int at = (m_game->dealer() + step) % kSeatCount;
        for (const Card &card : m_game->seat(at).hand) ids.append(card.id);
    }
    return ids;
}

void TablePage::noteTakeDeal(const QVector<int> &before, int retourneId) {
    QVector<int> added;
    for (int id : cardsInHands()) {
        if (!before.contains(id)) added.append(id);
    }
    if (added.isEmpty()) return;
    m_view->noteHanded(added, retourneId);
}

void TablePage::showDealIfNew() {
    // Une donne se distribue une fois ; onTurnBegan passe ici à chaque parole
    // et à chaque carte jouée.
    const int donne = m_game->donneNumber();
    if (donne <= 0 || donne == m_dealShown) return;
    m_dealShown = donne;
    m_view->noteDealt();
}

void TablePage::doTake(Suit suit) {
    // Relevé avant, parce que prendre complète les quatre mains du même coup :
    // après, plus rien ne dit lesquelles des huit cartes viennent d'arriver.
    const QVector<int> before = cardsInHands();
    const int retourneId = m_game->retourne().id;
    QString why;
    if (!m_game->take(suit, &why)) {
        grumble(why);
        return;
    }
    noteTakeDeal(before, retourneId);
}

void TablePage::doBid(int value, Suit suit) {
    QString why;
    if (!m_game->makeBid(value, suit, &why)) grumble(why);
}

void TablePage::doCoinche() {
    QString why;
    if (!m_game->coincher(&why)) grumble(why);
}

void TablePage::doPass() {
    QString why;
    if (!m_game->pass(&why)) grumble(why);
}

void TablePage::doPlay() {
    const int me = mySeat();
    const int chosen = m_view->chosen();
    if (me < 0 || m_game->turn() != me || m_game->phase() != Phase::Playing) {
        grumble(tr("Ce n'est pas à vous de jouer."));
        return;
    }
    if (chosen < 0) {
        grumble(tr("Choisissez d'abord une carte."));
        return;
    }
    const Card *held = findCard(m_game->seat(me).hand, chosen);
    const Card played = held ? *held : Card{};
    QString why;
    if (!m_game->play(chosen, &why)) {
        grumble(why);
        return;
    }
    m_view->clearChosen();
    m_view->notePlayed(me, played);
}

void TablePage::doHint() {
    const int me = mySeat();
    if (me < 0 || m_game->turn() != me) {
        m_view->say(tr("Ce n'est pas à vous de parler."));
        return;
    }
    m_view->say(advise(*m_game, me));
}

void TablePage::onCardPressed(int cardId) {
    const int me = mySeat();
    if (me < 0 || m_game->turn() != me || m_game->phase() != Phase::Playing) {
        // On peut toujours regarder ses cartes, même quand ce n'est pas à
        // nous : une carte qui se soulève sous le doigt est ce qui fait qu'une
        // main est une main et pas une image.
        m_view->setChosen(cardId);
        return;
    }
    // Un clic choisit, un second sur la même carte la joue. C'est la façon la
    // plus indulgente de jouer à la souris : rien ne part sur un clic de
    // travers, et rien ne demande deux gestes différents.
    if (m_view->chosen() == cardId) {
        doPlay();
        return;
    }
    m_view->setChosen(cardId);
    QString why;
    if (!m_game->legal(me, cardId, &why)) {
        grumble(why);
        return;
    }
    sayWhatToDo();
}

void TablePage::onCardDoubleClicked(int cardId) {
    const int me = mySeat();
    if (me < 0 || m_game->turn() != me || m_game->phase() != Phase::Playing) return;
    m_view->setChosen(cardId);
    doPlay();
}

void TablePage::moveChoice(int step) {
    const int me = mySeat();
    if (me < 0) return;
    const QVector<Card> &hand = m_game->seat(me).hand;
    if (hand.isEmpty()) return;
    int at = 0;
    for (int i = 0; i < hand.size(); ++i) {
        if (hand.at(i).id == m_view->chosen()) at = i;
    }
    if (m_view->chosen() < 0) {
        at = step > 0 ? 0 : hand.size() - 1;
    } else {
        at = (at + step + hand.size()) % hand.size();
    }
    m_view->setChosen(hand.at(at).id);
}

// ---------------------------------------------------------------------------
// Ce que les boutons disent.

void TablePage::updateActions() {
    const int me = mySeat();
    const bool mine = me >= 0 && m_game->turn() == me;
    const bool bidding = mine && m_game->phase() == Phase::Bidding;
    const bool laying = mine && m_game->phase() == Phase::Playing;
    const Seat &seat = m_game->seat(std::max(0, me));

    // Un bouton n'est montré que quand le coup qu'il ferait a un sens : les
    // couleurs de l'enchère disparaissent dès que quelqu'un a pris, et le
    // bouton Jouer n'existe pas tant qu'il n'y a pas de carte choisie.
    const bool coinche = m_game->variant() == Variant::Coinche;
    const bool mayAnnounce = bidding && coinche && m_game->mayBid(m_bidValue);

    for (int s = 0; s < 4; ++s) {
        const Suit suit = Suit(s);
        const bool offered = coinche ? mayAnnounce : (bidding && m_game->mayTake(suit));
        m_takeSuit[s]->setVisible(offered);
        m_takeSuit[s]->setEnabled(offered);
        // À la coinche, le bouton porte le contrat qu'il annoncera : c'est le
        // chiffre qui change, et la couleur qui est écrite dessus.
        m_takeSuit[s]->setText(coinche ? tr("%1 %2").arg(m_bidValue).arg(suitPip(suit))
                                       : tr("Prendre %1").arg(suitPip(suit)));
    }

    m_less->setVisible(mayAnnounce);
    m_less->setEnabled(mayAnnounce && m_bidValue > m_game->lowestBid());
    m_more->setVisible(mayAnnounce);
    m_more->setEnabled(mayAnnounce && m_bidValue < kHighestBid);

    const bool mayCapot = bidding && coinche && m_game->mayBid(kCapotBid);
    m_capot->setVisible(mayCapot);
    m_capot->setEnabled(mayCapot);

    const bool mayCoincher = bidding && me >= 0 && m_game->mayCoincher(me);
    m_coinche->setVisible(mayCoincher);
    m_coinche->setEnabled(mayCoincher);
    m_coinche->setText(m_game->awaitingSurcoinche() ? tr("Surcoincher") : tr("Coincher"));

    m_pass->setVisible(bidding);
    m_pass->setEnabled(bidding);
    m_pass->setText(m_game->awaitingSurcoinche() ? tr("Laisser") : tr("Passer"));

    m_play->setVisible(!bidding);
    const int chosen = m_view->chosen();
    m_play->setEnabled(laying && chosen >= 0 && m_game->legal(me, chosen));

    m_sortSuit->setEnabled(me >= 0 && !seat.hand.isEmpty());
    m_sortStrength->setEnabled(me >= 0 && !seat.hand.isEmpty());
    m_hint->setEnabled(mine);
    m_hint->setVisible(m_settings.offerHints);
    m_view->setZoom(m_settings.zoom);

    // Celui qui finit le tour est celui qu'on met en avant.
    m_play->setProperty("strong", m_play->isEnabled());
    for (int s = 0; s < 4; ++s) m_takeSuit[s]->setProperty("strong", false);
    // Au premier tour de la retourne il n'y a qu'une couleur à prendre : c'est
    // celle-là qu'on met en avant, puisque c'est la seule décision qui se
    // présente. À la coinche, c'est coincher qui saute aux yeux.
    if (bidding && !coinche && m_game->biddingRound() == 1) {
        m_takeSuit[int(m_game->retourne().suit)]->setProperty("strong", true);
    }
    m_coinche->setProperty("strong", mayCoincher && m_game->awaitingSurcoinche());
    QVector<QPushButton *> dressed = {m_play, m_coinche};
    for (QPushButton *button : m_takeSuit) dressed.append(button);
    for (QPushButton *button : dressed) {
        button->style()->unpolish(button);
        button->style()->polish(button);
    }

    if (me < 0) {
        setState(QString());
        return;
    }

    if (m_game->phase() == Phase::Bidding) {
        QString where;
        if (m_game->variant() == Variant::Coinche) {
            where = m_game->bidder() < 0
                        ? tr("Enchère — personne n'a parlé")
                        : tr("Enchère — %1 %2 par %3")
                              .arg(m_game->bid() >= kCapotBid ? tr("capot")
                                                              : QString::number(m_game->bid()))
                              .arg(suitWithArticle(m_game->bidSuit()),
                                   m_game->seat(m_game->bidder()).human()
                                       ? tr("vous")
                                       : m_game->seat(m_game->bidder()).name);
            if (m_game->coincheFactor() > 1) {
                where += m_game->coincheFactor() >= 4 ? tr(", surcoinché") : tr(", coinché");
            }
        } else {
            where = tr("Enchère, tour %1 — la retourne est %2")
                        .arg(m_game->biddingRound())
                        .arg(cardName(m_game->retourne()));
        }

        if (!m_settings.offerHints) {
            setState(where);
            return;
        }
        // Ce que la main vaut dans sa meilleure couleur : savoir qu'elle vaut
        // quarante à trèfle et soixante à cœur est à peu près tout ce qu'il y
        // a à apprendre de l'enchère.
        QVector<Card> hand = seat.hand;
        if (m_game->retourne().valid()) hand.append(m_game->retourne());
        int best = -1;
        Suit bestSuit = Suit::Pique;
        for (int s = 0; s < 4; ++s) {
            if (m_game->variant() == Variant::Classique && !m_game->mayTake(Suit(s))) continue;
            const int value = handEstimate(hand, Suit(s));
            if (value > best) {
                best = value;
                bestSuit = Suit(s);
            }
        }
        if (best < 0) {
            setState(where);
            return;
        }
        setState(tr("%1 — votre main vaut %2 points %3")
                             .arg(where)
                             .arg(best)
                             .arg(suitWithArticle(bestSuit)));
        return;
    }

    if (!m_game->trumpChosen()) {
        setState(QString());
        return;
    }
    const int myTeam = Belote::teamOf(me);
    const int takerTeam = Belote::teamOf(m_game->taker());
    const QString whose = takerTeam == myTeam ? tr("vous avez le contrat")
                                              : tr("ils ont le contrat");
    // Le contrat est celui de la donne, et non les quatre-vingt-deux de la
    // belote classique : à la coinche c'est un chiffre qu'on a choisi, et
    // l'écrire de travers rendrait la bande du bas mensongère.
    if (m_game->capotContract()) {
        setState(tr("Atout %1 — %2 : capot, %3 plis sur %4")
                             .arg(suitName(m_game->trump()), whose)
                             .arg(m_game->teamTricks(takerTeam))
                             .arg(kTrickCount));
        return;
    }
    QString contract = QString::number(m_game->contract());
    if (m_game->coincheFactor() >= 4) {
        contract += tr(" surcoinché");
    } else if (m_game->coincheFactor() == 2) {
        contract += tr(" coinché");
    }
    setState(tr("Atout %1 — %2, %3 points sur %4")
                         .arg(suitName(m_game->trump()), whose)
                         .arg(m_game->teamPoints(takerTeam))
                         .arg(contract));
}

void TablePage::setState(const QString &text) {
    m_stateText = text;
    elideState();
}

void TablePage::elideState() {
    if (!m_state) return;
    const QFontMetrics metrics(m_state->font());
    m_state->setText(
        metrics.elidedText(m_stateText, Qt::ElideRight, std::max(40, m_state->width())));
}

void TablePage::sayWhatToDo() {
    const int me = mySeat();
    if (me < 0 || m_game->turn() != me) return;

    if (m_game->phase() == Phase::Bidding) {
        if (m_game->variant() == Variant::Coinche) {
            if (m_game->awaitingSurcoinche()) {
                m_view->say(tr("On a coinché votre contrat : surcoinchez, ou laissez."));
            } else if (m_game->bidder() < 0) {
                m_view->say(tr("À vous : annoncer un contrat d'au moins %1 points, ou passer.")
                                .arg(kLowestBid));
            } else if (m_game->mayCoincher(me)) {
                m_view->say(tr("À vous : annoncer plus de %1, coincher leur contrat, ou "
                               "passer.")
                                .arg(m_game->bid()));
            } else {
                m_view->say(tr("À vous : annoncer plus de %1, ou passer.").arg(m_game->bid()));
            }
            return;
        }
        if (m_game->biddingRound() == 1) {
            m_view->say(tr("À vous : prendre %1, ou passer.")
                            .arg(suitWithArticle(m_game->retourne().suit)));
        } else {
            m_view->say(tr("À vous : prendre dans une autre couleur que %1, ou passer.")
                            .arg(suitName(m_game->retourne().suit)));
        }
        return;
    }
    if (m_game->phase() != Phase::Playing) return;

    const int chosen = m_view->chosen();
    if (chosen >= 0) {
        const Card *card = findCard(m_game->seat(me).hand, chosen);
        if (card) {
            m_view->say(tr("%1 — cliquez encore, ou Jouer.").arg(cardName(*card)));
            return;
        }
    }

    if (!m_game->trickStarted()) {
        m_view->say(tr("À vous d'entamer : vous jouez ce que vous voulez."));
        return;
    }
    const QVector<int> allowed = m_game->legalCards(me);
    const Suit led = m_game->ledSuit();
    const int master = m_game->trickMaster();
    const bool partnerMaster = master >= 0 && Belote::teamOf(master) == Belote::teamOf(me);
    if (allowed.size() == 1) {
        const Card *only = findCard(m_game->seat(me).hand, allowed.first());
        m_view->say(tr("Les règles ne vous laissent que %1.")
                        .arg(only ? cardName(*only) : tr("une carte")));
        return;
    }
    if (partnerMaster) {
        m_view->say(tr("%1 demandé. Le pli est à votre partenaire : chargez-le si vous "
                       "pouvez.")
                        .arg(suitName(led)));
        return;
    }
    m_view->say(tr("%1 demandé, et le pli est à l'adversaire.").arg(suitName(led)));
}

void TablePage::grumble(const QString &why) {
    m_view->sayTrouble(why.isEmpty() ? tr("Ce coup-là n'est pas permis.") : why);
}

// ---------------------------------------------------------------------------

void TablePage::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    elideState();
    if (m_board) m_board->setGeometry(rect());
    if (m_victory) m_victory->setGeometry(rect());
}

void TablePage::keyPressEvent(QKeyEvent *event) {
    const int me = mySeat();
    const bool mine = me >= 0 && m_game->turn() == me;
    switch (event->key()) {
    case Qt::Key_T: sort(false); return;
    case Qt::Key_F: sort(true); return;
    case Qt::Key_Left:
        // Les mêmes deux touches servent à choisir une carte et à faire monter
        // le contrat : ce sont les deux seules choses qu'on parcourt.
        if (mine && m_game->phase() == Phase::Bidding) {
            stepBid(-kBidStep);
        } else {
            moveChoice(-1);
        }
        return;
    case Qt::Key_Right:
        if (mine && m_game->phase() == Phase::Bidding) {
            stepBid(kBidStep);
        } else {
            moveChoice(1);
        }
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        if (mine && m_game->phase() == Phase::Playing) doPlay();
        return;
    case Qt::Key_P:
        if (mine && m_game->phase() == Phase::Bidding) doPass();
        return;
    case Qt::Key_C:
        if (mine && m_game->phase() == Phase::Bidding) doCoinche();
        return;
    case Qt::Key_0:
        if (mine && m_game->phase() == Phase::Bidding &&
            m_game->variant() == Variant::Coinche) {
            doBid(kCapotBid, m_game->bidSuit());
        }
        return;
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
        if (mine && m_game->phase() == Phase::Bidding) {
            doSuit(Suit(event->key() - Qt::Key_1));
        }
        return;
    case Qt::Key_H:
        if (m_settings.offerHints) doHint();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}
