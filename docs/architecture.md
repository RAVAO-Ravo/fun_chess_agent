# Architecture technique

## Objectif

L’architecture sépare les règles, la recherche, l’entraînement, le protocole et
l’interface. Le but est de permettre :

- de tester les règles sans instancier une IA ;
- de mesurer la recherche sans lancer la GUI ;
- d’entraîner des paramètres sans modifier le modèle actif ;
- de remplacer un client sans toucher au moteur ;
- de faire évoluer une couche sans créer de dépendance circulaire.

Le [glossaire](glossary.md) définit les termes propres aux moteurs d’échecs.

## Graphe des dépendances

```text
                                     ┌──────────────────┐
                                     │ gui/chess_gui    │
                                     │ processus client │
                                     └────────┬─────────┘
                                              │ stdin/stdout
                                              ▼
┌──────────────┐     ┌──────────────┐     ┌────────────────┐
│ chess_core   │ ──► │ chess_search │ ──► │ chess_protocol │
└──────┬───────┘     └──────┬───────┘     └────────────────┘
       │                    │
       └────────────────────┼──────────────► outils de mesure
                            │
                            ▼
                     ┌────────────────┐
                     │ chess_training │
                     └────────────────┘
```

Dans CMake :

- `chess_core` ne dépend d’aucune autre bibliothèque du projet ;
- `chess_search` dépend de `chess_core` ;
- `chess_protocol` dépend de `chess_search`, qui expose déjà le cœur ;
- `chess_training` dépend de `chess_search` ;
- les exécutables lient uniquement les bibliothèques dont ils ont besoin.

Une dépendance vers la GUI depuis le C++ est interdite. Une dépendance de
`chess_core` vers la recherche ou l’entraînement créerait également une
inversion de responsabilités.

## Vue par répertoire

| Répertoire | Responsabilité |
|---|---|
| `src/chess/` | Règles, état, coups, FEN et hachage |
| `src/search/` | Choix d’un coup, exploration, classement et évaluation |
| `src/protocol/` | Session humain/IA et commandes textuelles |
| `src/training/` | Corpus, tournoi, génétique et configuration |
| `apps/` | Points d’entrée destinés à l’utilisateur |
| `tools/` | Outils spécialisés et composables |
| `gui/chess_gui/` | Client graphique du protocole |
| `tests/` | Tests unitaires, d’intégration et perft |
| `benchmarks/` | Mesures de performance reproductibles |
| `data/` | Sources, données générées et modèle actif |

## `chess_core` : règles et représentation

### Types simples

- `Color` représente Blancs, Noirs ou absence de couleur ;
- `PieceType` représente le type d’une pièce ;
- `Piece` associe un type et une couleur ;
- `Square` représente une ligne et une colonne, avec une valeur invalide par
  défaut ;
- `Move` contient origine, destination, promotion et drapeaux de règle.

Ces types possèdent des accesseurs triviaux définis dans les headers pour
faciliter leur inlining. Ils ne contiennent aucune dépendance vers la recherche.

### `Position`

`Position` est la source de vérité sur l’état de la partie. Elle contient :

- `squares_` : tableau des 64 cases ;
- `pieceCounts_` : compte par couleur et type ;
- `sideToMove_` : camp au trait ;
- les quatre droits de roque ;
- `enPassantSquare_` ;
- les compteurs FEN ;
- les cases des rois ;
- `zobristHash_` ;
- `history_`.

#### Invariants

Après chaque opération publique :

1. une pièce vide possède la couleur `None` ;
2. les comptes de pièces correspondent au tableau ;
3. les cases de rois correspondent aux rois présents ;
4. l’empreinte Zobrist correspond aux pièces et droits courants ;
5. le dernier élément de l’historique suffit à annuler le dernier coup ;
6. `makeMove` suivi de `undoMove` restaure exactement la position antérieure.

`setPiece`, `setSideToMove`, `setCastlingRights` et
`setEnPassantSquare` centralisent les mises à jour de cache. Une nouvelle
mutation de plateau ne doit pas modifier directement un champ dérivé sans
préserver ces invariants.

#### Application d’un coup

```text
validation minimale de l’origine
             │
             ▼
sauvegarde du GameState antérieur
             │
             ▼
déplacement, capture ou promotion
             │
             ├── roque : déplacer aussi la tour
             └── en passant : retirer le pion sur sa vraie case
             │
             ▼
droits de roque + case en passant + compteurs
             │
             ▼
changement du trait + ajout à l’historique
```

`makeMove` suppose que le générateur a déjà construit les drapeaux spéciaux.
Pour jouer une chaîne UCI provenant de l’extérieur, il faut d’abord appeler
`findLegalMove`, qui retrouve le coup légal complet.

### `MoveGenerator`

Le générateur distingue :

- les coups pseudo-légaux, fondés sur la géométrie des pièces ;
- les coups légaux, qui ne laissent pas le roi en échec ;
- les coups tactiques destinés à la quiescence ;
- le test court `hasAnyLegalMove`.

La légalité est vérifiée avec :

```text
makeMove(candidat)
isInCheck(camp ayant joué)
undoMove()
```

Cette méthode évite de maintenir une seconde logique particulière pour les
clouages, découvertes et prises en passant.

Le mode d’annotation `Ordering` ajoute à chaque coup :

- s’il donne échec ;
- si sa destination est attaquée.

Ces calculs ne sont effectués que pour la recherche, pas pour les appels
ordinaires aux règles.

### FEN et Zobrist

`Fen` convertit une position depuis et vers les six champs FEN.

`Zobrist` fournit des clés déterministes pour :

- chaque couple pièce-case ;
- le camp au trait ;
- chaque droit de roque ;
- chaque colonne de prise en passant ;
- le compteur de demi-coups ajouté aux clés de recherche.

La méthode `Zobrist::hash` recalcule une empreinte complète et sert notamment
à vérifier les mises à jour incrémentales.

## `chess_search` : décision de l’agent

### `ChessAI`

`ChessAI` est la façade publique de la recherche :

1. vérifier si la bibliothèque possède un coup encore légal ;
2. sinon construire un `Searcher` avec les paramètres courants ;
3. mémoriser les statistiques du dernier calcul ;
4. retourner le meilleur coup.

Un coup du livre remet les statistiques à zéro, car aucun arbre n’a été
exploré.

### `Searcher`

`Searcher` possède l’état temporaire d’un appel :

- paramètres bornés ;
- évaluateur ;
- table de transposition ;
- heuristiques d’ordre ;
- limites ;
- statistiques ;
- échéance ;
- drapeau d’arrêt.

Il n’est pas conçu pour deux appels simultanés sur la même instance.
`ChessAI::analyze` crée actuellement une instance par recherche.

### Flux de recherche

```text
search(position, limites)
│
├── copie sans mutation de la racine de l’appelant
├── génération des coups légaux
├── ordre initial
│
└── pour profondeur = 1..maxDepth
    │
    ├── fenêtre complète ou aspiration
    ├── searchRoot
    │   ├── premier coup : fenêtre complète
    │   └── autres coups : PVS
    │
    ├── échec bas/haut : élargir la fenêtre
    └── publier l’itération seulement si elle est complète
```

La récursion `negamax` :

1. vérifie l’échéance ;
2. bascule en quiescence à profondeur nulle ;
3. traite les nulles de règle ;
4. consulte la table de transposition ;
5. génère et classe les coups ;
6. applique PVS et éventuellement LMR ;
7. enregistre les coupures calmes ;
8. stocke la borne obtenue dans la table.

### Scores de mat dans la table

Un score de mat encode aussi la distance au mat. Lorsqu’une entrée est stockée
à un niveau et relue à un autre, `scoreToTable` et `scoreFromTable` compensent
le niveau courant. Sans cette normalisation, la table pourrait préférer un mat
plus lent ou retarder une défaite de manière incohérente.

### `MoveOrdering`

Le classement mélange plusieurs sources dont les ordres de grandeur sont
séparés :

- coup préféré de la table ou de l’itération précédente ;
- promotion ;
- capture et estimation d’échange ;
- échec ;
- killer ;
- historique ;
- instinct.

Le tri est stable : deux coups de même score conservent leur ordre de
génération, ce qui aide la reproductibilité.

### `TranspositionTable`

La table utilise un tableau fixe et un adressage direct :

```text
index = clé % nombre_d_emplacements
```

Un emplacement contient la clé complète, l’entrée et un drapeau d’occupation.
Une entrée est remplacée si :

- l’emplacement est vide ;
- la clé est identique ;
- l’entrée appartient à une ancienne recherche ;
- la nouvelle profondeur est au moins égale.

Les types de bornes sont :

- `Exact` : score entièrement établi dans la fenêtre ;
- `Lower` : score au moins égal à la valeur, après coupure haute ;
- `Upper` : score au plus égal à la valeur, après échec bas.

### `Evaluator`

L’évaluateur reste sans état mutable. Il extrait en un parcours :

- pions par colonne et par case ;
- cases des pions ;
- cases des rois ;
- mobilité ;
- nombre de fous ;
- pièces mineures non développées.

Le matériel utilise les comptes déjà maintenus par `Position`.

## `chess_protocol` : frontière avec les clients

### `GameSession`

Une session associe :

- une position ;
- un `ChessAI` ;
- le camp humain ;
- des limites de recherche facultatives.

Elle empêche un coup humain lorsque ce n’est pas le trait de l’humain et
annule le nombre de demi-coups nécessaire pour lui rendre le trait.

### `CommandProcessor`

`CommandProcessor` transforme une ligne en une action atomique. Les exceptions
de validation sont converties en lignes `error`, afin que le processus reste
disponible après une mauvaise commande.

La classe reçoit un `std::ostream`, ce qui permet de tester le protocole avec un
flux mémoire sans créer de processus.

Le contrat détaillé se trouve dans [protocol.md](protocol.md).

## `chess_training` : optimisation des paramètres

### Types

- `Individual` : paramètres, identité, fitness et bilan ;
- `Population` : collection et mélange reproductible ;
- `SearchSpace` : bornes de chaque gène ;
- `CorpusSample` : position et score cible ;
- `Tournament` : partie entre deux individus ;
- `GeneticTrainer` : orchestration complète.

### Flux d’une génération

```text
population
    │
    ├── redimensionnement selon le calendrier
    ├── remise à zéro des bilans
    ├── mélange avec la graine
    │
    ├── mode matches ─► paires à couleurs inversées
    │                     └► validation des survivants
    │
    └── mode corpus ──► erreur sur positions étiquetées
                          │
                          ▼
                    moitié survivante
                          │
                 ┌────────┴────────┐
                 │                 │
             élitisme      croisement + mutation
                 │                 │
                 └────────┬────────┘
                          ▼
                  génération suivante
```

Les calculs parallèles renvoient leur indice de paire. Les résultats sont
réinsérés dans cet ordre, indépendamment de l’ordre de fin des threads.

## Interface graphique et concurrence

La GUI crée `chess_engine` avec `subprocess.Popen` et communique par flux texte.

Tkinter doit rester utilisé depuis son thread principal. Lors d’un coup IA :

1. la GUI se marque occupée ;
2. un thread de travail attend la réponse du moteur ;
3. ce thread ne modifie aucun widget ;
4. `after(0, ...)` reprogramme l’application de la réponse sur Tkinter.

Le moteur lui-même reste synchrone : une commande doit finir avant la suivante.

## Cycle de vie des données

```text
data/openings/source/*.tsv
          │
          ▼
build_opening_book
          │
          ├──► data/openings/generated/book.txt
          └──► data/openings/generated/training_positions.fen

config + données
          │
          ▼
train_genetic
          │
          ▼
runs/<id>/{best_model,generations,run_metadata}
          │
          ▼ validation explicite
scripts/promote_model.sh
          │
          ▼
data/models/current.json
```

Les données sources et le modèle actif sont versionnables. Les sorties
intermédiaires `runs/` sont locales et ignorées.

## Stratégie de tests

| Couche | Tests principaux |
|---|---|
| Types et position | `tests/unit/chess/` |
| Génération exhaustive | `tests/perft/` |
| Recherche et livre | `tests/unit/search/` |
| Génétique | `tests/unit/training/` |
| Protocole complet | `tests/integration/` |
| Performance | `benchmarks/` |
| Typage GUI | mypy strict |

Un benchmark ne remplace pas un test fonctionnel. Un test vérifie la justesse ;
un benchmark mesure le coût et surveille le meilleur coup.

## Règles d’évolution

### Ajouter une règle

1. modifier `Position` ou `MoveGenerator` ;
2. préserver `makeMove`/`undoMove` ;
3. ajouter un test unitaire ;
4. ajouter ou vérifier un cas perft ;
5. contrôler Zobrist si l’état de la position change.

### Ajouter une heuristique de recherche

1. rendre le mécanisme activable pour la mesure ;
2. ajouter des statistiques si nécessaire ;
3. vérifier les positions tactiques ;
4. comparer le nombre de nœuds et le meilleur coup ;
5. documenter les conditions de sécurité.

### Ajouter un paramètre entraînable

1. ajouter le champ à `EvaluationParameters` ;
2. définir une valeur par défaut et des bornes absolues ;
3. charger et sauvegarder le JSON ;
4. ajouter les bornes dans `SearchSpace` ;
5. inclure le champ dans génération, croisement et mutation ;
6. l’ajouter au journal CSV ;
7. tester ses bornes et sa sérialisation ;
8. documenter son sens.

### Ajouter une commande

1. définir une réponse non ambiguë ;
2. implémenter la commande dans `CommandProcessor` ;
3. ajouter un test d’intégration ;
4. mettre à jour [protocol.md](protocol.md) ;
5. adapter les clients concernés.

## Influence de l’arborescence sur les performances

L’organisation des fichiers ne change pas la vitesse de la recherche une fois
le programme chargé. Les performances dépendent des structures en mémoire, des
algorithmes et du compilateur.

L’arborescence améliore toutefois indirectement la qualité :

- dépendances plus simples ;
- recompilations ciblées ;
- séparation des résultats et des sources ;
- moindre risque de charger un ancien modèle ;
- commandes reproductibles ;
- audits et profils plus faciles.
