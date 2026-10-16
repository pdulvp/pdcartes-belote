# pdcartes-belote

La belote française, écrite en C++17 avec Qt 6, jouée sur un tapis vert à
quatre — vous, un partenaire en face, et deux adversaires qui apprennent d'une
partie à l'autre. À la retourne ou à la coinche, avec ou sans les annonces.

Trente-deux cartes, cinq en main au moment de parler, une retournée au milieu,
et un contrat à quatre-vingt-deux points qu'il faut prendre ou laisser sur une
main dont il manque trois cartes. Six joueurs dont l'expérience est gardée
d'une séance à la suivante : le premier que vous invitez s'assoit en face de
vous, ce qui à la belote est le seul choix qui compte vraiment.

## Jouer

Choisissez une table sur le menu d'ouverture : la partie commence aussitôt. Les
règles sont dans la fenêtre, au bouton **Règles** ou à `F1`.

| | |
|---|---|
| **Prendre ♠ ♥ ♦ ♣** / `1` `2` `3` `4` | prendre la donne dans cette couleur ; seules les couleurs permises par le tour d'enchère sont offertes |
| **− 90 +** / `←` `→` | à la coinche : régler le contrat qu'on s'apprête à annoncer |
| **Capot** / `0` | à la coinche : s'engager aux huit plis |
| **Coincher** / `C` | à la coinche : doubler le contrat de l'adversaire, ou surcoincher le vôtre |
| **Passer** / `P` | laisser parler le suivant |
| clic sur une carte | la choisir ; elle se soulève |
| second clic, **Jouer**, `Entrée` | la jouer |
| double-clic | la jouer tout de suite |
| `←` `→` | choisir une carte au clavier |
| `T` / `F` | ranger la main par couleur / par force |
| `Ctrl` + molette | la taille des cartes, de 80 à 200 % |
| `H` / **Conseil** | ce que le meilleur joueur de la table ferait de votre main |
| `F5` | une autre partie à la même table |
| `Échap` | revenir au menu |

Les cartes que les règles interdisent sont **dessinées éteintes** : il n'y a pas
moyen de jouer un coup défendu, et cliquer sur une carte éteinte dit laquelle
des règles s'y oppose. C'est la façon la plus courte d'apprendre l'obligation
de fournir.

## Les règles, en bref

Une carte n'a pas une valeur mais deux, selon qu'elle est de l'atout ou non.
C'est la seule chose qui surprenne, et c'est toute la belote :

| | à l'atout | ailleurs |
|---|---|---|
| Valet | **20** | 2 |
| Neuf | **14** | 0 |
| As | 11 | 11 |
| Dix | 10 | 10 |
| Roi | 4 | 4 |
| Dame | 3 | 3 |
| Huit, sept | 0 | 0 |

L'ordre suit les valeurs, et n'est donc pas le même des deux côtés : à l'atout
**V > 9 > A > 10 > R > D > 8 > 7**, ailleurs **A > 10 > R > D > V > 9 > 8 > 7**.
Le dix est au-dessus du roi, ce qui n'est vrai dans aucun autre jeu et ce qu'on
oublie toutes les trois donnes.

Cent cinquante-deux points dans les cartes, et dix de plus pour le dernier pli
— le **dix de der** : cent soixante-deux à se partager.

**L'enchère.** Cinq cartes chacun, une retournée. Chacun à son tour prend ou
passe ; si les quatre passent, second tour où l'on peut prendre dans une autre
couleur que la retourne ; si les quatre passent encore, la donne est annulée.
Celui qui prend ramasse la retourne et deux cartes, les autres trois : huit
chacun, et le jeu est épuisé.

**Le jeu.** On fournit à la couleur demandée. Si on ne l'a pas et que le pli est
à l'adversaire, on doit couper ; si c'est le partenaire qui le tient, on est
libre. À l'atout, et sur un atout déjà posé, il faut monter si on peut — et si
on ne peut pas, on n'est pas tenu de sous-couper.

**La belote.** Le roi et la dame d'atout dans la même main : vingt points, dits
en jouant la première des deux puis la seconde, et acquis quoi qu'il arrive
ensuite. Ce n'est pas une annonce : elle ne se compare à rien et ne se perd
jamais.

**La marque.** Le camp qui a pris doit faire **82 points**. S'il les fait,
chacun marque ce qu'il a ramassé. S'il en fait 80 ou moins il est **dedans** :
il ne marque rien et les défenseurs ramassent les 162 de la donne. À
quatre-vingt-un chacun c'est le **litige** : le preneur ne marque rien et ses 81
points sont mis de côté pour le camp qui réussira la donne suivante. Les huit
plis font **capot** : 250 points.

La feuille de marque écrit les deux chiffres, et ils ne disent pas la même
chose : ce que chaque camp a *ramassé* dans les plis, et ce qu'il a *marqué*. Un
camp dedans ramasse soixante-dix points et en marque zéro, ce qui est toute la
raison de ne pas prendre n'importe quoi.

## Les annonces

Au premier pli, en jouant sa première carte, chacun annonce ce qu'il a de suites
et de carrés. Ce n'est pas un supplément : un carré de valets vaut deux cents
points, plus que la donne entière. Une main sans un pli peut valoir plus qu'une
main qui les fait tous.

| | |
|---|---|
| Tierce — trois cartes qui se suivent dans la même couleur | 20 |
| Cinquante — quatre | 50 |
| Cent — cinq ou plus | 100 |
| Carré de valets | 200 |
| Carré de neufs | 150 |
| Carré d'as, de dix, de rois, de dames | 100 |
| Carré de huit, de sept | rien |

Une suite se lit dans l'**ordre naturel** — 7, 8, 9, 10, V, D, R, A — et non
dans celui qui prend les plis : le valet d'atout, qui passe devant tout le
reste, n'est ici qu'entre le dix et la dame. Une carte peut servir deux fois, à
la fois dans un carré et dans la suite où elle tombe.

Et la règle qui surprend : **un seul camp marque**. Celui qui tient la plus
forte annonce marque toutes les siennes, les deux mains réunies ; l'autre n'en
marque aucune. Une cinquante contre trois tierces, et les trois tierces ne
valent rien. Un carré passe devant n'importe quelle suite ; entre deux suites,
la plus longue gagne, puis la plus haute, puis celle qui est à l'atout — et si
rien ne les départage, elles s'annulent.

Les annonces **ne comptent pas** pour tenir le contrat, qui se joue sur les
seuls cent soixante-deux points des plis : elles s'ajoutent après. Un camp
preneur qui chute perd les siennes. Sur la table, chacun porte sur sa plaque ce
qu'il a annoncé, en doré quand ça compte et en gris quand l'autre camp a mieux.

On peut jouer sans : c'est une case dans les réglages.

## La coinche

La coinche — ou belote contrée — est la même belote avec une autre façon de
distribuer la parole. Elle se choisit dans les réglages et vaut pour la partie
suivante.

Les **huit cartes sont données d'un coup** et il n'y a pas de retourne : on
enchérit donc sur la main qu'on jouera. À la retourne on parie ; ici on sait.
Chacun à son tour annonce un **contrat** — un nombre de points de 80 à 160 de
dix en dix, et une couleur d'atout — ou passe, et chaque annonce doit dépasser
la précédente. **Capot** est l'engagement de faire les huit plis. Trois passes
de suite après une annonce closent l'enchère.

Un adversaire qui n'y croit pas **coinche** : la valeur du contrat est doublée,
dans les deux sens. Le camp qui a annoncé peut **surcoincher**, et elle est
quadruplée.

| | |
|---|---|
| Contrat tenu | le camp preneur marque ses points de plis **plus la valeur du contrat** ; les défenseurs marquent leurs points de plis |
| Chute | le camp preneur ne marque rien, sauf sa belote ; les défenseurs marquent les **162 points de la donne plus la valeur du contrat** |
| Coinché | la valeur du contrat compte double, ou quadruple si elle a été surcoinchée ; les points de plis ne changent pas |

Annoncer 120 et les faire rapporte 240 ; les annoncer, se faire coincher et
chuter en donne 402 à l'adversaire. C'est toute la raison de garder une marge en
annonçant — et c'est ce que les adversaires font, ce qui rend les coinches
rares : deux donnes sur cent.

Les écarts sont donc bien plus grands qu'à la retourne, et sur les tables
courtes une seule donne coinchée peut finir la partie. C'est la coinche.

## Les cinq tables

On est quatre, deux contre deux, et ce n'est pas réglable. Ce qui change d'une
table à l'autre n'est donc pas qui y joue mais la longueur de la partie — la
façon de jouer, elle, est dans les réglages et vaut pour toutes. Et la longueur
n'est pas un détail — c'est elle qui décide du jeu
qu'on y joue. À trois cent un points une donne mal prise coûte la partie et il
faut être prudent ; à mille, il y a le temps de tenter une prise sur cinq
cartes et de se refaire, et le temps aussi pour que ce que les six ont appris
finisse par se voir.

| | | |
|---|---|---|
| Une donne | — | une seule, et la meilleure marque l'emporte |
| Le comptoir | 301 | courte : une prise ratée et c'est fini |
| Le tapis vert | 501 | la belote telle qu'on la joue |
| Le cercle | 701 | de quoi se refaire après un capot |
| La soirée | 1000 | assez longue pour que les six y apprennent |

La partie s'arrête à la fin de la donne où un camp dépasse le but — et il faut
qu'il mène, deux camps ex æquo au-dessus du but n'ayant pas fini de jouer.

## Les six joueurs

Ce ne sont pas six positions d'un même curseur de difficulté. Ce qu'ils savent
est un seul chiffre, leur expérience, qui monte un peu à chaque donne jouée et
un peu plus à chaque donne gagnée, qui ne redescend jamais tout seul, et qui est
encore là au lancement suivant. Trois noms sont posés sur ses plages, et chacun
est une liste de choses qu'un joueur fait ou ne fait pas — des choses qu'on peut
voir arriver de l'autre côté de la table.

| | expérience | ce qui change |
|---|---|---|
| Débutant | 0 – 199 | compte les points qu'il a dans la couleur de la retourne et prend s'il y en a, sans voir ni la belote ni la longueur d'atout ; joue sa carte comme s'il était seul à table, et met son as sur l'as de son propre partenaire ; entame par sa plus grosse carte ; et se trompe franchement une fois sur trois ou quatre coups |
| Intermédiaire | 200 – 599 | pèse sa main avant de prendre — le valet, la longueur, la belote, les as dehors, les couleurs vides qui feront des coupes ; charge le partenaire quand le pli lui est acquis et attend quand il ne l'est pas encore ; ne se découvre pas pour un pli qui ne vaut rien |
| Expert | 600 – 1200 | se souvient des trente-deux cartes, sait donc si son roi est devenu maître et combien d'atouts restent dehors ; n'encaisse pas une maîtresse dans une couleur où quelqu'un est à sec ; tire l'atout quand il a pris et qu'il l'a plus long ; ne se défausse pas de la garde d'une maîtresse ; sait où en est le contrat, et quand il est en balance joue le pli et non les points ; coinche quand sa main dit que le contrat ne passera pas ; et ne fait plus de bourde |

L'enchère de la coinche est réglée sur deux droites mesurées en jouant : ce
qu'un camp ramasse selon ce que la main du preneur vaut, et ce que le preneur
ramasse selon ce qu'un défenseur tient dans la couleur annoncée. La seconde a dû
être mesurée deux fois — prise sur un preneur tiré au sort, elle annonçait des
chutes qui n'arrivaient pas, parce qu'un preneur n'est jamais tiré au sort : il
s'est choisi, et c'est pour cela qu'il réussit.

Le coinche, lui, reste un pari, et c'est dit comme tel dans le code : sur cinq
mille donnes mesurées, les adversaires coinchent deux fois sur cent et le
contrat tombe un peu moins d'une fois sur deux. Monter la barre n'y change rien,
et la réserver aux gros contrats non plus — les deux ont été essayés. Ce n'est
pas un défaut du calcul : un défenseur ne voit que sa main, et ce qui manque est
exactement ce que fera celle de son partenaire. À une vraie table non plus, on
ne coinche pas à coup sûr.

Les six commencent à des niveaux différents, pour que la première table ait
quelqu'un à battre et quelqu'un d'imbattable : Lulu et Nestor débutent, Zéphyr,
Prune et Barnabé connaissent le jeu, Oriane y joue depuis des années.

Les écarts ont été mesurés en jouant quelques milliers de parties : un expert
bat un débutant quatre fois sur cinq, un intermédiaire trois fois sur quatre, et
un expert ne bat un intermédiaire que cinquante-cinq fois sur cent — parce qu'à
la belote les cartes décident de l'essentiel, et qu'un bon joueur ne perd pas
souvent contre un meilleur.

**Trois des six s'assoient** à chaque partie. Au menu, un clic sur une carte
l'invite, et **le premier invité s'assoit en face de vous** : c'est donc lui
votre partenaire. Les places qui restent vont aux **moins joués**, pour que les
six progressent ensemble au lieu de laisser les trois derniers débutants pour
toujours.

**Trois profils** séparent tout cela : l'expérience des six est comptée dans le
profil sous lequel on joue et nulle part ailleurs, avec son propre compte de
parties gagnées et perdues. Changer de profil, c'est changer de table. Le profil
se tient dans la bande du haut, et sa plaque s'ouvre d'un clic : on en change de
là, on le renomme de là, on l'oublie de là.

Un **clic droit** sur la carte d'un joueur, au menu d'ouverture, le remet au
niveau avec lequel il est arrivé — lui seul, et sur ce profil seulement, ce qui
sert autant pour un partenaire devenu trop bon pour qu'on ait du mérite que pour
un adversaire devenu imbattable. Le même clic droit le **renomme** : il garde sa
couleur, son expérience et sa façon de jouer, et répond au nom qu'on lui donne
sur ce profil-là.

## Les cartes

La belote dessine ses cartes elle-même par défaut. Pour en employer d'autres,
on dépose un fichier `.cartes` dans `~/.config/cartes` et on le choisit dans
les réglages, avec son dos. Les deux se jugent **sur l'image** et non sur un
nom : sous le jeu choisi, une main d'exemple montre ses huit rangs dans les
quatre couleurs, et les dos s'offrent en vignettes — « Treillis » ne dit pas à
quoi un dos ressemble.

Un jeu installé apporte des images d'une finesse donnée — les `CARDS.DLL` de
Windows font 71 sur 96 — et une figure réduite au quart de cela sur le tapis
n'est plus qu'un bruit de couleurs. La **taille des cartes** se règle donc, de
80 à 200 %, dans les réglages ou à `Ctrl` + molette pendant la partie ; tout le
drap grandit avec elles plutôt que de les laisser déborder.

## La couleur du drap

Six draps au choix — vert, jade, bleu, prune, bordeaux, tabac — et toutes les
couleurs du programme en découlent, la bande du haut et les boîtes de dialogue
comprises. Ce n'est pas la table qui la porte : une partie à mille points n'est
pas d'une autre couleur qu'une partie à trois cent un, elle est seulement plus
longue. C'est donc un goût, et un goût appartient à qui joue — **chaque profil
garde le sien**, comme il garde le jeu de cartes, le dos, et le nom qu'il a
donné à Lulu.

Ces fichiers se fabriquent avec l'atelier, qui est dans
`../cartes-set-creator/` et qui sait aussi ouvrir les `CARDS.DLL` de
Windows. Le dossier est commun au rami : un jeu déposé une fois sert aux deux.
Les cartes qui manquent à un jeu restent dessinées par le programme.

La belote compte son as au-dessus du roi ; le format le range à 1 comme un jeu
de cartes ordinaire, et la traduction se fait en arrivant.

## Construire

```sh
cmake -S . -B build
cmake --build build --parallel
./build/belotesol
```

Un paquet Debian : `cmake --build build --target deb`.

## Le code

Les règles ne savent pas qu'il y a un écran, et le dessin ne sait pas qu'il y a
des règles. Entre les deux, `TablePage` tient la partie et fait jouer les trois
autres.

| | |
|---|---|
| `src/Cards.cpp` | ce qu'est une carte, et ce qu'elle vaut — deux barèmes, selon l'atout |
| `src/Annonces.cpp` | les tierces, les cinquantes, les carrés : les trouver dans une main, et dire laquelle l'emporte |
| `src/Belote.cpp` | la donne : la distribution, les deux enchères, ce qu'on a le droit de jouer, les huit plis et la marque |
| `src/Brain.cpp` | comment un joueur décide, et ce qu'un niveau change exactement |
| `src/Rivals.cpp` | les six, leur expérience et les trois niveaux |
| `src/Profiles.cpp` | les trois profils, et où tout cela est gardé |
| `src/Tables.cpp` | les cinq tables, et la longueur d'une partie |
| `src/TableLook.cpp` | la couleur du drap, le jeu et le dos — ce que le profil en cours veut |
| `src/CardArt.cpp` | le tapis, sa trame, et les trente-deux cartes — dessinées, pas chargées |
| `src/Palette.cpp` | toutes les couleurs, dérivées de la seule teinte du drap |
| `src/TableView.cpp` | la table à l'écran : les plaques, le pli en croix, la marque, votre main, et les cartes qui volent |
| `src/TablePage.cpp` | la page de jeu : les boutons, le rythme des adversaires, le temps que le pli reste sur la table, et l'expérience comptée en fin de donne |
| `src/ScoreBoard.cpp` | la feuille de marque posée sur le tapis |
| `src/VictoryEffect.cpp` | les cartes qui s'envolent et le feu d'artifice |
| `src/TablePicker.cpp` | le menu d'ouverture : les tables, les six, et leur petit menu |
| `src/RivalCard.cpp` | la carte d'un joueur, au menu d'ouverture |
| `src/ProfileChip.cpp` | la plaque du profil, dans la bande du haut |
| `src/NameDialog.cpp` | la petite boîte où l'on donne un nom |
| `src/MenuStyle.cpp` | l'habillage des menus contextuels |
| `src/HelpDialog.cpp` | les règles, en entier, dans la fenêtre |
| `src/MainWindow.cpp` | la fenêtre : la barre du haut et les deux pages |
