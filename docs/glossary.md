# Glossaire technique

Ce glossaire explique les termes employés dans le projet à une personne qui
connaît les règles générales des échecs mais pas nécessairement la
programmation d’un moteur.

Les mots sont classés par ordre alphabétique. Un terme anglais est conservé
lorsqu’il s’agit du nom habituel dans la littérature des moteurs d’échecs.

## A

### Agent

Programme capable de choisir une action dans une situation donnée. Ici,
l’agent reçoit une position d’échecs et choisit un coup légal.

### Alpha

Meilleur score que le joueur au trait est déjà certain de pouvoir obtenir dans
la branche courante. Une branche qui ne peut pas dépasser cette valeur devient
moins intéressante à explorer.

### Alpha-bêta

Méthode qui évite d’examiner des branches incapables de changer la décision
finale. Elle produit le même résultat qu’une recherche exhaustive à profondeur
égale, mais peut visiter beaucoup moins de positions si les bons coups sont
essayés tôt.

### Approfondissement itératif

Recherche successive aux profondeurs 1, 2, 3, etc. La meilleure réponse d’une
petite profondeur aide à organiser la profondeur suivante. Cette répétition
apparente accélère souvent le calcul total.

### Aspiration, fenêtre d’

Petite plage de scores construite autour du résultat de la profondeur
précédente. Si le nouveau score sort de cette plage, la recherche recommence
avec une fenêtre plus large.

## B

### Benchmark

Protocole de mesure répété sur les mêmes positions, la même profondeur et le
même modèle. Il sert à comparer deux versions du moteur. Le temps seul ne
suffit pas : il faut aussi comparer les coups et les nombres de nœuds.

### Bêta

Score au-delà duquel l’adversaire dispose déjà d’une meilleure alternative.
Atteindre bêta permet d’arrêter la branche : c’est une coupure bêta.

### Bitboard

Représentation d’un ensemble de cases avec les 64 bits d’un entier. Les
bitboards permettent des opérations très rapides, mais rendent souvent le code
des règles plus spécialisé. Ce projet n’en utilise pas actuellement.

### Branche

Suite hypothétique de coups dans l’arbre de recherche.

## C

### Capture

Coup qui retire une pièce adverse. Les captures sont prioritaires dans l’ordre
des coups et prolongées par la quiescence lorsqu’elles se trouvent à la limite
de profondeur.

### Centipion

Unité courante des moteurs : cent centipions correspondent approximativement à
un pion. Les scores de ce projet ont une échelle proche, mais les poids
entraînés ne sont pas calibrés comme des probabilités exactes.

### CMake

Outil qui décrit les bibliothèques, exécutables et options de compilation. Les
presets du projet produisent des builds Debug, Release, Benchmark, Sanitize et
Profile.

### Collision de hachage

Situation où deux clés différentes pointent vers le même emplacement d’une
table. La table de transposition vérifie la clé complète avant de réutiliser une
entrée et applique une politique de remplacement.

### Corpus

Ensemble de positions accompagné de valeurs attendues. En mode corpus,
l’entraînement compare directement l’évaluation de l’agent à ces valeurs.

### Coup calme

Coup qui n’est ni une capture ni une promotion. Dans le contexte LMR du projet,
un coup donnant échec est également protégé contre la réduction.

### Coup forcé

Coup imposé ou presque imposé par la position, par exemple une réponse à un
échec. Réduire ou ignorer un coup forcé serait particulièrement dangereux.

### Coupure bêta

Arrêt d’une branche après avoir prouvé qu’elle est déjà trop favorable pour que
l’adversaire la choisisse. Les `killer moves` et l’historique mémorisent les
coups qui provoquent souvent ces coupures.

## D

### Demi-coup

Déplacement d’un seul camp. Un coup complet contient généralement un demi-coup
blanc et un demi-coup noir. Le mot anglais courant est *ply*.

### Déterministe

Calcul qui redonne le même résultat avec les mêmes entrées. Les nombres de
nœuds sont généralement déterministes ; les temps dépendent de la machine et
de sa charge.

### Développement

Sortie des cavaliers et des fous depuis leurs cases initiales. L’évaluation et
le mode instinctif favorisent modestement ce principe d’ouverture.

## E

### Échec

Position dans laquelle le roi du camp au trait est attaqué. La quiescence doit
alors examiner toutes les réponses légales, pas seulement les captures.

### Échec et mat

Échec sans aucun coup légal. Le camp au trait perd immédiatement.

### Élagage

Abandon raisonné d’une partie de l’arbre. Alpha-bêta élague des branches
prouvées inutiles ; les LMR réduisent d’abord certaines branches au lieu de les
supprimer.

### Élitisme

Conservation directe des meilleurs individus d’une génération dans la
suivante. Cela évite de perdre un bon modèle uniquement à cause du hasard de la
reproduction.

### Empreinte Zobrist

Nombre de 64 bits représentant une position. Il est mis à jour avec des XOR
lorsqu’une pièce ou un droit change. Il permet de reconnaître rapidement une
position déjà rencontrée.

### En passant

Règle spéciale permettant à un pion de capturer un pion adverse qui vient
d’avancer de deux cases. La case de prise en passant fait partie de l’état FEN
et du hachage.

### Espace de recherche génétique

Ensemble des paramètres que l’entraînement peut modifier et de leurs bornes.
Deux bornes identiques figent un paramètre.

### Évaluation statique

Estimation d’une position sans continuer à jouer les coups principaux. Elle
combine ici matériel, mobilité, structure de pions, paire de fous,
développement et bouclier du roi.

### Effet d’horizon

Erreur produite lorsque la profondeur s’arrête juste avant une conséquence
importante. La quiescence réduit ce problème pour les suites de captures.

## F

### FEN

*Forsyth-Edwards Notation*. Ligne de texte décrivant le placement, le camp au
trait, les droits de roque, la prise en passant et les compteurs d’une
position.

### Fitness

Score utilisé par l’algorithme génétique pour classer les individus. Il ne
s’agit pas du score d’une position d’échecs, mais de la qualité globale d’un
jeu de paramètres pendant l’expérience.

## G

### Génération de coups

Production des déplacements candidats d’une position. Le projet génère d’abord
des coups pseudo-légaux puis élimine ceux qui exposent le roi.

### Génération génétique

Étape de l’entraînement contenant une population d’individus. Après leur
évaluation, les survivants produisent la génération suivante.

### Génome

Ensemble des paramètres héritables d’un individu : valeurs des pièces et poids
positionnels, ainsi que certains réglages si l’espace les rend variables.

### Graine aléatoire

Nombre initialisant le générateur pseudo-aléatoire. Utiliser la même graine
permet de reproduire les tirages dans un environnement identique.

## H

### Hachage

Transformation d’un état complexe en un nombre compact. Le hachage Zobrist est
adapté aux modifications incrémentales d’un plateau.

### Heuristique

Règle pratique qui améliore généralement le calcul sans constituer une preuve
absolue. Le classement des captures, les `killer moves` et le mode instinctif
sont des heuristiques.

### History heuristic

Mémoire des coups calmes qui ont souvent provoqué une coupure. Leur score
augmente afin qu’ils soient essayés plus tôt dans de futures positions.

## I

### Individu

Un modèle candidat de l’algorithme génétique. Il contient ses paramètres, un
identifiant, sa fitness et son bilan de parties.

### Instinct

Nom du classement positionnel propre au projet. Il favorise le développement,
la centralisation et les destinations approximativement sûres. Il ordonne les
coups mais ne les supprime pas.

## K

### Killer move

Coup calme ayant déjà provoqué une coupure bêta au même niveau de recherche.
Le moteur conserve deux killers par niveau et les essaie plus tôt lorsqu’ils
réapparaissent.

## L

### Légalité

Un coup est légal s’il respecte le déplacement de la pièce et ne laisse pas le
roi de son camp en échec.

### LMR

*Late Move Reductions*, ou réductions de coups tardifs. Les coups calmes classés
loin derrière les premiers sont d’abord cherchés moins profondément. Un coup
prometteur est ensuite recalculé à profondeur normale.

### LTO

*Link-Time Optimization*. Optimisation réalisée lors de l’édition des liens,
quand le compilateur peut examiner plusieurs unités de code ensemble.

## M

### Matériel

Ensemble des pièces présentes et valeur qui leur est associée. Un avantage
matériel ne garantit pas une victoire, mais constitue un signal important.

### Modèle

Fichier JSON contenant un jeu de paramètres d’évaluation et de recherche. Le
modèle actif est `data/models/current.json`.

### Mobilité

Nombre approximatif de cases accessibles aux pièces. Une mobilité plus grande
indique souvent davantage d’activité, sans être toujours synonyme de meilleure
position.

### Move ordering

Ordre dans lequel les coups sont explorés. Un bon ordre ne change pas les coups
légaux, mais rend alpha-bêta beaucoup plus efficace.

### Mutation

Modification aléatoire d’un ou plusieurs paramètres d’un enfant génétique. Son
taux décide si un gène change et son échelle limite l’amplitude du changement.

## N

### Negamax

Forme simplifiée de minimax reposant sur la symétrie :
la valeur pour un joueur est l’opposé de la valeur pour l’adversaire.

### Nœud

Position visitée dans l’arbre de recherche. Les statistiques distinguent les
nœuds principaux des nœuds de quiescence.

### Nœuds par seconde

Nombre de nœuds visités divisé par le temps. Cet indicateur mesure le débit,
mais pas directement la force : visiter moins de nœuds mieux choisis peut être
préférable.

### Null-move pruning

Élagage supposant provisoirement qu’un joueur passe son tour. Il peut accélérer
fortement la recherche, mais échouer dans des positions de zugzwang. Il n’est
pas implémenté dans ce projet.

## O

### Ouverture

Premiers coups d’une partie. Une bibliothèque d’ouvertures permet de jouer des
lignes connues sans lancer immédiatement la recherche.

## P

### Pat

Position sans coup légal dans laquelle le roi n’est pas en échec. La partie est
nulle.

### Perft

Test comptant tous les coups légaux jusqu’à une profondeur donnée. Comparer le
résultat à une référence permet de vérifier le générateur sans dépendre de
l’évaluation.

### Pion doublé

Pion partageant une colonne avec au moins un pion ami. Cette structure reçoit
une pénalité entraînable.

### Pion isolé

Pion sans pion ami sur les colonnes voisines.

### Pion passé

Pion sans pion adverse devant lui sur sa colonne ni sur les deux colonnes
voisines. Sa valeur augmente lorsqu’il approche de la promotion.

### Ply

Terme anglais pour demi-coup. `quiescence_max_ply = 10` autorise au maximum dix
demi-coups supplémentaires dans la recherche tactique.

### Position

État complet d’une partie à un instant donné, et pas seulement placement des
pièces. Les droits de roque, la prise en passant et les compteurs en font aussi
partie.

### Principal Variation Search

Optimisation qui recherche le premier coup avec une fenêtre complète et teste
les suivants avec une fenêtre étroite. Elle est souvent abrégée PVS.

### Profondeur

Nombre de demi-coups principaux explorés depuis la position racine. La
quiescence peut prolonger la recherche au-delà de cette limite nominale.

### Promotion

Transformation d’un pion atteignant la dernière rangée en dame, tour, fou ou
cavalier. Chaque choix est un coup UCI distinct.

### Protocole

Ensemble des commandes et réponses utilisées pour communiquer avec le moteur.
Le protocole de ce projet est textuel, ligne par ligne, et différent du
protocole UCI complet.

### Pseudo-légal

Coup respectant la géométrie d’une pièce mais pas encore vérifié vis-à-vis de
la sécurité du roi.

## Q

### Quiescence

Recherche tactique exécutée à la frontière de profondeur. Elle continue les
captures et promotions, ainsi que toutes les réponses à un échec, jusqu’à une
position plus calme.

## R

### Recherche complète

Recherche d’un coup à la profondeur normale avec la fenêtre requise. Elle
s’oppose ici à un premier essai réduit par LMR.

### Recherche sélective

Recherche qui accorde des profondeurs différentes selon les coups. Les LMR
rendent `instinct_lmr` sélectif sans supprimer définitivement les coups.

### Répétition triple

Règle déclarant la partie nulle lorsqu’une même position apparaît trois fois
avec les mêmes possibilités de jeu.

### Roque

Coup spécial déplaçant le roi et une tour. Sa légalité dépend des pièces, des
cases traversées, des attaques et de droits mémorisés dans la position.

## S

### SAN

*Standard Algebraic Notation*. Notation humaine comme `Nf3`, `O-O` ou
`exd8=Q`. L’outil de génération convertit les lignes SAN en UCI.

### Score terminal

Valeur représentant un mat, un pat ou une nulle, plutôt qu’une simple
estimation positionnelle.

### Sélection

Étape génétique choisissant les individus autorisés à survivre ou à se
reproduire.

### Stand pat

Dans la quiescence hors échec, score obtenu en évaluant la position sans jouer
de nouvelle capture. Il représente le choix implicite de s’arrêter si aucun
échange n’améliore la situation.

## T

### Table de finales

Base donnant le résultat exact de certaines finales avec peu de pièces. Le
projet n’en utilise pas.

### Table de transposition

Cache des positions déjà explorées. Une entrée contient notamment la
profondeur, le score, le type de borne et le meilleur coup.

### Tactique

Suite concrète de coups forcés, captures, échecs ou promotions. Les erreurs
tactiques sont souvent liées à une profondeur insuffisante ou à l’effet
d’horizon.

### Thread

Fil d’exécution. L’entraînement peut évaluer plusieurs paires en parallèle ; la
GUI utilise un thread de travail pour ne pas bloquer Tkinter.

### Trait

Camp qui doit jouer le prochain coup.

### Transposition

Même position atteinte par des ordres de coups différents.

## U

### UCI

*Universal Chess Interface*. Protocole standard utilisé par de nombreux
moteurs. Le projet utilise la notation UCI des coups, comme `e2e4` ou `e7e8q`,
mais son protocole de commandes n’est pas un UCI complet.

### Undo

Annulation d’un coup. `Position::undoMove` restaure les pièces, droits,
compteurs, camp au trait, caches et empreinte.

## Z

### Zobrist

Voir [empreinte Zobrist](#empreinte-zobrist).

### Zugzwang

Position où l’obligation de jouer détériore la situation. Ce motif rend
notamment le `null-move pruning` risqué dans certaines finales.
