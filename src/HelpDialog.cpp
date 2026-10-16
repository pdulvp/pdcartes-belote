#include "HelpDialog.h"

#include "DialogChrome.h"
#include "Palette.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace {

// La feuille de style du document. Les couleurs viennent de la table que le
// jeu porte, comme partout ailleurs : une boîte de règles ouverte sur une
// table bordeaux est bordeaux.
QString sheetFor(const Palette &scheme) {
    return QStringLiteral(R"CSS(
        h1 { color: %1; font-size: 17px; margin-top: 22px; margin-bottom: 2px; }
        h2 { color: %1; font-size: 13px; margin-top: 16px; margin-bottom: 2px; }
        p { color: %2; margin-top: 6px; margin-bottom: 6px; }
        p.lead { color: %2; font-size: 13px; }
        li { color: %2; margin-bottom: 3px; }
        b { color: %1; }
        th { color: %3; text-align: left; padding-right: 14px;
             padding-top: 3px; padding-bottom: 3px; font-weight: bold; }
        td { color: %2; padding-top: 3px; padding-bottom: 3px; }
        kbd { color: %1; font-weight: bold; }
    )CSS")
        .arg(scheme.accent.name(), scheme.surfaceText.name(), scheme.hint.name());
}

// Les règles, en entier. Écrites ici plutôt que dans un fichier à côté du
// binaire : un fichier à côté du binaire est un fichier qui manque sur la
// machine de quelqu'un d'autre.
const char *kRules = R"HTML(
<h1>La belote</h1>

<p class="lead">La belote se joue à quatre, deux contre deux, avec trente-deux
cartes — du sept à l'as. On joue avec celui d'en face, et les trente-deux
cartes sont distribuées : huit chacun, huit plis, et rien qui reste sur la
table. Un camp annonce un atout et s'engage à faire plus de la moitié des
points ; l'autre essaie de l'en empêcher.</p>

<h2>Ce que valent les cartes</h2>

<p>C'est la première chose à savoir, et c'est la seule chose qui surprenne : une
carte n'a pas une valeur mais deux, selon qu'elle est de l'atout ou non. Le
valet vaut deux points dans une couleur ordinaire et <b>vingt</b> à l'atout ; le
neuf ne vaut rien d'un côté et <b>quatorze</b> de l'autre.</p>

<table>
<tr><th></th><th>à l'atout</th><th>ailleurs</th></tr>
<tr><th>Valet</th><td>20</td><td>2</td></tr>
<tr><th>Neuf</th><td>14</td><td>0</td></tr>
<tr><th>As</th><td>11</td><td>11</td></tr>
<tr><th>Dix</th><td>10</td><td>10</td></tr>
<tr><th>Roi</th><td>4</td><td>4</td></tr>
<tr><th>Dame</th><td>3</td><td>3</td></tr>
<tr><th>Huit, sept</th><td>0</td><td>0</td></tr>
</table>

<p>Cent cinquante-deux points en tout, et dix de plus pour le dernier pli — le
<b>dix de der</b> : <b>cent soixante-deux</b> à se partager.</p>

<h2>L'ordre des cartes</h2>

<p>Il suit les valeurs, et il n'est donc pas le même à l'atout qu'ailleurs.</p>

<table>
<tr><th>À l'atout</th><td><b>V</b> &gt; <b>9</b> &gt; A &gt; 10 &gt; R &gt; D &gt; 8 &gt; 7</td></tr>
<tr><th>Ailleurs</th><td>A &gt; 10 &gt; R &gt; D &gt; V &gt; 9 &gt; 8 &gt; 7</td></tr>
</table>

<p>Le dix est au-dessus du roi, ce qui n'est vrai dans aucun autre jeu et ce
qu'on oublie toutes les trois donnes.</p>

<h1>L'enchère</h1>

<p>On reçoit <b>cinq cartes</b> — trois, puis deux — et une sixième est
<b>retournée</b> au milieu de la table. C'est elle qu'on propose comme atout.
Chacun à son tour, en commençant à la gauche du donneur :</p>

<ul>
<li><b>Prendre</b> : la couleur de la retourne devient l'atout, et c'est vous
qui avez le contrat.</li>
<li><b>Passer</b> : au suivant.</li>
</ul>

<p>Si les quatre passent, <b>second tour</b> : on peut alors prendre dans
n'importe quelle couleur <i>sauf</i> celle de la retourne. Si les quatre passent
encore, la donne est annulée et c'est au suivant de donner.</p>

<p>Celui qui prend ramasse la retourne et deux cartes, les trois autres en
reçoivent trois : huit chacun. La retourne lui revient même quand il a pris
dans une autre couleur — c'est alors une carte qui n'est pas de son atout, et
c'est ce qui fait réfléchir avant de prendre au second tour.</p>

<p>Prendre, c'est parier qu'on fera <b>quatre-vingt-deux points</b> avec son
partenaire, sur cinq cartes vues et trois encore inconnues. Le bouton
<b>Conseil</b> dit ce que votre main vaut dans chaque couleur ; il en faut une
cinquantaine à vous seul pour que la prise soit raisonnable, le reste venant
d'en face.</p>

<h1>Jouer un pli</h1>

<p>Celui qui est à la gauche du donneur entame le premier pli, et ensuite c'est
celui qui a ramassé le pli précédent. On entame ce qu'on veut. Les trois
autres n'ont presque jamais ce choix-là :</p>

<table>
<tr><th>Fournir</th><td>on joue la couleur demandée si on en a. Toujours.</td></tr>
<tr><th>Couper</th><td>si on ne l'a pas et que le pli est à <b>l'adversaire</b>,
on doit jouer atout.</td></tr>
<tr><th>Le partenaire</th><td>si c'est le <b>partenaire</b> qui tient le pli, on
est libre : on se défausse de ce qu'on veut, et on n'a pas à couper le pli de
son propre camp.</td></tr>
<tr><th>Monter</th><td>à l'atout demandé, et sur un atout déjà posé par un
adversaire, il faut passer <b>au-dessus</b> si on peut.</td></tr>
<tr><th>Sous-couper</th><td>si on ne peut pas monter, on n'y est pas obligé : on
joue ce qu'on veut.</td></tr>
</table>

<p>Dans la fenêtre, les cartes que les règles interdisent sont dessinées
éteintes : il n'y a pas moyen de jouer un coup défendu, et pas besoin de
retenir le tableau ci-dessus. Cliquer sur une carte éteinte dit laquelle des
cinq règles s'y oppose.</p>

<p>Le pli va à la plus forte carte de l'atout, ou à défaut à la plus forte
carte de la couleur demandée. Une carte d'une autre couleur ne prend rien : on
s'en est défaussé.</p>

<h2>Belote et rebelote</h2>

<p>Celui qui a le <b>roi et la dame de l'atout</b> dans la même main annonce
<b>belote</b> en jouant la première des deux et <b>rebelote</b> en jouant la
seconde. Cela vaut <b>vingt points</b> à son camp, et ces vingt points lui
restent quoi qu'il arrive ensuite — même si son camp tombe dedans. Ce n'est pas
une annonce : elle ne se compare à rien et ne se perd jamais.</p>

<h1>Les annonces</h1>

<p class="lead">Au premier pli, en jouant sa première carte, chacun annonce ce
qu'il a de suites et de carrés. Ce n'est pas un supplément : un carré de valets
vaut deux cents points, c'est-à-dire plus que la donne entière. Une main sans un
pli peut valoir plus qu'une main qui les fait tous.</p>

<table>
<tr><th>Tierce</th><td>trois cartes qui se suivent dans la même couleur : <b>20</b></td></tr>
<tr><th>Cinquante</th><td>quatre : <b>50</b></td></tr>
<tr><th>Cent</th><td>cinq ou plus : <b>100</b></td></tr>
<tr><th>Carré de valets</th><td><b>200</b></td></tr>
<tr><th>Carré de neufs</th><td><b>150</b></td></tr>
<tr><th>Carré d'as, de dix, de rois, de dames</th><td><b>100</b></td></tr>
<tr><th>Carré de huit, de sept</th><td>rien du tout</td></tr>
</table>

<p>Une suite se lit dans l'<b>ordre naturel</b> — 7, 8, 9, 10, V, D, R, A — et
non dans celui qui prend les plis. Le valet d'atout, qui passe devant tout le
reste, n'est ici qu'entre le dix et la dame. Une carte peut servir deux fois :
le valet d'un carré de valets est aussi dans la tierce où il tombe.</p>

<h2>Un seul camp marque</h2>

<p>C'est la règle qui surprend. Le camp qui tient la <b>plus forte annonce</b>
marque <b>toutes</b> les siennes, celles des deux joueurs réunies ; l'autre camp
n'en marque <b>aucune</b>. Une cinquante contre trois tierces, et les trois
tierces ne valent rien.</p>

<p>On compare la plus forte de chaque camp : un carré passe devant n'importe
quelle suite ; entre deux suites, la plus longue gagne, puis la plus haute, puis
celle qui est à l'atout. Si rien ne les départage, elles s'annulent et personne
ne marque.</p>

<p>Et surtout : les annonces <b>ne comptent pas</b> pour tenir le contrat, qui
se joue sur les seuls cent soixante-deux points des plis. Elles s'ajoutent
après. Un camp preneur qui chute <b>perd les siennes</b> ; les défenseurs
gardent toujours les leurs.</p>

<p>On peut jouer sans : c'est une case dans les réglages.</p>

<h1>Compter la donne</h1>

<p>Chaque camp compte les points des plis qu'il a ramassés, plus dix pour le
dernier pli, plus vingt s'il avait la belote.</p>

<table>
<tr><th>Contrat tenu</th><td>le camp qui a pris fait <b>82 points</b> ou plus :
chacun marque ce qu'il a ramassé.</td></tr>
<tr><th>Dedans</th><td>il en fait 80 ou moins : il ne marque rien, et les
défenseurs marquent les <b>162 points</b> de la donne — les leurs et les
siens.</td></tr>
<tr><th>Litige</th><td>quatre-vingt-un chacun, ce qui ne peut arriver que sans
belote : le camp preneur ne marque rien, et ses <b>81 points sont mis de
côté</b> ; ils iront au camp qui réussira la donne suivante. Les défenseurs
marquent leurs 81.</td></tr>
<tr><th>Capot</th><td>les huit plis : <b>250 points</b> au lieu de 162, pour
celui qui les a faits.</td></tr>
</table>

<p>La feuille de marque écrit les deux chiffres, et ils ne disent pas la même
chose : ce que chaque camp a <b>ramassé</b> dans les plis, et ce qu'il a
<b>marqué</b>. Un camp dedans ramasse soixante-dix points et en marque zéro,
et c'est tout l'intérêt de ne pas prendre n'importe quoi.</p>

<h1>La coinche</h1>

<p class="lead">La coinche — ou belote contrée — est la même belote avec une
autre façon de distribuer la parole. On la choisit dans les réglages, et elle
vaut pour la partie suivante.</p>

<p>Les <b>huit cartes sont données d'un coup</b> et il n'y a pas de retourne :
on enchérit donc sur la main qu'on jouera, ce qui est toute la différence. À la
retourne on parie ; ici on sait.</p>

<h2>L'enchère</h2>

<p>Chacun à son tour annonce un <b>contrat</b> — un nombre de points et une
couleur d'atout — ou passe. Les contrats vont de <b>80 à 160</b> de dix en dix,
et <b>capot</b> par-dessus, qui est l'engagement de faire les huit plis. Chaque
annonce doit dépasser la précédente. Quand trois joueurs passent de suite après
une annonce, l'enchère est close et c'est elle qu'on joue. Si les quatre passent
d'entrée, on redonne.</p>

<h2>Coincher</h2>

<p>Un adversaire du contrat qui ne croit pas qu'il passera <b>coinche</b> : la
valeur du contrat est <b>doublée</b>, dans les deux sens. Le camp qui a annoncé
peut répondre <b>surcoinche</b>, et elle est quadruplée. Après quoi l'enchère est
close et on joue.</p>

<h2>Compter</h2>

<p>Le contrat est tenu si le camp preneur fait <b>au moins le nombre annoncé</b>
en points de plis — les annonces et la belote n'y comptent pas, elles s'ajoutent
après.</p>

<table>
<tr><th>Contrat tenu</th><td>le camp preneur marque <b>ses points de plis plus
la valeur du contrat</b> ; les défenseurs marquent leurs points de plis</td></tr>
<tr><th>Chute</th><td>le camp preneur ne marque rien, sauf sa belote ; les
défenseurs marquent les <b>162 points de la donne plus la valeur du
contrat</b></td></tr>
<tr><th>Coinché</th><td>la valeur du contrat compte double, ou quadruple si elle
a été surcoinchée. Les points de plis, eux, ne changent pas</td></tr>
</table>

<p>Annoncer 120 et les faire rapporte donc 120 + 120 = 240 ; les annoncer, se
faire coincher et chuter en donne 162 + 240 = 402 à l'adversaire. C'est pour ça
qu'on garde une marge en annonçant.</p>

<h2>La partie</h2>

<p>Les cinq tables du menu sont cinq longueurs de partie : d'une seule donne à
mille points. La partie s'arrête à la fin de la donne où un camp dépasse le but
— et il faut qu'il mène, deux camps ex æquo au-dessus du but n'ayant pas fini
de jouer. À trois cent un points une donne mal prise coûte la partie ; à mille,
il y a le temps de se refaire.</p>

<h1>À la souris</h1>

<table>
<tr><th>Clic sur une carte</th><td>la choisir ; elle se soulève</td></tr>
<tr><th>Second clic sur la même</th><td>la jouer</td></tr>
<tr><th>Double-clic</th><td>la jouer tout de suite</td></tr>
<tr><th><b>Prendre ♠ ♥ ♦ ♣</b></th><td>prendre la donne dans cette couleur ; seules
les couleurs permises par le tour d'enchère sont offertes</td></tr>
<tr><th><b>− 90 +</b> puis <b>♠ ♥ ♦ ♣</b></th><td>à la coinche : régler le contrat,
puis l'annoncer dans cette couleur</td></tr>
<tr><th><b>Capot</b></th><td>à la coinche : s'engager aux huit plis</td></tr>
<tr><th><b>Coincher</b></th><td>à la coinche : doubler le contrat de
l'adversaire — ou le surcoincher quand c'est le vôtre qu'on a coinché</td></tr>
<tr><th><b>Passer</b></th><td>laisser parler le suivant</td></tr>
<tr><th><b>Par couleur</b> / <b>Par force</b></th><td>ranger la main, l'atout en
tête dans les deux cas</td></tr>
<tr><th><b>Conseil</b></th><td>ce que le meilleur joueur de la table ferait de
votre main — le même cerveau qu'Oriane, sans ses bourdes — et ce que votre main
vaut dans chaque couleur au moment de l'enchère</td></tr>
<tr><th><kbd>Ctrl</kbd> + molette</th><td>la taille des cartes. Un jeu installé
apporte des images d'une finesse donnée, et agrandies, les figures posées sur le
tapis se lisent. Le même réglage est dans les réglages, en pour cent.</td></tr>
<tr><th>Clic droit sur une carte de joueur, au menu</th><td>l'inviter, le
renommer, ou le remettre à son niveau de départ</td></tr>
</table>

<h2>Au clavier</h2>

<table>
<tr><th><kbd>←</kbd> <kbd>→</kbd></th><td>choisir une carte dans la main</td></tr>
<tr><th><kbd>Entrée</kbd> ou <kbd>Espace</kbd></th><td>jouer la carte choisie</td></tr>
<tr><th><kbd>1</kbd> <kbd>2</kbd> <kbd>3</kbd> <kbd>4</kbd></th><td>prendre — ou
annoncer — à pique, cœur, carreau, trèfle</td></tr>
<tr><th><kbd>←</kbd> <kbd>→</kbd> à l'enchère</th><td>régler le contrat</td></tr>
<tr><th><kbd>0</kbd></th><td>capot</td></tr>
<tr><th><kbd>C</kbd></th><td>coincher</td></tr>
<tr><th><kbd>P</kbd></th><td>passer</td></tr>
<tr><th><kbd>T</kbd> / <kbd>F</kbd></th><td>ranger par couleur / par force</td></tr>
<tr><th><kbd>H</kbd></th><td>un conseil</td></tr>
<tr><th><kbd>F5</kbd></th><td>une autre partie à la même table</td></tr>
<tr><th><kbd>Échap</kbd></th><td>revenir au menu</td></tr>
</table>

<h1>Les six autres</h1>

<p>Les joueurs ne sont pas six réglages d'une même difficulté. Chacun est un
joueur avec un passé : deux s'assoient en ne sachant presque rien, trois
connaissent le jeu, et Oriane y joue depuis des années. Ce qu'ils savent est un
seul chiffre, leur <b>expérience</b>, qui monte un peu à chaque donne jouée et
un peu plus à chaque donne gagnée — et qui est toujours là quand vous revenez.
Une table que vous battiez le mois dernier est une table qui vous bat ce
mois-ci.</p>

<table>
<tr><th>Débutant</th><td>0 à 199 — compte les points qu'il a dans la couleur de
la retourne et prend s'il y en a, sans voir la belote ni la longueur d'atout ;
joue sa carte comme s'il était seul à table, et met son as sur l'as de son
propre partenaire ; et se trompe franchement de temps en temps</td></tr>
<tr><th>Intermédiaire</th><td>200 à 599 — pèse sa main avant de prendre ; sait
que le pli acquis au partenaire se charge d'un dix ou d'un as, et que le pli
qui ne l'est pas encore se laisse passer ; ne se découvre pas pour un pli qui
ne vaut rien</td></tr>
<tr><th>Expert</th><td>600 à 1200 — se souvient des trente-deux cartes, sait
donc si son roi est devenu maître et combien d'atouts sont encore dehors ;
n'encaisse pas une maîtresse dans une couleur où quelqu'un est à sec ; garde un
atout pour le dix de der ; sait où en est le contrat et joue le pli plutôt que
les points quand il est en balance ; coinche quand sa main dit que le contrat
ne passera pas ; et ne fait plus de bourde</td></tr>
</table>

<h2>Qui est à table</h2>

<p>Trois des six s'assoient à chaque partie. Au menu, un clic sur une carte
l'<b>invite</b>, et le <b>premier invité s'assoit en face de vous</b> : c'est
donc lui votre partenaire, ce qui à la belote est le seul choix qui compte
vraiment. Les places qui restent vont aux <b>moins joués</b>, pour que les six
progressent à peu près ensemble au lieu de laisser les trois derniers débutants
pour toujours.</p>

<h2>Les trois profils</h2>

<p>L'expérience des six est comptée dans le profil sous lequel vous jouez, et
nulle part ailleurs. Changer de profil, c'est changer de table : le Lulu que
vous avez battu quarante fois sur le premier profil n'est pas celui du
deuxième. Le profil se tient dans la bande du haut, et sa plaque s'ouvre d'un
clic : on en change de là, on le renomme de là, on l'oublie de là.</p>

<p>Un clic droit sur la carte d'un joueur, au menu d'ouverture, le remet au
niveau avec lequel il est arrivé — lui seul, et sur ce profil seulement, ce qui
sert autant pour un partenaire devenu trop bon pour qu'on ait du mérite que
pour un adversaire devenu imbattable. Le même clic droit le renomme : il garde
sa couleur, son expérience et sa façon de jouer, et répond au nom qu'on lui
donne sur ce profil-là.</p>

<h2>Les cartes et le drap</h2>

<p>Les cartes que le jeu dessine lui-même ne sont pas les seules : les réglages
listent les jeux posés dans le dossier des cartes, avec leurs dos en aperçu, et
l'atelier sait en fabriquer — il ouvre aussi les <b>CARDS.DLL</b> de Windows. La
couleur du drap se choisit là aussi. Les trois — le jeu, le dos, le drap —
appartiennent au profil : changer de profil est changer de table, et une table
n'a pas forcément le même tapis que l'autre.</p>
)HTML";

} // namespace

HelpDialog::HelpDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Les règles de la belote"));

    const DialogChrome chrome = applyDialogChrome(this, tr("Les règles"));
    const Palette &scheme = currentPalette();

    m_pages = new QTextBrowser(this);
    m_pages->setOpenExternalLinks(false);
    m_pages->document()->setDefaultStyleSheet(sheetFor(scheme));
    m_pages->setHtml(QString::fromUtf8(kRules));
    m_pages->moveCursor(QTextCursor::Start);
    chrome.fields->addWidget(m_pages, 1);

    auto *buttons = new QDialogButtonBox(this);
    QPushButton *ok = buttons->addButton(tr("Entendu"), QDialogButtonBox::AcceptRole);
    markDefaultButton(ok);
    ok->setDefault(true);
    chrome.footer->addStretch();
    chrome.footer->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    resize(680, 740);
}
