#include "Brain.h"

#include <QCoreApplication>
#include <QRandomGenerator>
#include <QStringList>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

// ---------------------------------------------------------------------------
// Ce qu'on sait de la donne.
//
// Tout est public dans une donne de belote : les trente-deux cartes sont
// distribuées, et chacune qui tombe sur la table est vue par les quatre. Un
// expert n'est donc pas un joueur qui triche, c'est un joueur qui se souvient
// -- et c'est pour cela que la mémoire est un objet qu'on lui donne et qu'on
// ne donne pas aux autres.
struct Memory {
    // Vrai quand la carte est encore dans une des trois autres mains, pour
    // autant qu'on sache.
    std::array<bool, kPackSize> out{};
};

Memory remember(const Belote &game, int at, bool everything) {
    Memory memory;
    memory.out.fill(true);
    for (const Card &card : game.seat(at).hand) memory.out[card.id] = false;
    if (everything) {
        for (const Played &played : game.history()) memory.out[played.card.id] = false;
        return memory;
    }
    // Celui qui ne compte pas les cartes sait ce qu'il a sous les yeux, et
    // rien de plus : le pli en cours, et sa main. Ce qui est tombé au
    // troisième pli est tombé pour les autres.
    for (const Played &played : game.trick()) memory.out[played.card.id] = false;
    return memory;
}

int outstandingIn(const Memory &memory, Suit suit) {
    int count = 0;
    for (int rank = kSept; rank <= kAs; ++rank) {
        if (memory.out[cardIdFor(rank, suit)]) ++count;
    }
    return count;
}

// Reste-t-il dehors une carte de la même couleur qui passe devant celle-là ?
// C'est la question à laquelle il faut répondre pour savoir si un roi est
// devenu maître, et c'est celle qu'on ne peut pas se poser quand on n'a rien
// retenu.
bool higherOut(const Memory &memory, const Card &card, Suit trump) {
    const int mine = cardStrength(card, trump);
    for (int rank = kSept; rank <= kAs; ++rank) {
        if (!memory.out[cardIdFor(rank, card.suit)]) continue;
        Card other;
        other.id = cardIdFor(rank, card.suit);
        other.rank = rank;
        other.suit = card.suit;
        if (cardStrength(other, trump) > mine) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Lire une main.

int countSuit(const QVector<Card> &hand, Suit suit) {
    int count = 0;
    for (const Card &card : hand) {
        if (card.suit == suit) ++count;
    }
    return count;
}

bool holds(const QVector<Card> &hand, int rank, Suit suit) {
    for (const Card &card : hand) {
        if (card.rank == rank && card.suit == suit) return true;
    }
    return false;
}

// Ce qu'un débutant compte : les points qui sont dans la main, et rien
// d'autre. Ça ne voit ni la longueur d'atout, ni la belote, ni qu'un dix nu
// tombera au premier pli -- mais c'est bien ce qu'on fait quand on vient de
// s'asseoir, et ça prend des mains qu'il fallait laisser.
int crudeEstimate(const QVector<Card> &hand, Suit trump) {
    int total = 0;
    for (const Card &card : hand) total += cardPoints(card, trump);
    return total;
}

const Card *cardOf(const QVector<Card> &hand, int id) { return findCard(hand, id); }

// Ce qu'une carte coûte à lâcher, pour choisir une défausse. Les points
// d'abord, parce que c'est ce qu'on donne ; puis la force, parce qu'une carte
// haute qu'on jette est un pli qu'on ne fera plus ; et l'atout se garde à prix
// égal, puisqu'il y a un dix de der à la fin et qu'il se prend avec un atout.
//
// Et pour celui qui compte les cartes, une chose de plus : se défausser de la
// garde d'une maîtresse, c'est se défausser des deux. Un roi devenu maître et
// gardé par une seule carte tombe au tour suivant si on lâche la garde, alors
// on jette d'une couleur où il n'y a rien à garder. C'est le genre de calcul
// qui demande de savoir que l'as est tombé, et c'est pour cela qu'il est
// réservé à l'expert.
int lettingGoCost(const Card &card, const QVector<Card> &hand, const Memory &memory, Suit trump,
                  const Skill &skill) {
    int cost = cardPoints(card, trump) * 5 + cardStrength(card, trump);
    if (skill.savesDer && card.suit == trump) cost += 7;
    if (!skill.countsCards) return cost;

    int length = 0;
    bool guarding = false;
    for (const Card &held : hand) {
        if (held.suit != card.suit) continue;
        ++length;
        if (held.id != card.id && !higherOut(memory, held, trump)) guarding = true;
    }
    if (guarding && length == 2) cost += 9;
    return cost;
}

int cheapestOf(const QVector<Card> &hand, const QVector<int> &ids, const Memory &memory,
               Suit trump, const Skill &skill) {
    int best = -1;
    int bestCost = 0;
    for (int id : ids) {
        const Card *card = cardOf(hand, id);
        if (!card) continue;
        const int cost = lettingGoCost(*card, hand, memory, trump, skill);
        if (best < 0 || cost < bestCost) {
            best = id;
            bestCost = cost;
        }
    }
    return best;
}

// La carte qui met le plus de points dans le pli : c'est ce que « charger »
// veut dire, et c'est la moitié de ce qu'on fait à la belote.
int richestOf(const QVector<Card> &hand, const QVector<int> &ids, Suit trump) {
    int best = -1;
    int bestValue = -1;
    for (int id : ids) {
        const Card *card = cardOf(hand, id);
        if (!card) continue;
        const int value = cardPoints(*card, trump) * 5 + cardStrength(*card, trump);
        if (value > bestValue) {
            best = id;
            bestValue = value;
        }
    }
    return best;
}

QVector<int> byCost(const QVector<Card> &hand, const QVector<int> &ids, const Memory &memory,
                    Suit trump, const Skill &skill) {
    QVector<int> order = ids;
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        const Card *ca = cardOf(hand, a);
        const Card *cb = cardOf(hand, b);
        if (!ca || !cb) return false;
        return lettingGoCost(*ca, hand, memory, trump, skill) <
               lettingGoCost(*cb, hand, memory, trump, skill);
    });
    return order;
}

// ---------------------------------------------------------------------------
// La carte à jouer, et pourquoi.
//
// Le « pourquoi » n'est pas du décor : c'est lui qui sort par le bouton
// Conseil, et un conseil qui dit quoi jouer sans dire pourquoi n'apprend rien
// à personne.
struct Choice {
    int card = -1;
    QString why;
};

QString line(const char *text) { return QCoreApplication::translate("brain", text); }

Choice chooseLead(const Belote &game, int at, const Skill &skill, const QVector<int> &allowed) {
    const Suit trump = game.trump();
    const QVector<Card> &hand = game.seat(at).hand;
    const Memory memory = remember(game, at, skill.countsCards);
    const bool weTook = Belote::teamOf(at) == Belote::teamOf(game.taker());
    const int trumpsOut = outstandingIn(memory, trump);

    if (!skill.readsPartner) {
        // Le débutant entame par ce qui vaut le plus, atout compris. C'est ce
        // qu'on voit faire à une table de débutants, et c'est ce qui donne le
        // valet d'atout à l'adversaire au premier pli.
        return {richestOf(hand, allowed, trump), line("c'est sa plus grosse carte")};
    }

    // Tirer l'atout. Celui qui a pris a intérêt à faire sortir les atouts des
    // autres pendant qu'il tient le dessus : chaque atout sorti est un pli
    // qu'on ne se fera plus couper, et le preneur a huit cartes à faire
    // passer.
    if (weTook && trumpsOut > 0) {
        int strongest = -1;
        for (int id : allowed) {
            const Card *card = cardOf(hand, id);
            if (!card || card->suit != trump) continue;
            if (strongest < 0 ||
                cardStrength(*card, trump) > cardStrength(*cardOf(hand, strongest), trump)) {
                strongest = id;
            }
        }
        if (strongest >= 0) {
            // Soit on tient le dessus, et l'atout sort sans rien coûter. Soit
            // on en a au moins autant que les trois autres réunis -- ce qui ne
            // se sait qu'en comptant -- et alors l'atout sort quand même : on
            // perd un pli et on prend les suivants, ce qui est le marché que
            // fait tout preneur avec une longue.
            const bool commands = !higherOut(memory, *cardOf(hand, strongest), trump);
            const bool longest = skill.countsCards && countSuit(hand, trump) >= trumpsOut;
            if (commands) {
                return {strongest, line("il a pris : il tire l'atout tant qu'il tient le "
                                        "dessus")};
            }
            if (longest) {
                return {strongest, line("il a pris et il a l'atout le plus long : il le fait "
                                        "sortir")};
            }
        }
    }

    // Encaisser une maîtresse. Rien ne passe devant, alors autant la jouer
    // maintenant -- et de préférence celle qui rapporte.
    int cashable = -1;
    int cashValue = -1;
    for (int id : allowed) {
        const Card *card = cardOf(hand, id);
        if (!card || card->suit == trump) continue;
        if (higherOut(memory, *card, trump)) continue;
        // Mais pas dans une couleur où plus personne n'en a et où il reste de
        // l'atout : une maîtresse qui se fait couper est un cadeau.
        if (skill.countsCards && trumpsOut > 0 && outstandingIn(memory, card->suit) == 0) continue;
        const int value = cardPoints(*card, trump);
        if (value > cashValue) {
            cashable = id;
            cashValue = value;
        }
    }
    if (cashable >= 0) {
        return {cashable, line("plus rien ne passe devant : il l'encaisse")};
    }

    // Rien de maître en main : une petite carte, et on garde ce qui peut
    // encore servir.
    return {cheapestOf(hand, allowed, memory, trump, skill),
            line("il n'a rien de maître : il sort une petite carte")};
}

Choice chooseFollow(const Belote &game, int at, const Skill &skill, const QVector<int> &allowed) {
    const Suit trump = game.trump();
    const Suit led = game.ledSuit();
    const QVector<Card> &hand = game.seat(at).hand;
    const Memory memory = remember(game, at, skill.countsCards);
    const int master = game.trickMaster();
    const bool partnerMaster = master >= 0 && Belote::teamOf(master) == Belote::teamOf(at);
    const int behind = kSeatCount - game.trick().size() - 1;
    const int pot = game.trickValue();

    Card best;
    for (const Played &played : game.trick()) {
        if (played.seat == master) best = played.card;
    }

    // Où en est le contrat.
    //
    // C'est le calcul qui sépare un bon joueur d'un joueur correct, et il ne
    // demande que de savoir compter. Quand ce qui manque au camp preneur tient
    // dans le pli en cours et une grosse carte, ce pli-là ne vaut plus ses
    // points : il vaut les cent soixante-deux de la donne, puisque c'est lui
    // qui décide si le preneur marque tout ou rien. L'expert joue donc le pli
    // et non les points -- et seulement là, parce que le reste du temps ce
    // sont bien les points qui comptent.
    const int takerTeam = Belote::teamOf(game.taker());
    const int missing = kContract - game.teamPoints(takerTeam);
    const bool decisive = skill.countsCards && missing > 0 && missing <= pot + 20;

    QVector<int> winners;
    for (int id : allowed) {
        const Card *card = cardOf(hand, id);
        if (card && beatsTrick(*card, best, led, trump)) winners.append(id);
    }

    if (partnerMaster) {
        if (!skill.readsPartner) {
            // Et voilà la faute qui se voit le plus à une table de débutants :
            // l'as posé sur l'as de son propre camp.
            return {richestOf(hand, allowed, trump),
                    line("il met sa plus grosse carte sans regarder qui tient le pli")};
        }
        const bool sure =
            behind == 0 ||
            (skill.countsCards && !higherOut(memory, best, trump) &&
             (best.suit == trump || outstandingIn(memory, trump) == 0));
        if (sure) {
            return {richestOf(hand, allowed, trump),
                    line("le pli est au partenaire et personne ne peut le lui reprendre : "
                         "on le charge")};
        }
        return {cheapestOf(hand, allowed, memory, trump, skill),
                line("le pli est au partenaire mais il reste quelqu'un derrière : on attend")};
    }

    if (!winners.isEmpty()) {
        const QVector<int> order = byCost(hand, winners, memory, trump, skill);
        if (behind == 0) {
            return {order.first(), line("il est le dernier à parler : il prend au plus juste")};
        }
        if (decisive && order.size() == 1) {
            return {order.first(), line("le contrat se joue sur ce pli, et il n'a qu'une carte "
                                        "pour le prendre")};
        }
        // Il reste du monde derrière : prendre avec une carte qu'on ne
        // repassera pas, sinon c'est deux cartes perdues au lieu d'une.
        for (int id : order) {
            if (!higherOut(memory, *cardOf(hand, id), trump)) {
                return {id, line("il prend avec une carte que personne ne repassera")};
            }
        }
        if (decisive) {
            // Le contrat tient à ce pli-là : on y met ce qu'il faut, et le
            // plus gros plutôt que le plus juste.
            return {order.last(), line("le contrat se joue sur ce pli : il le prend, et avec "
                                       "de quoi tenir")};
        }
        if (pot >= 10) {
            return {order.first(), line("le pli vaut trop pour le laisser filer")};
        }
        return {cheapestOf(hand, allowed, memory, trump, skill),
                line("le pli ne vaut rien : il ne se découvre pas pour si peu")};
    }

    return {cheapestOf(hand, allowed, memory, trump, skill),
            line("il ne peut pas prendre : il se défausse au moins cher")};
}

Choice decide(const Belote &game, int at, const Skill &skill) {
    const QVector<int> allowed = game.legalCards(at);
    if (allowed.isEmpty()) return {};
    if (allowed.size() == 1) {
        return {allowed.first(), line("les règles n'en laissent pas d'autre")};
    }
    return game.trickStarted() ? chooseFollow(game, at, skill, allowed)
                               : chooseLead(game, at, skill, allowed);
}

} // namespace

// ---------------------------------------------------------------------------
// Les niveaux.

Skill skillFor(int xp) {
    Skill skill;
    skill.mastery = masteryOf(xp);
    const qreal within = progressWithin(xp);

    switch (levelOf(xp)) {
    case Level::Debutant:
        // Un coup sur trois en arrivant, un coup sur huit en sortant du
        // niveau : c'est à peu près la seule chose qui s'arrange chez lui, et
        // elle suffit pour qu'on sente la différence entre un Lulu neuf et un
        // Lulu de vingt donnes.
        skill.blunder = 0.30 - 0.17 * within;
        // Et il prend trop : la main lui paraît bonne parce qu'il y a des
        // points dedans.
        skill.boldness = 1.20 - 0.10 * within;
        break;
    case Level::Intermediaire:
        // Il joue bien, et il se trompe encore : une fois sur neuf en
        // arrivant, une fois sur trente en sortant. C'est peu, et c'est à peu
        // près tout ce qui le sépare d'un expert -- parce qu'à la belote les
        // cartes décident de l'essentiel, et qu'un bon joueur ne perd pas
        // souvent contre un meilleur.
        skill.blunder = 0.11 - 0.075 * within;
        skill.readsPartner = true;
        skill.weighsHand = true;
        skill.boldness = 1.05 - 0.05 * within;
        break;
    case Level::Expert:
        skill.blunder = 0.0;
        skill.readsPartner = true;
        skill.weighsHand = true;
        skill.countsCards = true;
        skill.savesDer = true;
        // Un expert ne prend pas moins souvent qu'un autre : il prend mieux.
        // Sa barre est celle qui est juste, et c'est sa lecture de la main qui
        // est meilleure.
        skill.boldness = 1.0;
        break;
    }
    return skill;
}

Skill bestSkill() { return skillFor(kMaxExperience); }

// ---------------------------------------------------------------------------
// Peser une main.

int handEstimate(const QVector<Card> &hand, Suit trump) {
    const int trumps = countSuit(hand, trump);
    const bool valet = holds(hand, kValet, trump);
    const bool belote = holds(hand, kRoi, trump) && holds(hand, kDame, trump);

    int total = 0;
    for (const Card &card : hand) {
        if (card.suit == trump) {
            switch (card.rank) {
            // Le valet d'atout : vingt points, et un pli qui ne se perd
            // jamais. C'est la carte qui décide d'une prise.
            case kValet: total += 26; break;
            // Le neuf vaut quatorze, mais seulement s'il passe -- derrière le
            // valet, ou porté par assez d'atout pour qu'on le sorte au bon
            // moment.
            case kNeuf: total += valet ? 18 : 9; break;
            case kAs: total += trumps >= 3 ? 11 : 7; break;
            case kDix: total += trumps >= 3 ? 8 : 4; break;
            case kRoi: total += 4; break;
            case kDame: total += 3; break;
            default: total += 1; break;
            }
            continue;
        }
        const int length = countSuit(hand, card.suit);
        switch (card.rank) {
        case kAs: total += 10; break;
        // Un dix nu tombe au premier pli ; un dix derrière l'as est dix
        // points à peu près sûrs.
        case kDix: total += holds(hand, kAs, card.suit) ? 9 : (length >= 2 ? 4 : 1); break;
        case kRoi: total += length >= 2 ? 3 : 1; break;
        default: break;
        }
    }

    if (belote) total += kBelotePoints;
    // La longueur d'atout, qui est ce qui fait tenir un contrat : quatre
    // atouts coupent, cinq commandent.
    if (trumps >= 4) total += 8;
    if (trumps >= 5) total += 10;
    // Et une couleur vide avec de l'atout en main, c'est un pli coupé qu'on
    // peut aller chercher.
    if (trumps >= 3) {
        for (int s = 0; s < 4; ++s) {
            const Suit suit = Suit(s);
            if (suit != trump && countSuit(hand, suit) == 0) total += 6;
        }
    }
    return total;
}

namespace {

// Ce que le camp ramasse vraiment, en points de plis, quand la main vaut tant.
//
// Mesuré et non deviné : quelques milliers de donnes jouées, l'estimation de la
// main du preneur notée d'un côté et ce que son camp a ramassé de l'autre. La
// droite est nette -- un peu moins des deux tiers de l'estimation, plus les
// soixante-douze points qu'un camp prend à peu près quoi qu'il arrive. Une
// main médiane vaut quarante-neuf et ramasse cent deux ; une main à quatre-
// vingts en ramasse cent vingt-six.
int expectedTricks(int estimate) { return (estimate * 62) / 100 + kBaselineTricks; }

// Et ce que le preneur ramassera, vu d'un défenseur qui tient `estimate` dans
// la couleur annoncée. L'autre droite : elle descend de trois quarts de point
// par point tenu, ce qui est net -- les atouts qu'on a dans sa main sont
// exactement ceux qu'il n'a pas dans la sienne.
int takerUnderDefence(int estimate) {
    return kTakerUnopposed - (estimate * 73) / 100;
}

// À la coinche tout le monde lit sa main de la même façon : ce qui sépare les
// niveaux ici n'est pas la lecture mais la marge qu'on garde, et le débutant
// n'en garde aucune.
int expectedFor(const QVector<Card> &hand, Suit suit) {
    return expectedTricks(handEstimate(hand, suit));
}

// L'enchère à la coinche : on annonce un chiffre qu'il faudra tenir.
Bid chooseCoincheBid(const Belote &game, int at, const Skill &skill) {
    Bid bid;
    const QVector<Card> &hand = game.seat(at).hand;

    // Coincher, ou surcoincher. La question est la même des deux côtés : est-ce
    // que ma main dit le contraire de ce que l'autre vient d'annoncer ?
    if (game.mayCoincher(at)) {
        const int stake = game.bid();
        if (game.awaitingSurcoinche()) {
            // C'est mon contrat qu'on vient de coincher. On ne surcoinche que
            // quand la main est franchement au-dessus de ce qu'on a dit.
            bid.coinche = skill.weighsHand &&
                          expectedFor(hand, game.bidSuit()) >= stake + 25;
            if (bid.coinche) return bid;
        } else if (skill.weighsHand && stake < kCapotBid) {
            // Coincher, c'est dire « vous n'y arriverez pas », et on ne le dit
            // que sur une main qui le montre : ce que je tiens dans leur atout
            // est ce qu'ils n'auront pas.
            const int theirs = takerUnderDefence(handEstimate(hand, game.bidSuit()));
            if (theirs + kCoincheMargin < stake) {
                bid.coinche = true;
                return bid;
            }
        }
    }
    if (game.awaitingSurcoinche()) return bid;      // on laisse, et on joue

    int best = -1;
    Suit bestSuit = Suit::Pique;
    for (int s = 0; s < 4; ++s) {
        const int value = expectedFor(hand, Suit(s));
        if (value > best) {
            best = value;
            bestSuit = Suit(s);
        }
    }

    // On n'annonce pas ce qu'on fera en moyenne : la moyenne, on la rate une
    // fois sur deux. On garde une marge -- et le débutant n'en garde pas, ce
    // qui est exactement ce qui le fait chuter.
    const int margin = skill.weighsHand ? kBidMargin : 0;
    int holdable = std::min(((best - margin) / kBidStep) * kBidStep, kHighestBid);

    const int wanted = game.lowestBid();
    if (wanted > kHighestBid || holdable < wanted) return bid;
    // Ne pas monter sur son propre partenaire pour dix points : il a dit ce
    // qu'il tenait, et le lui reprendre de justesse ne rapporte rien.
    if (game.bidder() >= 0 && Belote::teamOf(game.bidder()) == Belote::teamOf(at) &&
        holdable < game.bid() + 2 * kBidStep) {
        return bid;
    }

    bid.take = true;
    bid.value = wanted;
    bid.suit = bestSuit;
    return bid;
}

} // namespace

Bid chooseBid(const Belote &game, int at, const Skill &skill) {
    Bid bid;
    if (game.phase() != Phase::Bidding) return bid;
    if (game.variant() == Variant::Coinche) return chooseCoincheBid(game, at, skill);

    // La retourne revient au preneur, quel que soit l'atout annoncé : une
    // estimation sur les cinq cartes en main ignore la sixième qu'on va
    // ramasser, et ce n'est pas la même main.
    QVector<Card> hand = game.seat(at).hand;
    hand.append(game.retourne());

    int bestValue = -1;
    Suit bestSuit = Suit::Pique;
    for (int s = 0; s < 4; ++s) {
        const Suit suit = Suit(s);
        if (!game.mayTake(suit)) continue;
        const int value = skill.weighsHand ? handEstimate(hand, suit) : crudeEstimate(hand, suit);
        if (value > bestValue) {
            bestValue = value;
            bestSuit = suit;
        }
    }
    if (bestValue < 0) return bid;

    int bar = std::max(20, int(std::lround(kTakeEstimate / std::max(0.5, skill.boldness))));
    if (skill.weighsHand) {
        // Le donneur parle en dernier : s'il passe au premier tour, la
        // retourne est refusée par les quatre et l'adversaire choisira sa
        // couleur au second. On prend donc un peu plus bas à cette place.
        if (game.biddingRound() == 1 && at == game.dealer()) bar -= 6;
        // Et si le partenaire a déjà passé, la main doit porter la donne
        // toute seule.
        if (game.seat(Belote::partnerOf(at)).passed) bar += 5;
    }

    bool take = bestValue >= bar;
    // La bourde vaut aussi pour l'enchère : un débutant prend une main qu'il
    // fallait laisser, et laisse celle qu'il fallait prendre.
    if (skill.blunder > 0.0 &&
        QRandomGenerator::global()->generateDouble() < skill.blunder * 0.7) {
        take = !take;
    }

    bid.take = take;
    bid.suit = bestSuit;
    return bid;
}

int chooseCard(const Belote &game, int at, const Skill &skill) {
    const QVector<int> allowed = game.legalCards(at);
    if (allowed.isEmpty()) return -1;
    if (allowed.size() == 1) return allowed.first();

    // La bourde : pas une carte au hasard parmi les trente-deux, mais une
    // carte au hasard parmi celles qui étaient permises -- ce qui est
    // exactement ce qu'on voit faire, et ce qui reste jouable.
    if (skill.blunder > 0.0 && QRandomGenerator::global()->generateDouble() < skill.blunder) {
        return allowed.at(QRandomGenerator::global()->bounded(allowed.size()));
    }

    const Choice choice = decide(game, at, skill);
    return choice.card >= 0 ? choice.card : allowed.first();
}

// ---------------------------------------------------------------------------
// Le conseil.

QString advise(const Belote &game, int at) {
    const Skill skill = bestSkill();

    if (game.phase() == Phase::Bidding) {
        QVector<Card> hand = game.seat(at).hand;
        if (game.retourne().valid()) hand.append(game.retourne());

        QStringList said;
        int bestValue = -1;
        for (int s = 0; s < 4; ++s) {
            const Suit suit = Suit(s);
            if (game.variant() == Variant::Classique && !game.mayTake(suit)) continue;
            const int value = handEstimate(hand, suit);
            said.append(QCoreApplication::translate("brain", "%1 : %2")
                            .arg(suitWithArticle(suit))
                            .arg(value));
            bestValue = std::max(bestValue, value);
        }
        if (bestValue < 0) return line("il n'y a rien à prendre.");

        const Bid bid = chooseBid(game, at, skill);
        const QString counted = said.join(QCoreApplication::translate("brain", ", "));

        if (game.variant() == Variant::Coinche) {
            if (game.awaitingSurcoinche()) {
                return bid.coinche
                           ? line("Votre main vaut nettement plus que ce que vous avez "
                                  "annoncé : surcoinchez.")
                           : line("Ne surcoinchez pas : votre main vaut à peu près ce que "
                                  "vous avez dit, et surcoincher double aussi la chute.");
            }
            if (bid.coinche) {
                return QCoreApplication::translate(
                           "brain", "Il leur faut %1 points et votre seule main en vaut %2 "
                                    "contre eux : coinchez.")
                    .arg(game.bid())
                    .arg(handEstimate(hand, game.bidSuit()));
            }
            if (bid.take) {
                return QCoreApplication::translate(
                           "brain", "Votre main vaut %1 — avec ce que le partenaire apporte, "
                                    "elle tient %2 %3.")
                    .arg(counted)
                    .arg(bid.value)
                    .arg(suitWithArticle(bid.suit));
            }
            return QCoreApplication::translate(
                       "brain", "Votre main vaut %1, et il faudrait tenir %2 : passez.")
                .arg(counted)
                .arg(game.lowestBid());
        }

        if (bid.take) {
            return QCoreApplication::translate(
                       "brain", "Avec la retourne, votre main vaut %1 — il en faut une "
                                "cinquantaine, donc prenez %2.")
                .arg(counted, suitWithArticle(bid.suit));
        }
        return QCoreApplication::translate(
                   "brain", "Avec la retourne, votre main vaut %1 — il en faut une "
                            "cinquantaine : passez.")
            .arg(counted);
    }

    if (game.phase() != Phase::Playing || game.turn() != at) {
        return line("Ce n'est pas à vous de jouer.");
    }

    const Choice choice = decide(game, at, skill);
    const Card *card = findCard(game.seat(at).hand, choice.card);
    if (!card) return line("Jouez ce que vous voulez : tout se vaut.");
    return QCoreApplication::translate("brain", "Jouez %1 : %2.")
        .arg(cardName(*card), choice.why);
}
