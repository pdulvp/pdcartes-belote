#pragma once

#include "Annonces.h"
#include "Cards.h"

#include <QObject>
#include <QString>
#include <QVector>

// La donne : les règles, et rien d'autre.
//
// Rien ici ne sait qu'il y a un écran, ni que trois des quatre joueurs ne sont
// pas des gens. Ce fichier tient les trente-deux cartes, l'enchère, le pli en
// cours et la marque -- et il refuse un coup que la belote ne permet pas, avec
// une phrase qui dit pourquoi, parce qu'une règle qu'on applique en silence
// est une règle que personne n'apprend. C'est d'autant plus vrai ici que la
// belote interdit presque toujours la moitié de la main : fournir à la couleur
// demandée, couper quand on ne l'a pas, monter sur un atout déjà posé.
//
// Deux façons de distribuer la parole, et une seule façon de jouer :
//
//   à la retourne   La belote classique. Cinq cartes chacun, une retournée au
//                   milieu : on la prend ou on passe, et au second tour on
//                   peut prendre dans une autre couleur. Le contrat est
//                   toujours le même -- quatre-vingt-deux des cent
//                   soixante-deux points des plis.
//
//   à la coinche    Huit cartes d'un coup, et c'est le contrat qu'on annonce :
//                   quatre-vingts points, cent, cent trente, capot. Le plus
//                   offrant joue, et l'adversaire qui ne le croit pas coinche,
//                   ce qui double tout.
//
// Le jeu, lui, ne change pas d'une à l'autre : huit plis, on fournit, on
// coupe, on monte. Et les annonces -- tierces, cinquantes, carrés -- se
// déclarent au premier pli dans les deux cas.
//
// Les deux camps sont les places paires et les places impaires : on joue avec
// celui d'en face, ce qui n'a pas besoin d'être réglé puisque c'est la seule
// façon dont la belote se joue.

enum class Phase { Idle, Bidding, Playing, TrickOver, DonneOver, MatchOver };

enum class Variant { Classique, Coinche };

// Ce qui se dit à l'enchère de la coinche : de quatre-vingts à cent soixante
// de dix en dix, et le capot par-dessus.
inline constexpr int kLowestBid = 80;
inline constexpr int kHighestBid = 160;
inline constexpr int kBidStep = 10;
inline constexpr int kCapotBid = kCapot;      // 250, et il faut les huit plis

struct SeatSpec {
    QString rivalId;     // vide pour vous
    QString name;
    int hue = 0;
};

struct Seat {
    QString rivalId;
    QString name;
    int hue = 0;

    QVector<Card> hand;
    // Ce qu'il a dit au tour d'enchère en cours. Remis à plat entre les deux
    // tours de la retourne : passer à la retourne n'empêche pas de prendre à
    // cœur.
    bool passed = false;
    // Ce qu'il a dit la dernière fois qu'il a parlé, écrit court : « passe »,
    // « 90 ♥ », « coinche ». C'est l'enchère telle qu'on l'entend autour de la
    // table, et c'est ce que la table affiche à sa place.
    QString said;
    // Ce qu'il déclare au premier pli, et s'il l'a déjà dit.
    QVector<Annonce> annonces;
    bool declared = false;

    bool human() const { return rivalId.isEmpty(); }
};

// Une carte sur la table, et par qui.
struct Played {
    int seat = -1;
    Card card;
};

// Ce que la donne a donné, une fois comptée. Rempli d'un coup à la fin, pour
// la feuille de marque : c'est la seule chose qu'on regarde entre deux donnes,
// et elle doit tout dire sans qu'on ait à se souvenir de la donne.
struct DonneResult {
    bool played = false;
    bool annulled = false;        // personne n'a pris
    Variant variant = Variant::Classique;
    int taker = -1;
    Suit trump = Suit::Pique;
    int contract = kContract;     // 82 à la retourne, l'enchère à la coinche
    int coinche = 1;              // un, deux ou quatre

    int points[kTeamCount] = {0, 0};     // les plis, dix de der compris
    int annonces[kTeamCount] = {0, 0};   // ce que les annonces ont rapporté
    int gained[kTeamCount] = {0, 0};     // ce qui a été marqué, tout compris
    int tricks[kTeamCount] = {0, 0};

    bool made = false;
    bool capot = false;
    bool litige = false;
    int litigeHeld = 0;           // les points mis de côté par ce litige
    int litigeTaken = 0;          // ceux d'avant, encaissés par cette donne
    int dixDeDer = -1;            // le camp qui a fait le dernier pli
    int beloteTeam = -1;
    int annonceTeam = -1;         // le camp dont les annonces comptent
};

class Belote : public QObject {
    Q_OBJECT

public:
    explicit Belote(QObject *parent = nullptr);

    // `withAnnonces` vaut pour les deux façons de jouer : on peut jouer la
    // coinche sans annonces, et la belote classique avec.
    void startMatch(const QVector<SeatSpec> &seats, int targetPoints, Variant variant,
                    bool withAnnonces);
    void startDonne();

    int seatCount() const { return m_seats.size(); }
    const Seat &seat(int at) const;
    const QVector<Seat> &seats() const { return m_seats; }

    Variant variant() const { return m_variant; }
    bool withAnnonces() const { return m_withAnnonces; }

    // Les camps : les places d'en face jouent ensemble, et il n'y a rien à
    // régler là-dedans.
    static int teamOf(int seat) { return seat & 1; }
    static int partnerOf(int seat) { return (seat + 2) % kSeatCount; }
    static int otherTeam(int team) { return team ^ 1; }

    Phase phase() const { return m_phase; }
    int turn() const { return m_turn; }
    int dealer() const { return m_dealer; }
    int donneNumber() const { return m_donneNumber; }
    int targetPoints() const { return m_target; }

    // --- L'enchère, à la retourne.

    int biddingRound() const { return m_bidRound; }   // un ou deux
    Card retourne() const { return m_retourne; }
    // Les couleurs qu'on peut prendre maintenant : celle de la retourne au
    // premier tour, les trois autres au second.
    bool mayTake(Suit suit) const;
    bool take(Suit suit, QString *why = nullptr);

    // --- L'enchère, à la coinche.

    // Le contrat annoncé jusqu'ici, et par qui. Zéro tant que personne n'a
    // parlé.
    int bid() const { return m_bid; }
    Suit bidSuit() const { return m_bidSuit; }
    int bidder() const { return m_bidder; }
    // Le plus petit contrat qu'on puisse annoncer maintenant.
    int lowestBid() const;
    bool mayBid(int value) const;
    bool makeBid(int value, Suit suit, QString *why = nullptr);
    // Coincher, puis surcoincher : le facteur passe de un à deux, puis à
    // quatre. Le même geste des deux côtés, et c'est pour cela qu'il n'y a
    // qu'une fonction.
    bool mayCoincher(int at) const;
    bool coincher(QString *why = nullptr);
    int coincheFactor() const { return m_coinche; }
    bool awaitingSurcoinche() const { return m_surcoinche; }

    // --- Les deux.

    bool pass(QString *why = nullptr);
    int taker() const { return m_taker; }
    Suit trump() const { return m_trump; }
    bool trumpChosen() const { return m_trumpChosen; }
    // Ce qu'il faut faire : quatre-vingt-deux à la retourne, le contrat annoncé
    // à la coinche, et les huit plis quand c'est un capot.
    int contract() const { return m_contract; }
    bool capotContract() const { return m_contract >= kCapotBid; }

    // --- Le jeu.

    const QVector<Played> &trick() const { return m_trick; }
    const QVector<Played> &lastTrick() const { return m_lastTrick; }
    // Tout ce qui est passé sur la table depuis le début de la donne, dans
    // l'ordre. C'est public : un expert est un joueur qui s'en souvient, pas
    // un joueur qui a le droit de regarder votre main.
    const QVector<Played> &history() const { return m_history; }

    int trickNumber() const { return m_trickNumber; }   // de un à huit
    Suit ledSuit() const;
    bool trickStarted() const { return !m_trick.isEmpty(); }
    // La place qui tient le pli en cours, ou -1 s'il est vide.
    int trickMaster() const;
    int trickWinner() const { return m_trickWinner; }
    // Ce que le pli en cours vaut déjà, pour qu'on sache ce qu'on dispute.
    int trickValue() const;

    // Ce qu'on a le droit de jouer, et pourquoi pas. La liste est la règle :
    // `legal` ne fait que la consulter et mettre une phrase sur un refus.
    QVector<int> legalCards(int at) const;
    bool legal(int at, int cardId, QString *why = nullptr) const;

    bool play(int cardId, QString *why = nullptr);
    // Le pli reste sur la table une fois la quatrième carte posée : il faut le
    // ramasser pour que la donne avance. C'est ce qui laisse le temps de voir
    // qui a pris quoi.
    void sweepTrick();

    // --- La marque.
    //
    // Les points des plis d'un côté, ce qui s'y ajoute de l'autre : le contrat
    // se joue sur les premiers seulement, et la belote comme les annonces
    // viennent après. C'est pour cela que ce sont deux chiffres et non un.

    int teamScore(int team) const;          // le total de la partie
    int teamPoints(int team) const;         // les plis de cette donne
    int teamBonus(int team) const;          // la belote et les annonces acquises
    int teamTricks(int team) const;
    int teamAnnonce(int team) const;        // ce que ses annonces lui vaudront
    int beloteTeam() const { return m_beloteTeam; }
    // Le camp dont les annonces comptent, une fois le premier pli fini. -1
    // tant qu'on ne sait pas, ou quand elles s'annulent.
    int annonceTeam() const { return m_annonceTeam; }
    bool annoncesSettled() const { return m_annoncesSettled; }
    int litige() const { return m_litige; }
    int matchWinner() const { return m_matchWinner; }   // un camp, ou -1
    QString teamName(int team) const;
    const DonneResult &result() const { return m_result; }

    // Ranger une main. L'ordre dans lequel on tient ses cartes appartient à
    // celui qui les tient, alors il vit ici avec la main.
    void sortSeatHand(int at, bool byStrength);

Q_SIGNALS:
    void changed();
    // Une ligne de ce qui vient de se passer, pour la bande au-dessus de la
    // main : le seul récit suivi d'une partie jouée par trois autres
    // personnes.
    void acted(int seat, const QString &tale);
    void turnBegan(int seat);
    void trickFinished(int winner);
    void donneFinished();
    void matchFinished();

private:
    void dealRetourne();
    void dealCoinche();
    void completeDeal();
    void beginPlay();
    void nextBidder();
    void annulDonne();
    void finishDonne();
    void noteBelote(int at, const Card &card);
    void declare(int at);
    void settleAnnonces();
    Seat &here();

    QVector<Seat> m_seats;
    QVector<Card> m_stock;
    Card m_retourne;

    QVector<Played> m_trick;
    QVector<Played> m_lastTrick;
    QVector<Played> m_history;

    Variant m_variant = Variant::Classique;
    bool m_withAnnonces = true;
    Phase m_phase = Phase::Idle;
    int m_turn = 0;
    int m_dealer = 0;
    int m_donneNumber = 0;
    int m_target = 0;
    int m_matchWinner = -1;

    int m_bidRound = 1;
    int m_bidsThisRound = 0;
    int m_passesInARow = 0;
    int m_bid = 0;
    Suit m_bidSuit = Suit::Pique;
    int m_bidder = -1;
    int m_coinche = 1;
    bool m_surcoinche = false;

    int m_taker = -1;
    Suit m_trump = Suit::Pique;
    bool m_trumpChosen = false;
    int m_contract = kContract;

    int m_trickNumber = 0;
    int m_trickWinner = -1;

    int m_score[kTeamCount] = {0, 0};
    int m_points[kTeamCount] = {0, 0};
    int m_annonce[kTeamCount] = {0, 0};
    int m_tricks[kTeamCount] = {0, 0};
    int m_annonceTeam = -1;
    bool m_annoncesSettled = false;
    int m_beloteTeam = -1;
    int m_beloteSeat = -1;
    int m_beloteSaid = 0;
    // Les points mis de côté par un litige, qui reviennent au camp qui
    // réussira la donne suivante.
    int m_litige = 0;

    DonneResult m_result;
};
