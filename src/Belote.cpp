#include "Belote.h"

#include <QStringList>

#include <algorithm>

namespace {

Seat emptySeat() { return Seat{}; }

// La distribution à la retourne : trois cartes, puis deux. Ce n'est pas un
// détail de folklore -- c'est ce qui fait qu'au moment de l'enchère on en a
// cinq et pas huit, et donc qu'on prend sur une main incomplète. Toute la
// belote classique tient là.
constexpr int kFirstDeal = 3;
constexpr int kSecondDeal = 2;

} // namespace

Belote::Belote(QObject *parent) : QObject(parent) {}

const Seat &Belote::seat(int at) const {
    static const Seat nobody = emptySeat();
    if (at < 0 || at >= m_seats.size()) return nobody;
    return m_seats.at(at);
}

Seat &Belote::here() { return m_seats[qBound(0, m_turn, m_seats.size() - 1)]; }

Suit Belote::ledSuit() const {
    return m_trick.isEmpty() ? m_trump : m_trick.first().card.suit;
}

int Belote::trickMaster() const {
    if (m_trick.isEmpty()) return -1;
    const Suit led = m_trick.first().card.suit;
    int best = 0;
    for (int i = 1; i < m_trick.size(); ++i) {
        if (beatsTrick(m_trick.at(i).card, m_trick.at(best).card, led, m_trump, m_trumpChosen)) {
            best = i;
        }
    }
    return m_trick.at(best).seat;
}

int Belote::trickValue() const {
    int total = 0;
    for (const Played &played : m_trick) {
        total += cardPoints(played.card, m_trump, m_trumpChosen);
    }
    return total;
}

int Belote::teamScore(int team) const {
    if (team < 0 || team >= kTeamCount) return 0;
    return m_score[team];
}

int Belote::teamPoints(int team) const {
    if (team < 0 || team >= kTeamCount) return 0;
    return m_points[team];
}

int Belote::teamAnnonce(int team) const {
    if (team < 0 || team >= kTeamCount) return 0;
    return m_annonce[team];
}

int Belote::teamBonus(int team) const {
    if (team < 0 || team >= kTeamCount) return 0;
    return (m_beloteTeam == team ? kBelotePoints : 0) + m_annonce[team];
}

int Belote::teamTricks(int team) const {
    if (team < 0 || team >= kTeamCount) return 0;
    return m_tricks[team];
}

QString Belote::teamName(int team) const {
    QStringList names;
    for (int at = 0; at < m_seats.size(); ++at) {
        if (teamOf(at) != team) continue;
        names.append(m_seats.at(at).human() ? tr("Vous") : m_seats.at(at).name);
    }
    if (names.size() < 2) return names.join(QString());
    return tr("%1 et %2").arg(names.at(0), names.at(1));
}

void Belote::sortSeatHand(int at, bool byStrength) {
    if (at < 0 || at >= m_seats.size()) return;
    if (byStrength) {
        sortByStrength(m_seats[at].hand, m_trump, m_trumpChosen);
    } else {
        sortBySuit(m_seats[at].hand, m_trump, m_trumpChosen);
    }
    Q_EMIT changed();
}

// ---------------------------------------------------------------------------
// La partie, et la donne.

void Belote::startMatch(const QVector<SeatSpec> &specs, int targetPoints, Variant variant,
                        bool withAnnonces) {
    m_seats.clear();
    for (const SeatSpec &spec : specs) {
        Seat seat;
        seat.rivalId = spec.rivalId;
        seat.name = spec.name;
        seat.hue = spec.hue;
        m_seats.append(seat);
    }
    m_variant = variant;
    m_withAnnonces = withAnnonces;
    m_target = std::max(0, targetPoints);
    m_donneNumber = 0;
    m_matchWinner = -1;
    m_litige = 0;
    m_score[0] = 0;
    m_score[1] = 0;
    // Le donneur ne parle pas le premier, alors c'est la dernière place qui
    // donne : la première chose qui arrive dans une partie est une décision du
    // joueur, et non trois adversaires qui réfléchissent.
    m_dealer = m_seats.size() - 1;
    startDonne();
}

void Belote::startDonne() {
    if (m_seats.size() != kSeatCount) return;
    // La donne tourne d'une place à chaque fois, et l'entame avec elle.
    if (m_donneNumber > 0) m_dealer = (m_dealer + 1) % kSeatCount;
    ++m_donneNumber;

    m_trick.clear();
    m_lastTrick.clear();
    m_history.clear();
    m_taker = -1;
    m_trumpChosen = false;
    m_trump = Suit::Pique;
    m_contract = kContract;
    m_trickNumber = 0;
    m_trickWinner = -1;
    m_beloteTeam = -1;
    m_beloteSeat = -1;
    m_beloteSaid = 0;
    m_annonceTeam = -1;
    m_annoncesSettled = false;
    m_points[0] = 0;
    m_points[1] = 0;
    m_annonce[0] = 0;
    m_annonce[1] = 0;
    m_tricks[0] = 0;
    m_tricks[1] = 0;
    m_result = DonneResult{};

    for (Seat &seat : m_seats) {
        seat.hand.clear();
        seat.passed = false;
        seat.said.clear();
        seat.annonces.clear();
        seat.declared = false;
    }

    m_bidRound = 1;
    m_bidsThisRound = 0;
    m_passesInARow = 0;
    m_bid = 0;
    m_bidder = -1;
    m_coinche = 1;
    m_surcoinche = false;

    const QString dealer =
        m_seats.at(m_dealer).human() ? tr("Vous donnez") : tr("%1 donne").arg(m_seats.at(m_dealer).name);

    if (m_variant == Variant::Coinche) {
        dealCoinche();
        Q_EMIT acted(m_dealer, tr("%1 : huit cartes chacun, et on annonce.").arg(dealer));
    } else {
        dealRetourne();
        Q_EMIT acted(m_dealer, tr("%1. %2 est retournée.").arg(dealer, cardName(m_retourne)));
    }

    m_turn = (m_dealer + 1) % kSeatCount;
    m_phase = Phase::Bidding;
    Q_EMIT changed();
    Q_EMIT turnBegan(m_turn);
}

void Belote::dealRetourne() {
    m_stock = freshPack();
    shuffleCards(m_stock);

    // Trois à chacun, puis deux, en tournant depuis la gauche du donneur.
    for (const int packet : {kFirstDeal, kSecondDeal}) {
        for (int i = 0; i < kSeatCount; ++i) {
            const int who = (m_dealer + 1 + i) % kSeatCount;
            for (int card = 0; card < packet; ++card) {
                m_seats[who].hand.append(m_stock.takeLast());
            }
        }
    }
    m_retourne = m_stock.takeLast();
    // Pas d'atout encore : la main est rangée par couleur dans l'ordre
    // ordinaire, et elle sera rangée de nouveau quand on saura à quoi on joue.
    for (Seat &seat : m_seats) sortBySuit(seat.hand, Suit::Pique, false);
}

void Belote::dealCoinche() {
    m_stock = freshPack();
    shuffleCards(m_stock);
    m_retourne = Card{};

    // Les huit d'un coup. On enchérit donc sur la main qu'on jouera, ce qui
    // est toute la différence : à la retourne on parie, ici on sait.
    for (int i = 0; i < kSeatCount; ++i) {
        const int who = (m_dealer + 1 + i) % kSeatCount;
        for (int card = 0; card < kHandSize; ++card) {
            m_seats[who].hand.append(m_stock.takeLast());
        }
    }
    for (Seat &seat : m_seats) sortBySuit(seat.hand, Suit::Pique, false);
}

void Belote::completeDeal() {
    // Le preneur ramasse la retourne et deux cartes, les autres trois : huit
    // chacun, et il ne reste rien sur la table.
    m_seats[m_taker].hand.append(m_retourne);
    for (int i = 0; i < kSeatCount; ++i) {
        const int who = (m_dealer + 1 + i) % kSeatCount;
        const int wanted = who == m_taker ? kSecondDeal : kFirstDeal;
        for (int card = 0; card < wanted && !m_stock.isEmpty(); ++card) {
            m_seats[who].hand.append(m_stock.takeLast());
        }
    }
}

void Belote::beginPlay() {
    // Qui tient le roi et la dame d'atout. Gardé et pas annoncé : la belote se
    // dit en jouant la première des deux, et pas avant.
    m_beloteSeat = -1;
    for (int at = 0; at < kSeatCount; ++at) {
        bool roi = false;
        bool dame = false;
        for (const Card &card : m_seats.at(at).hand) {
            if (card.suit != m_trump) continue;
            if (card.rank == kRoi) roi = true;
            if (card.rank == kDame) dame = true;
        }
        if (roi && dame) m_beloteSeat = at;
    }

    // Et ce que chacun a à déclarer au premier pli. Calculé maintenant, dit
    // plus tard : les annonces se lisent sur la main complète, et la main
    // complète n'existe qu'ici.
    for (Seat &seat : m_seats) {
        seat.annonces = m_withAnnonces ? annoncesIn(seat.hand) : QVector<Annonce>();
        seat.declared = false;
        sortBySuit(seat.hand, m_trump, true);
    }
    m_annoncesSettled = !m_withAnnonces;

    m_phase = Phase::Playing;
    m_trickNumber = 1;
    m_turn = (m_dealer + 1) % kSeatCount;
    Q_EMIT changed();
    Q_EMIT turnBegan(m_turn);
}

// ---------------------------------------------------------------------------
// L'enchère à la retourne.

bool Belote::mayTake(Suit suit) const {
    if (m_phase != Phase::Bidding || m_variant != Variant::Classique) return false;
    // Au premier tour il n'y a qu'une couleur sur la table, et c'est celle de
    // la retourne ; au second, la retourne a été refusée par tout le monde et
    // c'est précisément celle qu'on ne peut plus demander.
    if (m_bidRound == 1) return suit == m_retourne.suit;
    return suit != m_retourne.suit;
}

bool Belote::take(Suit suit, QString *why) {
    if (m_phase != Phase::Bidding) {
        if (why) *why = tr("Ce n'est plus le moment d'enchérir.");
        return false;
    }
    if (m_variant != Variant::Classique) {
        if (why) *why = tr("À la coinche, on annonce un contrat : il n'y a pas de retourne.");
        return false;
    }
    if (!mayTake(suit)) {
        if (why) {
            *why = m_bidRound == 1
                       ? tr("Au premier tour on ne peut prendre qu'à la couleur de la "
                            "retourne, et elle est %1.")
                             .arg(suitWithArticle(m_retourne.suit))
                       : tr("La retourne a été refusée par les quatre : on ne peut plus "
                            "prendre %1.")
                             .arg(suitWithArticle(m_retourne.suit));
        }
        return false;
    }

    m_taker = m_turn;
    m_trump = suit;
    m_trumpChosen = true;
    m_contract = kContract;
    here().said = tr("prend %1").arg(suitPip(suit));
    const QString who = here().human() ? tr("Vous prenez") : tr("%1 prend").arg(here().name);
    Q_EMIT acted(m_turn, tr("%1 %2.").arg(who, suitWithArticle(suit)));

    completeDeal();
    beginPlay();
    return true;
}

// ---------------------------------------------------------------------------
// L'enchère à la coinche.

int Belote::lowestBid() const {
    if (m_bid <= 0) return kLowestBid;
    if (m_bid >= kHighestBid) return kCapotBid;
    return m_bid + kBidStep;
}

bool Belote::mayBid(int value) const {
    if (m_phase != Phase::Bidding || m_variant != Variant::Coinche) return false;
    // Une fois coinché, l'enchère est close : il ne reste qu'à surcoincher ou
    // à laisser.
    if (m_surcoinche || m_coinche > 1) return false;
    if (value == kCapotBid) return m_bid < kCapotBid;
    if (value < kLowestBid || value > kHighestBid) return false;
    if (value % kBidStep != 0) return false;
    return value > m_bid;
}

bool Belote::makeBid(int value, Suit suit, QString *why) {
    if (m_phase != Phase::Bidding || m_variant != Variant::Coinche) {
        if (why) *why = tr("Ce n'est pas le moment d'annoncer un contrat.");
        return false;
    }
    if (m_surcoinche || m_coinche > 1) {
        if (why) *why = tr("Le contrat est coinché : l'enchère est close.");
        return false;
    }
    if (!mayBid(value)) {
        if (why) {
            *why = m_bid > 0
                       ? tr("Il faut annoncer plus que %1 : au moins %2.")
                             .arg(m_bid)
                             .arg(lowestBid())
                       : tr("L'enchère commence à %1 points.").arg(kLowestBid);
        }
        return false;
    }

    m_bid = value;
    m_bidSuit = suit;
    m_bidder = m_turn;
    m_passesInARow = 0;
    here().said = value >= kCapotBid ? tr("capot %1").arg(suitPip(suit))
                                     : tr("%1 %2").arg(value).arg(suitPip(suit));

    const QString who = here().human() ? tr("Vous annoncez") : tr("%1 annonce").arg(here().name);
    Q_EMIT acted(m_turn, value >= kCapotBid
                             ? tr("%1 capot %2.").arg(who, suitWithArticle(suit))
                             : tr("%1 %2 %3.").arg(who).arg(value).arg(suitWithArticle(suit)));

    m_turn = (m_turn + 1) % kSeatCount;
    Q_EMIT changed();
    Q_EMIT turnBegan(m_turn);
    return true;
}

bool Belote::mayCoincher(int at) const {
    if (m_phase != Phase::Bidding || m_variant != Variant::Coinche) return false;
    if (m_bidder < 0) return false;
    // Coincher est pour les adversaires du contrat ; surcoincher, pour le camp
    // qui l'a annoncé.
    if (m_surcoinche) return teamOf(at) == teamOf(m_bidder);
    return m_coinche == 1 && teamOf(at) != teamOf(m_bidder);
}

bool Belote::coincher(QString *why) {
    if (!mayCoincher(m_turn)) {
        if (why) {
            *why = m_bidder < 0 ? tr("Il n'y a pas encore de contrat à coincher.")
                                : tr("On ne coinche pas le contrat de son propre camp.");
        }
        return false;
    }

    const QString who = here().human() ? tr("Vous") : here().name;
    if (!m_surcoinche) {
        m_coinche = 2;
        m_surcoinche = true;
        here().said = tr("coinche !");
        Q_EMIT acted(m_turn, tr("%1 coinche : le contrat est doublé.").arg(who));
        // Au preneur de surcoincher, ou de laisser.
        m_turn = m_bidder;
        Q_EMIT changed();
        Q_EMIT turnBegan(m_turn);
        return true;
    }

    m_coinche = 4;
    m_surcoinche = false;
    here().said = tr("surcoinche !");
    Q_EMIT acted(m_turn, tr("%1 surcoinche : le contrat est quadruplé.").arg(who));
    m_taker = m_bidder;
    m_trump = m_bidSuit;
    m_trumpChosen = true;
    m_contract = m_bid;
    beginPlay();
    return true;
}

// ---------------------------------------------------------------------------
// Passer, des deux côtés.

bool Belote::pass(QString *why) {
    if (m_phase != Phase::Bidding) {
        if (why) *why = tr("Ce n'est plus le moment d'enchérir.");
        return false;
    }

    const QString who = here().human() ? tr("Vous passez.") : tr("%1 passe.").arg(here().name);

    if (m_surcoinche) {
        // Le preneur ne surcoinche pas : le contrat reste coinché, et on joue.
        m_surcoinche = false;
        Q_EMIT acted(m_turn, here().human() ? tr("Vous ne surcoinchez pas.")
                                            : tr("%1 ne surcoinche pas.").arg(here().name));
        m_taker = m_bidder;
        m_trump = m_bidSuit;
        m_trumpChosen = true;
        m_contract = m_bid;
        beginPlay();
        return true;
    }

    here().passed = true;
    here().said = tr("passe");
    Q_EMIT acted(m_turn, who);
    nextBidder();
    return true;
}

void Belote::nextBidder() {
    if (m_variant == Variant::Coinche) {
        ++m_passesInARow;
        // Trois passes après une annonce, et c'est joué ; quatre sans annonce,
        // et personne n'en veut.
        if (m_bidder < 0) {
            if (m_passesInARow >= kSeatCount) {
                annulDonne();
                return;
            }
        } else if (m_passesInARow >= kSeatCount - 1) {
            m_taker = m_bidder;
            m_trump = m_bidSuit;
            m_trumpChosen = true;
            m_contract = m_bid;
            beginPlay();
            return;
        }
        m_turn = (m_turn + 1) % kSeatCount;
        Q_EMIT changed();
        Q_EMIT turnBegan(m_turn);
        return;
    }

    ++m_bidsThisRound;
    if (m_bidsThisRound < kSeatCount) {
        m_turn = (m_turn + 1) % kSeatCount;
        Q_EMIT changed();
        Q_EMIT turnBegan(m_turn);
        return;
    }

    if (m_bidRound == 1) {
        // Personne n'a voulu de la retourne. Second tour : chacun peut prendre
        // dans une autre couleur, et le preneur ramassera quand même la
        // retourne -- ce qui fait qu'au second tour on prend avec une carte
        // qui n'est pas de l'atout qu'on annonce.
        m_bidRound = 2;
        m_bidsThisRound = 0;
        for (Seat &seat : m_seats) {
            seat.passed = false;
            seat.said.clear();
        }
        m_turn = (m_dealer + 1) % kSeatCount;
        Q_EMIT acted(-1, tr("Les quatre ont passé : on peut prendre dans une autre couleur."));
        Q_EMIT changed();
        Q_EMIT turnBegan(m_turn);
        return;
    }

    annulDonne();
}

void Belote::annulDonne() {
    // Deux tours de table et personne : la donne ne se joue pas, et c'est au
    // suivant de donner. Ça arrive, et ce n'est pas un échec du programme --
    // c'est une main que personne n'a voulu prendre.
    m_phase = Phase::DonneOver;
    m_result = DonneResult{};
    m_result.played = false;
    m_result.annulled = true;
    m_result.variant = m_variant;
    Q_EMIT acted(-1, tr("Personne n'a pris : la donne est annulée."));
    Q_EMIT changed();
    Q_EMIT donneFinished();
}

// ---------------------------------------------------------------------------
// Ce qu'on a le droit de jouer.
//
// Trois règles, et elles sont toute la belote :
//
//   fournir     on joue la couleur demandée si on l'a.
//   couper      si on ne l'a pas et que le pli est à l'adversaire, on doit
//               jouer atout -- mais si c'est le partenaire qui tient le pli,
//               on est libre, et c'est là que la belote devient un jeu à deux
//               plutôt qu'un jeu à un.
//   monter      à l'atout, et sur un atout déjà posé, il faut passer dessus
//               si on peut. Si on ne peut pas, on n'est pas obligé de
//               sous-couper : on se défausse de ce qu'on veut.

QVector<int> Belote::legalCards(int at) const {
    QVector<int> allowed;
    const Seat &who = seat(at);
    if (m_phase != Phase::Playing) return allowed;

    // Celui qui entame joue ce qu'il veut : il n'y a pas d'obligation
    // d'entamer atout en belote, ni de garder quoi que ce soit.
    if (m_trick.isEmpty()) {
        for (const Card &card : who.hand) allowed.append(card.id);
        return allowed;
    }

    const Suit led = m_trick.first().card.suit;
    const int master = trickMaster();
    const bool partnerMaster = master >= 0 && teamOf(master) == teamOf(at);

    // L'atout le plus fort déjà sur la table, s'il y en a un.
    Card bestTrump;
    for (const Played &played : m_trick) {
        if (played.card.suit != m_trump) continue;
        if (!bestTrump.valid() ||
            cardStrength(played.card, m_trump, true) > cardStrength(bestTrump, m_trump, true)) {
            bestTrump = played.card;
        }
    }

    const bool hasLed = hasSuit(who.hand, led);

    if (led == m_trump) {
        // À l'atout demandé : fournir l'atout, et monter si on peut.
        if (hasLed) {
            QVector<int> higher;
            for (const Card &card : who.hand) {
                if (card.suit != m_trump) continue;
                if (cardStrength(card, m_trump, true) > cardStrength(bestTrump, m_trump, true)) {
                    higher.append(card.id);
                }
            }
            if (!higher.isEmpty()) return higher;
            // Aucun atout ne passe dessus : on en met un quand même, puisqu'on
            // en a. Lequel est libre.
            for (const Card &card : who.hand) {
                if (card.suit == m_trump) allowed.append(card.id);
            }
            return allowed;
        }
        for (const Card &card : who.hand) allowed.append(card.id);
        return allowed;
    }

    if (hasLed) {
        for (const Card &card : who.hand) {
            if (card.suit == led) allowed.append(card.id);
        }
        return allowed;
    }

    // On n'a pas la couleur demandée.
    const bool hasTrump = hasSuit(who.hand, m_trump);
    if (partnerMaster || !hasTrump) {
        // Libre : soit le partenaire tient le pli et on n'a rien à y faire,
        // soit on n'a pas d'atout et on ne peut rien y faire.
        for (const Card &card : who.hand) allowed.append(card.id);
        return allowed;
    }

    if (bestTrump.valid()) {
        // Un adversaire a déjà coupé : il faut monter dessus si on peut.
        QVector<int> higher;
        for (const Card &card : who.hand) {
            if (card.suit != m_trump) continue;
            if (cardStrength(card, m_trump, true) > cardStrength(bestTrump, m_trump, true)) {
                higher.append(card.id);
            }
        }
        if (!higher.isEmpty()) return higher;
        // On ne peut pas monter : on n'est pas tenu de sous-couper, et tout
        // est permis.
        for (const Card &card : who.hand) allowed.append(card.id);
        return allowed;
    }

    // Personne n'a coupé et le pli est à l'adversaire : il faut couper.
    for (const Card &card : who.hand) {
        if (card.suit == m_trump) allowed.append(card.id);
    }
    return allowed;
}

bool Belote::legal(int at, int cardId, QString *why) const {
    const Seat &who = seat(at);
    const Card *card = findCard(who.hand, cardId);
    if (!card) {
        if (why) *why = tr("Cette carte n'est pas dans votre main.");
        return false;
    }
    if (m_phase != Phase::Playing) {
        if (why) *why = tr("Ce n'est pas le moment de jouer une carte.");
        return false;
    }
    if (legalCards(at).contains(cardId)) return true;
    if (!why) return false;
    // Tout est permis à l'entame, donc un refus veut dire qu'il y a un pli
    // en cours ; la garde est là pour que ce raisonnement soit écrit quelque
    // part plutôt que supposé.
    if (m_trick.isEmpty()) return false;

    // Pourquoi pas. C'est la moitié de l'intérêt de la fonction : une carte
    // qu'on ne peut pas jouer et dont on ne sait pas pourquoi est une règle
    // qu'on n'apprendra pas.
    const Suit led = m_trick.first().card.suit;
    const int master = trickMaster();
    const bool partnerMaster = master >= 0 && teamOf(master) == teamOf(at);

    Card bestTrump;
    for (const Played &played : m_trick) {
        if (played.card.suit != m_trump) continue;
        if (!bestTrump.valid() ||
            cardStrength(played.card, m_trump, true) > cardStrength(bestTrump, m_trump, true)) {
            bestTrump = played.card;
        }
    }

    if (led == m_trump) {
        if (card->suit != m_trump) {
            *why = tr("L'atout est demandé, et vous en avez : il faut fournir.");
        } else {
            *why = tr("À l'atout, il faut monter si on peut : %1 ne passe pas devant %2.")
                       .arg(cardName(*card), cardName(bestTrump));
        }
        return false;
    }
    if (hasSuit(who.hand, led)) {
        *why = tr("%1 est demandé, et vous en avez : il faut fournir à la couleur.")
                   .arg(suitName(led));
        return false;
    }
    if (bestTrump.valid() && !partnerMaster) {
        *why = tr("%1 a coupé : il faut monter à l'atout si on peut, et vous pouvez.")
                   .arg(seat(master).human() ? tr("Quelqu'un") : seat(master).name);
        return false;
    }
    *why = tr("Vous n'avez pas de %1 et le pli est à l'adversaire : il faut couper.")
               .arg(suitName(led));
    return false;
}

// ---------------------------------------------------------------------------
// Les annonces, et la belote.

void Belote::declare(int at) {
    Seat &who = m_seats[at];
    if (who.declared) return;
    who.declared = true;
    if (who.annonces.isEmpty()) return;

    QStringList said;
    for (const Annonce &annonce : who.annonces) said.append(annonce.name());
    const QString all = said.join(tr(" et "));
    Q_EMIT acted(at, who.human() ? tr("Vous annoncez %1.").arg(all)
                                 : tr("%1 annonce %2.").arg(who.name, all));
}

void Belote::settleAnnonces() {
    if (m_annoncesSettled) return;
    m_annoncesSettled = true;
    if (!m_withAnnonces) return;

    // La plus forte annonce de chaque camp, et c'est elle seule qui décide :
    // le camp qui la tient marque tout ce qu'il a annoncé, l'autre rien.
    Annonce best[kTeamCount];
    for (int at = 0; at < kSeatCount; ++at) {
        const Annonce mine = bestAnnonce(m_seats.at(at).annonces, m_trump);
        if (!mine.valid()) continue;
        const int team = teamOf(at);
        if (!best[team].valid() || compareAnnonces(mine, best[team], m_trump) > 0) {
            best[team] = mine;
        }
    }
    if (!best[0].valid() && !best[1].valid()) return;

    const int which = compareAnnonces(best[0], best[1], m_trump);
    if (which == 0) {
        Q_EMIT acted(-1, tr("Les annonces se valent : personne ne les marque."));
        return;
    }

    m_annonceTeam = which > 0 ? 0 : 1;
    for (int at = 0; at < kSeatCount; ++at) {
        if (teamOf(at) != m_annonceTeam) continue;
        m_annonce[m_annonceTeam] += annonceTotal(m_seats.at(at).annonces);
    }
    Q_EMIT acted(-1, tr("%1 : %2 points d'annonces, et l'autre camp rien.")
                         .arg(teamName(m_annonceTeam))
                         .arg(m_annonce[m_annonceTeam]));
}

void Belote::noteBelote(int at, const Card &card) {
    if (at != m_beloteSeat || card.suit != m_trump) return;
    if (card.rank != kRoi && card.rank != kDame) return;

    ++m_beloteSaid;
    if (m_beloteSaid == 1) {
        // Les vingt points sont acquis en disant « belote » : celui qui tient
        // les deux jouera la seconde, il n'y a pas de doute à avoir là-dessus.
        m_beloteTeam = teamOf(at);
        Q_EMIT acted(at, seat(at).human() ? tr("Belote ! Vingt points.")
                                          : tr("%1 annonce belote. Vingt points.")
                                                .arg(seat(at).name));
        return;
    }
    if (m_beloteSaid == 2) {
        Q_EMIT acted(at, seat(at).human() ? tr("Et rebelote.")
                                          : tr("%1 dit rebelote.").arg(seat(at).name));
    }
}

// ---------------------------------------------------------------------------
// Le pli.

bool Belote::play(int cardId, QString *why) {
    if (m_phase != Phase::Playing) {
        if (why) *why = tr("Ce n'est pas le moment de jouer une carte.");
        return false;
    }
    if (!legal(m_turn, cardId, why)) return false;

    const int at = m_turn;
    // Les annonces se disent en jouant sa première carte, et pas avant : c'est
    // le moment où chacun découvre ce que les autres ont.
    if (m_trickNumber == 1 && m_withAnnonces) declare(at);

    const Card card = takeCard(here().hand, cardId);
    m_trick.append({at, card});
    m_history.append({at, card});
    noteBelote(at, card);

    Q_EMIT acted(at, seat(at).human() ? tr("Vous jouez %1.").arg(cardName(card))
                                      : tr("%1 joue %2.").arg(seat(at).name, cardName(card)));

    if (m_trick.size() < kSeatCount) {
        m_turn = (m_turn + 1) % kSeatCount;
        Q_EMIT changed();
        Q_EMIT turnBegan(m_turn);
        return true;
    }

    // Les quatre ont parlé : on sait maintenant quelles annonces comptent.
    if (m_trickNumber == 1) settleAnnonces();

    m_trickWinner = trickMaster();
    m_phase = Phase::TrickOver;
    Q_EMIT changed();
    Q_EMIT trickFinished(m_trickWinner);
    return true;
}

void Belote::sweepTrick() {
    if (m_phase != Phase::TrickOver) return;

    const int winner = m_trickWinner;
    const int team = teamOf(winner);
    for (const Played &played : m_trick) {
        m_points[team] += cardPoints(played.card, m_trump, true);
    }
    ++m_tricks[team];

    const bool last = m_trickNumber >= kTrickCount;
    if (last) {
        // Le dix de der : dix points pour le dernier pli, qui est la raison
        // pour laquelle on garde un atout quand on n'en a plus besoin.
        m_points[team] += kDixDeDer;
        m_result.dixDeDer = team;
    }

    m_lastTrick = m_trick;
    m_trick.clear();

    if (last) {
        finishDonne();
        return;
    }

    ++m_trickNumber;
    m_turn = winner;
    m_phase = Phase::Playing;
    Q_EMIT changed();
    Q_EMIT turnBegan(m_turn);
}

// ---------------------------------------------------------------------------
// La marque.
//
// Le contrat se joue sur les cent soixante-deux points des plis, et sur eux
// seuls. La belote et les annonces s'ajoutent après -- ce qui veut dire qu'un
// carré de valets ne fait pas tenir un contrat, et qu'un camp peut marquer
// deux cent vingt points en étant dedans de trois.

void Belote::finishDonne() {
    m_phase = Phase::DonneOver;

    const int takers = teamOf(m_taker);
    const int defenders = otherTeam(takers);
    const int mine = m_points[takers];
    const int theirs = m_points[defenders];
    const bool takersCapot = m_tricks[takers] == kTrickCount;
    const bool defendersCapot = m_tricks[defenders] == kTrickCount;

    bool made = false;
    bool litige = false;
    if (m_variant == Variant::Coinche) {
        made = capotContract() ? takersCapot : mine >= m_contract;
    } else {
        // Le litige : quatre-vingt-un chacun, et personne n'a gagné la donne.
        litige = !takersCapot && !defendersCapot && mine == kLitige && theirs == kLitige;
        made = !litige && mine >= kContract;
    }

    // La belote reste à qui la tenait, dedans ou pas : vingt points annoncés
    // sont vingt points marqués, et c'est la règle la plus répandue. Les
    // annonces, elles, se perdent quand le camp qui a pris chute.
    auto bonus = [&](int team, bool keepsAnnonces) {
        int total = m_beloteTeam == team ? kBelotePoints : 0;
        if (keepsAnnonces) total += m_annonce[team];
        return total;
    };

    DonneResult done;
    done.played = true;
    done.variant = m_variant;
    done.taker = m_taker;
    done.trump = m_trump;
    done.contract = m_contract;
    done.coinche = m_coinche;
    done.beloteTeam = m_beloteTeam;
    done.annonceTeam = m_annonceTeam;
    done.dixDeDer = m_result.dixDeDer;
    for (int team = 0; team < kTeamCount; ++team) {
        done.points[team] = m_points[team];
        done.tricks[team] = m_tricks[team];
    }
    done.capot = takersCapot || defendersCapot;
    done.made = made;
    done.litige = litige;

    if (m_variant == Variant::Coinche) {
        const int stake = m_contract * m_coinche;
        if (made) {
            done.gained[takers] = mine + stake + bonus(takers, true) + m_litige;
            done.gained[defenders] = theirs + bonus(defenders, true);
        } else {
            done.gained[defenders] =
                kDonnePoints + stake + bonus(defenders, true) + m_litige;
            done.gained[takers] = bonus(takers, false);
        }
        done.litigeTaken = m_litige;
        m_litige = 0;
    } else if (litige) {
        // Le camp preneur ne marque rien et ses quatre-vingt-un points sont
        // mis de côté : ils reviendront au camp qui réussira la donne
        // suivante. Les défenseurs marquent les leurs.
        done.gained[takers] = bonus(takers, false);
        done.gained[defenders] = theirs + bonus(defenders, true);
        m_litige += kLitige;
        done.litigeHeld = kLitige;
    } else if (takersCapot) {
        done.gained[takers] = kCapot + bonus(takers, true) + m_litige;
        done.gained[defenders] = bonus(defenders, true);
        done.litigeTaken = m_litige;
        m_litige = 0;
    } else if (defendersCapot) {
        done.gained[defenders] = kCapot + bonus(defenders, true) + m_litige;
        done.gained[takers] = bonus(takers, false);
        done.litigeTaken = m_litige;
        m_litige = 0;
    } else if (made) {
        done.gained[takers] = mine + bonus(takers, true) + m_litige;
        done.gained[defenders] = theirs + bonus(defenders, true);
        done.litigeTaken = m_litige;
        m_litige = 0;
    } else {
        // Dedans : les défenseurs ramassent les cent soixante-deux points de
        // la donne, et le camp preneur ne garde que sa belote.
        done.gained[defenders] = kDonnePoints + bonus(defenders, true) + m_litige;
        done.gained[takers] = bonus(takers, false);
        done.litigeTaken = m_litige;
        m_litige = 0;
    }

    for (int team = 0; team < kTeamCount; ++team) {
        done.annonces[team] = m_annonce[team];
        m_score[team] += done.gained[team];
    }
    m_result = done;

    // Une seule donne : la partie est cette donne. Sinon, il faut que
    // quelqu'un dépasse le but -- et qu'il mène, parce que deux camps à cinq
    // cent cinq points ex æquo n'ont pas fini de jouer.
    bool over = m_target <= 0;
    if (!over && (m_score[0] >= m_target || m_score[1] >= m_target)) {
        over = m_score[0] != m_score[1];
    }

    if (over) {
        m_matchWinner = m_score[0] == m_score[1] ? -1 : (m_score[0] > m_score[1] ? 0 : 1);
    }

    Q_EMIT changed();
    Q_EMIT donneFinished();
    if (!over) return;

    m_phase = Phase::MatchOver;
    Q_EMIT matchFinished();
}
