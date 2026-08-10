# Moteur d’échecs C++ et optimisation génétique

## Résumé du projet

Ce projet réunit un moteur d’échecs écrit en C++20, une interface graphique
Tkinter et un pipeline d’optimisation génétique des paramètres d’évaluation.
Son objectif n’est pas de reproduire un moteur de compétition moderne, mais de
proposer un agent de jeu autonome, compréhensible, mesurable et suffisamment
rapide pour atteindre confortablement une profondeur nominale de 8 sur le
corpus de référence du projet.

L’agent sait :

- appliquer les règles usuelles des échecs, y compris le roque, la promotion,
  la prise en passant, le pat, la règle des cinquante coups, le matériel
  insuffisant et la triple répétition ;
- charger et sérialiser une position au format FEN ;
- utiliser une bibliothèque d’ouvertures générée depuis
  `lichess-org/chess-openings` ;
- explorer l’arbre de jeu avec negamax et élagage alpha-bêta ;
- stabiliser ses feuilles de recherche avec une recherche de quiescence ;
- réutiliser les positions déjà analysées grâce à une table de transposition ;
- classer les coups avec plusieurs heuristiques, dont un mode dit
  « instinctif » ;
- réduire prudemment certains coups tardifs avec les LMR ;
- apprendre les poids de son évaluation par sélection génétique ;
- exposer un protocole textuel utilisable par la GUI ou par un autre client.

Le modèle actif livré avec le projet est conservé dans
[`data/models/current.json`](data/models/current.json). Il s’agit d’un résultat
d’entraînement, et non d’un fichier temporaire. Sa provenance est documentée
dans [`data/models/README.md`](data/models/README.md).

Les termes techniques employés dans ce document sont expliqués dans le
[`glossaire`](docs/glossary.md).

## Sommaire

1. [Installation](#installation)
   1. [Prérequis](#prérequis)
   2. [Environnement Conda](#environnement-conda)
   3. [Compilation](#compilation)
   4. [Tests et vérifications](#tests-et-vérifications)
   5. [Exécutables produits](#exécutables-produits)
2. [Description de l’agent de jeu](#description-de-lagent-de-jeu)
   1. [Fonctionnement général](#fonctionnement-général)
   2. [Choix d’optimisation](#choix-doptimisation)
   3. [Évaluation de la position](#évaluation-de-la-position)
   4. [Données d’ouvertures](#données-douvertures)
   5. [Optimisation et sélection de l’agent](#optimisation-et-sélection-de-lagent)
   6. [Limites connues](#limites-connues)
3. [Interface graphique](#interface-graphique)
4. [Organisation du projet](#organisation-du-projet)
5. [Documentation complémentaire](#documentation-complémentaire)
6. [Licence](#licence)

## Installation

### Prérequis

Le projet a besoin des éléments suivants :

- un compilateur compatible C++20 :
  - GCC ou Clang sous Linux ;
  - Clang sous macOS ;
  - MSVC récent sous Windows ;
- CMake 3.21 ou plus récent ;
- Ninja ;
- Python 3.10 ou plus récent ;
- Tkinter ;
- Pillow pour les images de pièces ;
- mypy pour la vérification statique de la GUI.

Le fichier `environment.yml` installe CMake, Ninja, Python 3.11, Tkinter,
Pillow et mypy. Le compilateur C++ reste celui du système, car son installation
dépend de la plateforme.

Sous Debian ou Ubuntu, le compilateur peut par exemple être installé avec :

```bash
sudo apt install build-essential
```

Sous Windows, il faut installer les outils de compilation C++ de Visual Studio
ou un autre compilateur C++20 avant de configurer CMake.

### Environnement Conda

Depuis la racine du projet :

```bash
conda env create -f environment.yml
conda activate chess-ai
```

Pour mettre à jour un environnement existant :

```bash
conda env update -f environment.yml --prune
conda activate chess-ai
```

Le canal `conda-forge` est utilisé seul. L’entrée `nodefaults` empêche le
mélange implicite de paquets provenant du canal `defaults`.

Conda facilite la reproductibilité des outils, mais ne rend pas deux
compilations C++ strictement identiques entre systèmes. Les benchmarks et les
entraînements consignent donc aussi le système, l’architecture, le compilateur
et le type de compilation.

### Compilation

La compilation recommandée pour jouer est la compilation Release :

```bash
cmake --preset release
cmake --build --preset release --parallel
```

Le script équivalent est :

```bash
scripts/build.sh release
```

Les presets disponibles sont :

| Preset | Usage | Répertoire produit |
|---|---|---|
| `debug` | Développement et assertions | `build/debug/` |
| `release` | Jeu et entraînement rapides | `build/release/` |
| `benchmark` | Mesures avec symboles de diagnostic | `build/benchmark/` |
| `sanitize` | Détection d’erreurs mémoire et de comportements indéfinis | `build/sanitize/` |
| `profile` | Profilage avec `gprof` | `build/profile/` |

Pour nettoyer les artefacts régénérables :

```bash
scripts/clean.sh
```

Cette commande retire les répertoires de compilation et les caches Python.
Elle ne supprime ni les données, ni le modèle actif, ni les résultats
d’entraînement placés dans `runs/`.

### Tests et vérifications

Le chemin le plus simple est :

```bash
scripts/test.sh debug
```

Ce script exécute :

1. mypy en mode strict sur la GUI Python ;
2. la configuration CMake ;
3. la compilation C++ ;
4. la suite de tests CTest.

Les mêmes étapes peuvent être lancées séparément :

```bash
python -m mypy
cmake --preset debug
cmake --build --preset debug --parallel
ctest --preset debug
```

La suite couvre notamment :

- les conversions FEN ;
- les coups ordinaires et spéciaux ;
- les états terminaux et les règles de nulle ;
- les restaurations de position et l’empreinte Zobrist ;
- des décomptes `perft` de référence ;
- la recherche et ses optimisations ;
- la bibliothèque d’ouvertures ;
- les composants de l’entraînement génétique ;
- le protocole textuel complet.

Pour lancer les instruments mémoire :

```bash
cmake --preset sanitize
cmake --build --preset sanitize --parallel
ctest --preset sanitize
```

### Exécutables produits

Après une compilation Release, les exécutables principaux sont :

| Exécutable | Rôle |
|---|---|
| `build/release/chess_engine` | Moteur interactif utilisé par la GUI |
| `build/release/train_genetic` | Entraînement et sélection des modèles |
| `build/release/fen_eval` | Évaluation statique d’une FEN |
| `build/release/self_play` | Partie directe entre deux modèles |
| `build/release/compare_models` | Comparaison d’un candidat au modèle actif |
| `build/release/build_opening_book` | Génération du livre et des positions d’entraînement |
| `build/release/search_benchmark` | Mesure détaillée de la recherche |
| `build/release/chess_tests` | Banc de tests autonome |

Exemple d’évaluation d’une position :

```bash
build/release/fen_eval \
  --fen "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" \
  --params data/models/current.json
```

Exemple de confrontation directe :

```bash
build/release/self_play \
  --white data/models/current.json \
  --black runs/<experience>/best_model.json \
  --max-halfmoves 300
```

## Description de l’agent de jeu

### Fonctionnement général

#### Architecture logique

L’application est divisée en couches à dépendances orientées :

```text
┌───────────────────────────────────────────────────────────────────┐
│                         Interface Tkinter                         │
│ gui/chess_gui                                                     │
└──────────────────────────────┬────────────────────────────────────┘
                               │ protocole texte
                               ▼
┌───────────────────────────────────────────────────────────────────┐
│ chess_protocol                                                    │
│ GameSession + CommandProcessor                                    │
└──────────────────────────────┬────────────────────────────────────┘
                               │
                               ▼
┌───────────────────────────────────────────────────────────────────┐
│ chess_search                                                      │
│ ChessAI + OpeningBook + Searcher + Evaluator                      │
└──────────────────────────────┬────────────────────────────────────┘
                               │
                               ▼
┌───────────────────────────────────────────────────────────────────┐
│ chess_core                                                        │
│ Position + MoveGenerator + FEN + Zobrist                          │
└───────────────────────────────────────────────────────────────────┘

chess_training dépend de chess_search pour évaluer et confronter les modèles.
```

Le cœur des règles ne dépend ni de l’interface, ni de l’entraînement. Cette
séparation permet de tester les règles et la recherche sans lancer de fenêtre.

#### Représentation d’une position

`chess::Position` contient :

- les 64 cases ;
- le camp au trait ;
- les quatre droits de roque ;
- la case éventuelle de prise en passant ;
- le compteur de demi-coups pour la règle des cinquante coups ;
- le numéro du coup complet ;
- les cases des deux rois ;
- les comptes de pièces maintenus incrémentalement ;
- l’empreinte Zobrist ;
- l’historique nécessaire à l’annulation et à la triple répétition.

La recherche applique un coup avec `makeMove`, explore la branche, puis
restaure exactement la position avec `undoMove`. Une copie du plateau n’est
donc pas créée pour chaque nœud.

#### Génération et légalité des coups

La génération suit deux étapes :

1. produire les coups pseudo-légaux à partir de la géométrie des pièces ;
2. jouer chaque candidat et retirer ceux qui laissent le roi actif en échec.

Cette méthode traite avec une logique commune les pièces clouées, les échecs
découverts et les prises en passant exposant le roi.

Deux modes de génération existent :

- `All` : tous les coups utiles à la recherche principale ;
- `Tactical` : captures et promotions nécessaires à la quiescence.

Lorsque le moteur doit seulement savoir s’il existe une réponse légale, par
exemple pour distinguer mat et pat, il s’arrête dès le premier coup trouvé.

#### Choix d’un coup

Le chemin général est :

```text
Position courante
      │
      ├── Coup d’ouverture disponible ? ── oui ──► coup du livre
      │
      └── non
           │
           ▼
  Approfondissement 1, 2, 3, ... jusqu’à la limite
           │
           ▼
  Classement des coups + negamax alpha-bêta
           │
           ├── profondeur normale atteinte ──► quiescence
           │
           └── mat / pat / nulle ────────────► score terminal
           │
           ▼
  Meilleur coup de la dernière profondeur entièrement terminée
```

L’évaluation est toujours calculée du point de vue des Blancs. Negamax inverse
le signe selon le camp au trait, ce qui permet de partager la même logique
récursive entre les deux joueurs.

#### Modes de recherche

| Mode | Ordre des coups | Réductions tardives | Usage conseillé |
|---|---|---|---|
| `classic` | Heuristiques tactiques et historiques | Non | Référence simple |
| `instinct` | Même base + développement, centralisation et sécurité approximative | Non | Recherche non sélective mieux ordonnée |
| `instinct_lmr` | Même ordre que `instinct` | Oui, sur certains coups calmes tardifs | Jeu profond et rapide |

Le mode `instinct` ne supprime aucun coup. Il modifie seulement l’ordre dans
lequel les coups sont visités. Le mode `instinct_lmr` réduit temporairement la
profondeur de certains coups jugés peu prometteurs, mais les recherche de
nouveau à profondeur normale s’ils améliorent la position.

### Choix d’optimisation

#### Approfondissement itératif

Au lieu de lancer directement une recherche de profondeur 8, le moteur cherche
successivement aux profondeurs 1, 2, 3, etc. Le meilleur coup de l’itération
précédente est essayé en premier à l’itération suivante.

Le travail des petites profondeurs n’est pas perdu :

- il améliore l’ordre des coups ;
- il remplit la table de transposition ;
- il garantit un coup complètement calculé si la limite de temps expire.

#### Élagage alpha-bêta

Negamax conserve une fenêtre `[alpha, bêta]`. Lorsqu’une branche montre qu’un
adversaire dispose déjà d’une réponse au moins aussi bonne qu’une option connue,
les variantes restantes de cette branche sont abandonnées. Le résultat reste
exact si l’ordre et la profondeur sont identiques ; seul le travail évité
change.

L’efficacité d’alpha-bêta dépend fortement de l’ordre des coups. Le moteur
essaie donc en priorité :

1. le meilleur coup fourni par la table de transposition ;
2. les promotions ;
3. les captures estimées favorables ;
4. les échecs ;
5. les `killer moves` ;
6. les coups valorisés par l’historique des coupures ;
7. le score du mode instinctif ;
8. les autres coups.

#### Table de transposition et Zobrist

Deux suites de coups peuvent atteindre la même position. Une table de
transposition fixe évite alors de recalculer le même sous-arbre.

La clé principale est une empreinte Zobrist mise à jour lors de chaque
mutation du plateau. Elle tient compte :

- des pièces et de leurs cases ;
- du camp au trait ;
- des droits de roque ;
- de la colonne de prise en passant.

Le compteur de demi-coups est ajouté à la clé utilisée par la recherche, car
deux plateaux visuellement identiques peuvent avoir un statut différent au
regard de la règle des cinquante coups.

La table possède une taille fixe d’un million d’emplacements par défaut. Une
collision remplace en priorité une entrée ancienne ou moins profonde. Cela
évite les allocations dynamiques pendant la recherche.

#### Recherche de quiescence

Une évaluation statique peut être trompeuse si la profondeur s’arrête au milieu
d’une suite de captures. La quiescence prolonge donc la recherche :

- hors échec, elle évalue la position courante puis explore les captures et
  promotions ;
- en échec, elle explore toutes les réponses légales ;
- elle s’arrête lorsque la position devient calme ou lorsque
  `quiescenceMaxPly` est atteint.

La profondeur de quiescence ne fait pas partie du génome entraîné. Elle est une
limite de calcul commune à tous les agents comparés.

#### Principal Variation Search

Le premier coup classé est recherché avec une fenêtre complète. Les coups
suivants sont d’abord testés avec une fenêtre très étroite. S’ils ne semblent
pas meilleurs, le moteur évite une recherche complète ; s’ils dépassent la
borne, ils sont recalculés normalement.

Cette optimisation est activée par défaut et peut être désactivée dans le
benchmark avec `--no-pvs`.

#### Fenêtres d’aspiration

À partir de la deuxième itération, le score précédent fournit une estimation
du nouveau score. Le moteur cherche d’abord autour de cette valeur avec une
fenêtre réduite, puis l’élargit si l’estimation était trop basse ou trop haute.

Les fenêtres d’aspiration sont utilisées avec `classic` et `instinct`. Elles
sont volontairement désactivées avec `instinct_lmr`, car leur combinaison
n’était pas assez stable sur le corpus de référence.

#### Réductions tardives prudentes

En mode `instinct_lmr`, un coup peut perdre un niveau de profondeur seulement
si toutes les conditions suivantes sont réunies :

- la profondeur restante est suffisante ;
- le niveau courant a atteint `lmrMinPly` ;
- les `lmrFullDepthMoves` premiers coups ont déjà été cherchés complètement ;
- le coup est calme ;
- le roi n’est pas en échec ;
- le coup ne donne pas échec.

Si l’essai réduit améliore `alpha`, le coup est recherché de nouveau à
profondeur normale. Cette vérification est le mécanisme de sécurité principal
des LMR.

#### Optimisations d’implémentation

Les choix retenus visent un compromis entre performance et risque de
régression :

- génération directe des coups tactiques pendant la quiescence ;
- calcul des métadonnées d’ordre pendant la vérification de légalité ;
- application et annulation incrémentales des coups ;
- comptes de pièces et cases des rois maintenus dans `Position` ;
- table de transposition fixe ;
- tampon de candidats réutilisé pour le test « existe-t-il un coup légal ? » ;
- optimisation interprocédurale en Release lorsqu’elle est disponible ;
- compilation avec avertissements stricts.

Le benchmark actuel atteint la profondeur 8 sur les cinq positions de
référence en environ 14,8 secondes au total avec `instinct_lmr`, contre environ
87 secondes avant le lot d’optimisations. Ces nombres décrivent la machine de
référence et ne constituent pas une garantie sur un autre système. La méthode
complète est détaillée dans
[`docs/performance.md`](docs/performance.md).

### Évaluation de la position

#### Convention de score

Un score positif favorise les Blancs et un score négatif favorise les Noirs.
Les valeurs ressemblent à des centipions, mais elles ne doivent pas être
interprétées comme une probabilité de victoire exacte : les poids sont
entraînables et le moteur ne réalise aucune calibration probabiliste.

L’évaluation générale peut être résumée ainsi :

```text
score =
    matériel blanc - matériel noir
  + caractéristiques positionnelles blanches
  - caractéristiques positionnelles noires
```

#### Matériel

Chaque type de pièce possède une valeur configurable :

| Paramètre JSON | Signification |
|---|---|
| `pawn` | Valeur d’un pion |
| `knight` | Valeur d’un cavalier |
| `bishop` | Valeur d’un fou |
| `rook` | Valeur d’une tour |
| `queen` | Valeur d’une dame |
| `king` | Valeur technique du roi dans certains classements |

Le compte des pièces est maintenu pendant `makeMove` et `undoMove`. Le moteur
n’a donc pas besoin de recompter le matériel sur les 64 cases à chaque feuille.

#### Structure des pions

Le moteur évalue :

- les pions doublés : plusieurs pions du même camp sur une colonne ;
- les pions isolés : aucun pion ami sur les colonnes voisines ;
- les pions protégés par un autre pion ;
- les pions passés : aucun pion adverse devant eux sur la même colonne ou une
  colonne voisine ;
- l’avancement d’un pion passé vers sa promotion.

Les poids correspondants sont :

- `doubledPawnPenalty` ;
- `isolatedPawnPenalty` ;
- `protectedPawnBonus` ;
- `passedPawnBonus`.

#### Activité et développement

La mobilité compte les destinations pseudo-légales accessibles aux pièces.
Elle est volontairement approximative : vérifier la légalité complète de
chaque destination rendrait l’évaluation presque aussi coûteuse qu’une
nouvelle génération de coups.

Le moteur ajoute également :

- `bishopPairBonus` si un camp possède au moins deux fous ;
- `undevelopedMinorPenalty` pour les fous et cavaliers encore sur leurs cases
  initiales ;
- `kingShieldBonus` pour les pions amis placés devant le roi.

#### Ce que l’évaluation ne contient plus

L’ancienne table `pieceSquareTable` et les 768 bonus pièce-case ont été
entièrement supprimés. Le mode instinctif ne dépend donc pas d’une table cachée
de cases apprises. Il utilise des propriétés calculées comme le développement,
la centralisation et le risque approximatif de la case d’arrivée.

L’évaluation n’emploie actuellement ni réseau neuronal, ni table de finale, ni
phases distinctes d’ouverture/milieu/finale.

### Données d’ouvertures

#### Source

Les fichiers sources `data/openings/source/a.tsv` à `e.tsv` proviennent du
projet `lichess-org/chess-openings` et sont distribués sous CC0 1.0. Le texte
complet de cette licence est conservé dans
[`data/openings/source/COPYING.txt`](data/openings/source/COPYING.txt).

#### Génération

La commande recommandée est :

```bash
scripts/generate_opening_book.sh
```

Elle :

1. compile `build_opening_book` ;
2. lit les cinq fichiers TSV ;
3. convertit les coups SAN en coups UCI ;
4. rejoue chaque ligne pour vérifier sa légalité ;
5. écrit les lignes théoriques dans
   `data/openings/generated/book.txt` ;
6. extrait des positions finales non terminales dans
   `data/openings/generated/training_positions.fen`.

Le jeu de données généré actuel contient 3 690 lignes théoriques et
2 905 positions d’entraînement uniques.

#### Indexation

Le livre indexe une position avec les quatre champs FEN qui influencent les
coups disponibles :

- placement des pièces ;
- camp au trait ;
- droits de roque ;
- case de prise en passant.

Les compteurs de demi-coups et de coups complets sont ignorés pour regrouper
les positions équivalentes atteintes par des ordres de coups différents.

Chaque occurrence d’un coup dans les lignes sources augmente son poids.

#### Modes de sélection

| Mode | Comportement |
|---|---|
| `chill` | Écarte les variantes très rares puis effectue un tirage pondéré |
| `competition` | Choisit systématiquement le coup légal de poids maximal |

Le mode `chill` conserve de la variété. Le mode `competition` est déterministe
à position et livre identiques.

Pour désactiver complètement le livre, lancer le moteur avec `--no-book`.

### Optimisation et sélection de l’agent

#### Ce qui est entraîné

Un individu contient :

- le mode de recherche fixé par l’expérience ;
- la profondeur de recherche, si ses bornes ne sont pas identiques ;
- les réglages LMR fixés par l’espace de recherche ;
- les six valeurs matérielles ;
- huit poids positionnels.

Le fichier
[`config/training/search_space.json`](config/training/search_space.json)
définit les valeurs modifiables et leurs bornes. Deux bornes égales figent un
paramètre. Dans la configuration actuelle, `searchDepth` et `kingValue` sont
figés.

Le fichier
[`config/training/genetic.json`](config/training/genetic.json) définit le coût
et le déroulement de l’expérience.

#### Lancer un entraînement

```bash
build/release/train_genetic \
  --config config/training/genetic.json
```

Par défaut, un lancement crée un dossier unique :

```text
runs/AAAAMMJJ-HHMMSS-mmm_seed42/
├── best_model.json
├── generations.csv
└── run_metadata.json
```

- `best_model.json` contient le meilleur modèle sauvegardé ;
- `generations.csv` contient les individus, scores et paramètres ;
- `run_metadata.json` contient la graine, les limites, l’environnement et la
  version logique de la recherche.

Le répertoire `runs/` est ignoré par Git. Une nouvelle expérience ne remplace
jamais automatiquement le modèle actif.

#### Paramètres principaux de l’expérience

| Paramètre | Rôle |
|---|---|
| `random_state` | Graine de reproductibilité |
| `generations` | Nombre total de générations |
| `max_halfmoves` | Limite des parties de validation |
| `quiescence_max_ply` | Limite tactique commune à tous les individus |
| `threads` | Nombre maximal d’évaluations concurrentes |
| `fitness_mode` | `matches` ou `corpus` |
| `training_positions_path` | Positions de départ des parties |
| `corpus_path` | Positions étiquetées du mode corpus |
| `search_space_path` | Bornes des paramètres entraînés |
| `generation_schedule` | Taille de population et mutation par période |

Chaque entrée de `generation_schedule` contient :

| Paramètre | Signification |
|---|---|
| `from`, `to` | Première et dernière génération concernées |
| `population_size` | Nombre pair d’individus |
| `mutation_individual_fraction` | Fraction des enfants effectivement mutés |
| `mutation_rate_scalar` | Probabilité de mutation de chaque poids numérique |
| `mutation_rate_depth` | Probabilité de modifier la profondeur de ±1 |
| `mutation_scale_fraction` | Amplitude maximale relative aux bornes du gène |

Dans la configuration actuelle, `mutation_rate_depth` vaut `0.00` parce que la
profondeur est figée à 4 dans l’espace de recherche.

Les options principales peuvent être remplacées ponctuellement :

```bash
build/release/train_genetic \
  --config config/training/genetic.json \
  --generations 20 \
  --population 16 \
  --seed 123 \
  --max-halfmoves 60 \
  --quiescence-depth 10 \
  --threads 4
```

#### Sélection par parties

Le mode `matches` suit plusieurs étages :

1. la population est mélangée avec la graine de l’expérience ;
2. chaque paire joue deux parties rapides avec couleurs inversées ;
3. le meilleur de chaque paire survit ;
4. les meilleurs survivants jouent des validations supplémentaires ;
5. jusqu’à deux champions antérieurs servent d’adversaires de référence ;
6. la moitié retenue est conservée directement par élitisme ;
7. les places restantes sont créées par croisement et mutation ;
8. une validation finale confronte chaque candidat à plusieurs voisins avec
   couleurs inversées.

La fitness d’une paire privilégie le résultat des parties, puis utilise des
signaux secondaires bornés :

- avantage matériel final ;
- vitesse de conversion d’une victoire ;
- résilience dans une défaite ou une nulle ;
- faible bonus d’efficacité pour une profondeur moins coûteuse.

L’évaluation finale du tournoi utilise des valeurs matérielles objectives et
fixes. Un individu ne peut donc pas améliorer artificiellement son résultat en
augmentant ses propres poids.

#### Sélection par corpus

Le mode `corpus` remplace les parties par une comparaison entre l’évaluation
de l’agent et un score cible :

```bash
build/release/train_genetic \
  --config config/training/genetic.json \
  --fitness-mode corpus \
  --corpus data/positions/training_corpus.tsv
```

La fitness augmente lorsque l’erreur absolue moyenne diminue. Ce mode est
beaucoup moins coûteux, mais sa qualité dépend directement de la taille, de la
diversité et de la fiabilité des scores du corpus.

`training_corpus.tsv` sert à l’apprentissage et `holdout_corpus.tsv` au
contrôle. Le corpus fourni est volontairement petit ; il constitue un test de
pipeline, pas une base suffisante pour entraîner un moteur de haut niveau.

#### Comparer et promouvoir un modèle

Pour comparer un candidat au modèle actif :

```bash
scripts/compare_model.sh runs/<experience>/best_model.json
```

La comparaison utilise le corpus de contrôle et deux parties à couleurs
inversées.

Pour promouvoir explicitement le résultat :

```bash
scripts/promote_model.sh runs/<experience>/best_model.json
```

Cette commande remplace `data/models/current.json`. Il est conseillé de
documenter simultanément la provenance et l’empreinte du modèle dans
`data/models/README.md`.

La méthodologie complète est décrite dans
[`docs/training.md`](docs/training.md).

### Limites connues

#### Niveau de jeu

Le moteur reste un projet pédagogique et expérimental. Une profondeur nominale
ne se compare pas directement à celle d’un autre moteur : la quiescence, les
réductions, l’ordre des coups et la puissance de la machine changent le nombre
réel de positions examinées.

Il ne faut pas interpréter l’objectif de profondeur 8 comme un classement Elo
garanti.

#### Représentation du plateau

Le plateau utilise un tableau de 64 cases et des parcours directs. Il
n’emploie pas de bitboards. Cette représentation est simple et robuste, mais
elle limite le débit maximal de génération et de détection des attaques.

Le profil actuel place principalement `isSquareAttacked` et l’évaluation
statique parmi les points chauds.

#### Recherche

Le moteur n’implémente pas :

- le `null-move pruning` ;
- les extensions sélectives avancées ;
- une évaluation statique des échanges complète ;
- les tables de finales ;
- le parallélisme à l’intérieur d’une recherche ;
- un protocole UCI complet ;
- l’apprentissage neuronal.

Ces absences sont volontaires. Les optimisations plus agressives augmenteraient
le risque de régression tactique ou la complexité du projet.

#### Évaluation

L’évaluation :

- ne distingue pas explicitement ouverture, milieu de partie et finale ;
- ne connaît pas les motifs stratégiques complexes ;
- mesure une mobilité pseudo-légale ;
- approxime la sécurité du roi avec son bouclier de pions ;
- n’analyse pas complètement les échanges avant la recherche.

Les poids génétiques ne compensent pas toutes les limites d’une fonction
d’évaluation structurellement simple.

#### Entraînement

Les parties génétiques sont coûteuses et leur nombre reste limité. Le tournoi
économique réduit le coût, mais ne remplace pas un tournoi complet contre une
large population d’adversaires.

Le mode corpus est rapide, mais les corpus livrés sont petits. Un résultat peut
surapprendre les positions d’entraînement et doit toujours être contrôlé sur
des positions séparées et des parties à couleurs inversées.

#### Reproductibilité

Une graine fixe rend les tirages reproductibles dans un même environnement,
mais les différences de compilateur, de bibliothèque standard, de matériel et
d’ordonnancement des threads peuvent modifier les temps et, dans certains cas,
le déroulement exact d’une expérience parallèle.

#### Protocole

Le protocole interactif est propre au projet. Les commandes emploient la
notation UCI des coups, mais l’exécutable n’est pas un moteur UCI complet
directement compatible avec toutes les interfaces d’échecs tierces.

## Interface graphique

### Lancement

Compiler d’abord le moteur, puis exécuter :

```bash
python -m gui.chess_gui
```

La GUI recherche automatiquement `chess_engine` dans :

1. `build/debug/` ;
2. `build/release/` ;
3. `build/benchmark/` ;
4. `build/sanitize/` ;
5. `build/`.

Un autre chemin peut être fourni comme premier argument :

```bash
python -m gui.chess_gui /chemin/vers/chess_engine
```

### Fonctions disponibles

La barre d’outils permet de :

- choisir entre humain contre IA et deux humains ;
- jouer les Blancs ou les Noirs ;
- choisir une profondeur de 1 à 10 ;
- sélectionner `classic`, `instinct` ou `instinct_lmr` ;
- sélectionner le mode d’ouverture `chill` ou `competition` ;
- démarrer une nouvelle partie ;
- annuler un tour ou un demi-coup selon le mode.

Le plateau :

- s’oriente du point de vue du joueur dans le mode humain contre IA ;
- affiche les destinations légales d’une pièce sélectionnée ;
- distingue visuellement déplacements et captures ;
- propose une fenêtre de promotion limitée aux choix légaux ;
- conserve un historique avec le bilan matériel ;
- affiche mat, échec, pat et nulle ;
- joue des motifs sonores non bloquants.

### Synchronisation avec le moteur

La GUI ne réimplémente pas les règles. Elle demande les coups légaux au moteur
et reconstruit le plateau depuis la FEN reçue.

La recherche de l’IA s’exécute dans un thread de travail pour ne pas figer la
boucle Tkinter. L’application du résultat revient ensuite sur le thread
principal.

Le protocole précis est documenté dans
[`docs/protocol.md`](docs/protocol.md).

### Ressources graphiques

Les images PNG des pièces Cburnett sont embarquées dans
`gui/chess_gui/assets/pieces/`. Elles évitent de dépendre d’une police Unicode
installée sur la machine. Leur provenance et leur licence BSD à trois clauses
sont détaillées dans
[`gui/chess_gui/assets/pieces/README.md`](gui/chess_gui/assets/pieces/README.md).

### Dépannage

Si la fenêtre signale que le moteur est introuvable :

```bash
cmake --preset release
cmake --build --preset release --target chess_engine --parallel
python -m gui.chess_gui
```

Si Tkinter est absent, recréer ou mettre à jour l’environnement Conda :

```bash
conda env update -f environment.yml --prune
conda activate chess-ai
```

Si les pièces n’apparaissent pas, vérifier que le dossier
`gui/chess_gui/assets/pieces/` est présent et contient les douze fichiers PNG.

## Organisation du projet

```text
.
├── apps/                    # exécutables principaux
├── benchmarks/              # corpus et résultats de performance
├── cmake/                   # avertissements, optimisation et instruments
├── config/training/         # expérience génétique et espace de recherche
├── data/
│   ├── models/              # modèle actif
│   ├── openings/            # sources et données générées
│   └── positions/           # corpus d’entraînement et de contrôle
├── docs/                    # documentation technique détaillée
├── gui/chess_gui/           # interface Tkinter et ressources
├── scripts/                 # commandes reproductibles
├── src/
│   ├── chess/               # règles et représentation
│   ├── protocol/            # session et protocole textuel
│   ├── search/              # recherche, ordre et évaluation
│   └── training/            # corpus, tournoi et génétique
├── tests/                   # tests unitaires, intégration et perft
├── tools/                   # évaluation, comparaison et génération
└── runs/                    # sorties locales ignorées par Git
```

La structure des dossiers n’accélère pas directement la recherche en mémoire.
Elle réduit toutefois les erreurs de dépendances, les écrasements de modèles et
les confusions entre sources, données générées et résultats expérimentaux.

## Documentation complémentaire

Le point d’entrée de la documentation détaillée est
[`docs/README.md`](docs/README.md).

| Document | Contenu |
|---|---|
| [`docs/architecture.md`](docs/architecture.md) | Couches, dépendances, invariants et flux |
| [`docs/performance.md`](docs/performance.md) | Méthode de benchmark, résultats et profil |
| [`docs/protocol.md`](docs/protocol.md) | Contrat textuel complet du moteur |
| [`docs/training.md`](docs/training.md) | Configuration et méthodologie d’entraînement |
| [`docs/glossary.md`](docs/glossary.md) | Définitions accessibles des termes techniques |

## Licence

Le projet principal est distribué sous licence
**Creative Commons Attribution – Pas d’Utilisation Commerciale 4.0
International (CC BY-NC 4.0)**. Le texte juridique complet se trouve dans
[`LICENCE`](LICENCE).

En résumé, cette licence autorise le partage et l’adaptation à condition :

- de créditer l’auteur ou les auteurs ;
- d’indiquer les modifications ;
- de ne pas utiliser le projet principalement à des fins commerciales ;
- de ne pas ajouter de restriction empêchant les droits accordés par la
  licence.

Ce résumé n’a pas de valeur juridique et ne remplace pas le texte de la
licence.

Deux ensembles de ressources conservent leurs licences propres :

- les données `lichess-org/chess-openings` dans `data/openings/source/` sont
  sous CC0 1.0 ;
- les images de pièces Cburnett sont sous licence BSD à trois clauses.

Les mentions et textes associés à ces ressources doivent être conservés lors
de leur redistribution.
