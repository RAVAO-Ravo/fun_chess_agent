# Performance, benchmarks et profilage

## Objectif de mesure

Le projet vise une profondeur nominale de 8 utilisable avec le mode
`instinct_lmr`, sans sacrifier les règles, la stabilité tactique de base ou la
lisibilité de l’implémentation.

Une optimisation n’est pas retenue uniquement parce qu’elle réduit un temps
sur une position. Elle doit être évaluée selon quatre axes :

1. le meilleur coup choisi ;
2. la profondeur entièrement terminée ;
3. le nombre et la nature des nœuds visités ;
4. le temps dans un environnement identifié.

Le [glossaire](glossary.md) explique les indicateurs techniques.

## Corpus de benchmark

### Corpus quotidien

`benchmarks/positions/search.tsv` contient cinq positions :

- position initiale ;
- Kiwipete, riche en coups et roques ;
- position tactique ;
- position défensive ;
- finale de tours.

Ce corpus est court afin de permettre plusieurs répétitions après chaque
modification.

### Corpus étendu

`benchmarks/positions/extended.tsv` contient trente positions et ajoute :

- plusieurs ouvertures et milieux de partie ;
- promotions ;
- prises en passant ;
- échecs, mats et pats ;
- finales ;
- positions où une réduction sélective peut changer l’ordre principal.

Le corpus étendu sert à rechercher une régression après qu’un gain a été
observé sur le corpus quotidien.

### Format

Chaque ligne non commentée contient :

```text
identifiant<TAB>catégorie<TAB>FEN
```

Les identifiants doivent rester stables pour permettre la comparaison des CSV
entre versions.

## Exécuter les mesures

### Benchmark quotidien

```bash
scripts/benchmark.sh 8
```

Le script :

1. configure le preset `benchmark` ;
2. compile `search_benchmark` ;
3. exécute les trois modes avec le modèle actif ;
4. écrit un résultat daté sous `runs/benchmarks/` ;
5. copie la dernière mesure dans `benchmarks/results/latest.csv`.

### Corpus étendu

```bash
scripts/benchmark.sh 6 benchmarks/positions/extended.tsv
```

Une profondeur plus faible est généralement suffisante pour le contrôle
étendu, dont le coût total est plus important.

### Commande directe

```bash
build/benchmark/search_benchmark \
  --positions benchmarks/positions/search.tsv \
  --params data/models/current.json \
  --mode all \
  --depth 8 \
  --output runs/manual_benchmark.csv
```

Options utiles :

| Option | Effet |
|---|---|
| `--mode classic` | Mesurer un seul mode |
| `--mode all` | Mesurer les trois modes |
| `--depth N` | Fixer la profondeur principale |
| `--time-ms N` | Ajouter une limite temporelle par position |
| `--no-pvs` | Désactiver Principal Variation Search |
| `--no-aspiration` | Désactiver les fenêtres d’aspiration |
| `--lmr-min-ply N` | Remplacer le premier niveau réductible |
| `--lmr-full-depth-moves N` | Remplacer le nombre de coups protégés |

Une comparaison doit modifier une seule famille d’options à la fois.

## Schéma CSV

Les premières lignes commencent par `#` et décrivent :

- version du schéma ;
- système ;
- architecture ;
- compilateur et version ;
- type de build ;
- options générales d’optimisation.

Les colonnes sont :

| Colonne | Signification |
|---|---|
| `id` | Identifiant stable de la position |
| `category` | Famille de la position |
| `mode` | Mode de recherche |
| `requested_depth` | Profondeur demandée |
| `completed_depth` | Dernière profondeur entièrement terminée |
| `best_move` | Coup UCI retenu |
| `score` | Score retourné |
| `time_us` | Durée en microsecondes |
| `nodes` | Nœuds de recherche principale |
| `qnodes` | Nœuds de quiescence |
| `nps` | Nœuds totaux par seconde |
| `tt_probes` | Consultations de la table |
| `tt_hits` | Entrées trouvées |
| `tt_entries` | Emplacements occupés |
| `cutoffs` | Coupures bêta |
| `stopped` | Arrêt causé par la limite de temps |

`nodes + qnodes` représente le total. La feuille où la recherche principale
bascule vers la quiescence n’est pas comptée deux fois.

## Résultat de référence

Les mesures suivantes ont été effectuées sous Linux x86-64 avec GCC 11.4, en
Release, avec le même modèle actif. La mesure optimisée est la médiane de trois
exécutions.

| Position | Avant | `instinct` optimisé | `instinct_lmr` prudent | Gain total |
|---|---:|---:|---:|---:|
| initiale | 20,410 s | 6,737 s | 4,109 s | ×4,97 |
| Kiwipete | 65,134 s | 20,806 s | 10,137 s | ×6,43 |
| tactique | 0,903 s | 0,391 s | 0,331 s | ×2,73 |
| défense | 0,001 s | 0,001 s | 0,001 s | ×1,67 |
| finale de tours | 0,557 s | 0,272 s | 0,192 s | ×2,90 |
| **Total** | **87,004 s** | **28,207 s** | **14,769 s** | **×5,89** |

Les cinq recherches terminent la profondeur 8 et conservent les mêmes coups
principaux que la référence antérieure.

Ces temps ne sont pas des objectifs universels. Sur une autre machine, les
nombres de nœuds sont plus utiles que les secondes pour vérifier que
l’algorithme se comporte de manière comparable.

Les fichiers associés sont :

- `benchmarks/results/history/reference_before_refactor.csv` ;
- `benchmarks/results/history/optimized_after_refactor.csv` ;
- `benchmarks/results/latest.csv`.

Les fichiers `mode_comparison_depth4.csv` et
`mode_comparison_depth5.csv` documentent les comparaisons intermédiaires des
modes et restent utiles pour comprendre le choix des LMR.

## Origine des gains

### Génération tactique directe

L’ancienne quiescence générait tous les coups puis retirait les coups calmes.
Le mode tactique produit maintenant directement les captures et promotions.

Gain attendu :

- moins de créations de `Move` ;
- moins de vérifications de légalité inutiles ;
- meilleure proportion de travail tactiquement pertinent.

### Métadonnées d’ordre pendant la légalité

Le classement doit savoir si un coup donne échec et si sa destination est
attaquée. Ces informations sont maintenant produites pendant que le coup est
déjà appliqué pour sa validation.

Cela évite une seconde paire `makeMove`/`undoMove` uniquement pour le tri.

### Approfondissement et meilleur coup précédent

Le meilleur coup de la profondeur précédente est prioritaire. Cette
amélioration augmente les coupures et aide les fenêtres d’aspiration.

### Table fixe

La table de transposition évite `unordered_map` et les allocations par entrée.
Son coût d’accès est constant. La génération logique protège les résultats
récents.

### Principal Variation Search

Les coups secondaires sont testés avec une fenêtre nulle avant une éventuelle
recherche complète. Le gain dépend directement de la qualité de l’ordre.

### LMR prudentes

Les coups calmes tardifs perdent un niveau lors du premier essai. Un coup qui
améliore alpha est recalculé. Sur le corpus quotidien, ce mécanisme réduit
presque de moitié le temps restant après les optimisations exactes.

## Exactitude et sélectivité

`classic` et `instinct` restent non sélectifs quant à la profondeur nominale :
ils visitent tous les coups légaux à la profondeur prévue, sous réserve des
coupures prouvées alpha-bêta.

`instinct_lmr` est sélectif. À profondeur 5, les trente positions étendues
conservent le même meilleur coup que `instinct`. À profondeur 6, lorsque les
réductions deviennent actives, cinq positions d’ouverture choisissent une
variante de valeur proche ; les positions tactiques, défensives et de finale
conservent leur coup.

Ce compromis justifie la présence des deux modes :

- utiliser `instinct` pour une référence sans réduction ;
- utiliser `instinct_lmr` lorsque le temps ou la profondeur importe davantage.

## Profilage

### Commande

```bash
scripts/profile.sh 6
```

Le résultat est écrit dans :

```text
runs/profiles/<identifiant>/
├── search_results.csv
└── gprof.txt
```

Le script utilise le mode `classic` afin que les réductions sélectives ne
masquent pas le coût des primitives.

### Profil actuel

Après optimisation, le temps propre est principalement attribué à :

- `MoveGenerator::isSquareAttacked` : environ 31 % ;
- `Evaluator::evaluate` : environ 15 % ;
- fonctions de caractéristiques d’évaluation : environ 7 % ;
- génération des coups glissants : environ 6 % ;
- Zobrist et `Position::undoMove` : environ 5 % chacun.

Les allocations de `std::vector` ne ressortent pas comme point chaud majeur.
Une liste fixe de coups offrirait donc un rapport gain/risque moins favorable
que l’amélioration de la détection des attaques ou de l’évaluation.

## Protocole d’acceptation d’une optimisation

### Avant la modification

1. compiler le preset Benchmark ;
2. exécuter au moins trois fois le corpus quotidien ;
3. conserver la médiane ;
4. enregistrer le commit, le modèle et les options.

### Après la modification

1. exécuter les tests Debug ;
2. exécuter les tests Sanitize ;
3. répéter exactement le benchmark ;
4. comparer les coups et profondeurs ;
5. comparer `nodes`, `qnodes`, `nps`, hits TT et coupures ;
6. contrôler le corpus étendu ;
7. profiler si le gain ne correspond pas à l’hypothèse.

### Critères

Une modification peut être retenue si :

- elle corrige une erreur sans nouvelle régression ;
- ou elle apporte un gain reproductible ;
- ou elle simplifie nettement le code à performance équivalente.

Une modification doit être rejetée ou retravaillée si :

- elle accélère une position mais ralentit fortement les autres ;
- elle change des coups tactiques sans explication ;
- elle réduit seulement le temps parce qu’elle termine moins profondément ;
- elle rend les résultats non reproductibles ;
- sa complexité n’est pas couverte par des tests.

## Interprétation prudente

### Temps

Le temps varie avec :

- fréquence du processeur ;
- charge du système ;
- température et limitation thermique ;
- compilateur ;
- options de compilation ;
- activité d’autres processus.

Utiliser une médiane et éviter les mesures uniques.

### Nœuds par seconde

Une hausse de NPS ne prouve pas un meilleur moteur. Une opération plus coûteuse
par nœud peut réduire suffisamment l’arbre pour gagner au total.

### Nombre de nœuds

Une baisse peut signifier :

- meilleur ordre ;
- meilleure table ;
- élagage plus efficace ;
- ou réduction trop agressive.

Le meilleur coup et le corpus tactique permettent de distinguer ces cas.

### Profondeur

La profondeur nominale ne compte pas uniformément :

- la quiescence prolonge certaines feuilles ;
- les LMR réduisent certains coups ;
- les mats peuvent arrêter tôt ;
- le livre d’ouvertures ne recherche aucun nœud.

## Optimisations volontairement non retenues

### Null-move pruning

Non implémenté en raison du risque dans les finales et zugzwangs.

### Bitboards

Ils pourraient accélérer les attaques et mouvements, mais imposeraient une
refonte complète de la représentation, de la génération et des tests.

### Évaluation entièrement incrémentale

Elle éviterait le parcours positionnel des 64 cases, mais augmenterait
fortement la quantité d’état à restaurer lors de `undoMove`.

### Liste de coups fixe

Le profil ne montre pas actuellement les allocations de vecteurs comme coût
dominant.

Ces mécanismes ne sont pas interdits à l’avenir. Ils nécessitent simplement une
hypothèse mesurable et une validation plus large que leur coût
d’implémentation.
