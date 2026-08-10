# Entraînement, sélection et reproductibilité

## Objectif

L’entraînement ne découvre pas les règles des échecs et ne remplace pas
l’algorithme de recherche. Il optimise un nombre limité de valeurs et de poids
utilisés par une recherche déjà définie.

Cette séparation est essentielle :

- le moteur décide quelles caractéristiques existent ;
- l’espace de recherche décide lesquelles peuvent changer ;
- l’algorithme génétique propose des combinaisons ;
- les parties ou le corpus mesurent ces combinaisons ;
- une validation humaine explicite décide de promouvoir un résultat.

Le [glossaire](glossary.md) définit individu, génome, fitness, mutation et
génération.

## Fichiers de configuration

### Configuration de l’expérience

[`config/training/genetic.json`](../config/training/genetic.json) décrit le
coût, les données et le calendrier.

Exemple abrégé :

```json
{
  "random_state": 42,
  "generations": 100,
  "max_halfmoves": 80,
  "quiescence_max_ply": 10,
  "threads": 4,
  "run_root": "runs",
  "fitness_mode": "matches",
  "corpus_path": "../../data/positions/training_corpus.tsv",
  "search_space_path": "search_space.json",
  "training_positions_path": "../../data/openings/generated/training_positions.fen",
  "generation_schedule": []
}
```

Les chemins relatifs contenus dans ce JSON sont résolus depuis le dossier du
fichier de configuration. Cela explique les `../../` employés pour rejoindre
`data/` depuis `config/training/`.

| Clé | Type | Rôle |
|---|---|---|
| `random_state` | entier positif ou nul | Graine pseudo-aléatoire |
| `generations` | entier positif ou nul | Nombre de générations |
| `max_halfmoves` | entier | Longueur maximale des validations |
| `quiescence_max_ply` | entier ≥ 1 | Borne tactique commune |
| `threads` | entier ≥ 1 | Évaluations concurrentes |
| `run_root` | chemin | Parent des dossiers de lancement |
| `fitness_mode` | chaîne | `matches` ou `corpus` |
| `corpus_path` | chemin | Corpus étiqueté |
| `search_space_path` | chemin | Bornes des gènes |
| `training_positions_path` | chemin | FEN de départ des parties |
| `generation_schedule` | liste | Réglages par période |

Le chargeur accepte encore quelques anciens alias camelCase pour la
compatibilité, mais les nouvelles configurations doivent utiliser snake_case.

### Calendrier des générations

Une entrée contient :

```json
{
  "from": 1,
  "to": 40,
  "population_size": 32,
  "mutation_individual_fraction": 0.70,
  "mutation_rate_scalar": 0.40,
  "mutation_rate_depth": 0.00,
  "mutation_scale_fraction": 0.10
}
```

#### `from` et `to`

Bornes inclusives de la période, numérotées à partir de 1.

#### `population_size`

Nombre d’individus. Il doit être strictement positif et pair, car la première
sélection forme des paires.

Lorsque le calendrier réduit la population, celle-ci est mélangée avant
troncature. Lorsqu’il l’augmente, de nouveaux enfants sont créés depuis les
individus existants.

#### `mutation_individual_fraction`

Probabilité qu’un nouvel enfant passe par l’opérateur de mutation.

Avec `0.70`, environ 70 % des enfants issus du croisement reçoivent une
tentative de mutation ; les autres sont seulement bornés.

#### `mutation_rate_scalar`

Probabilité appliquée indépendamment à chaque poids numérique d’un enfant
muté.

Avec `0.40`, chaque valeur de pièce ou poids positionnel a 40 % de chance
d’être modifié, sous réserve que l’enfant fasse partie de la fraction mutée.

#### `mutation_rate_depth`

Probabilité de modifier `searchDepth` de `+1` ou `-1`.

Dans la configuration actuelle, cette valeur est `0.00` et l’espace fixe la
profondeur à `[4, 4]`. Une mutation de profondeur n’aurait donc aucun effet
utile.

#### `mutation_scale_fraction`

Amplitude maximale du déplacement, exprimée comme fraction de la largeur des
bornes du gène.

Pour des bornes `[0, 80]` et une fraction `0.10`, la mutation ajoute un entier
compris approximativement entre `-8` et `+8`, puis remet la valeur dans les
bornes.

### Espace de recherche

[`config/training/search_space.json`](../config/training/search_space.json)
sépare les propriétés fixées de l’expérience et les intervalles entraînables.

```json
{
  "searchMode": "instinct_lmr",
  "lmrMinPly": 3,
  "lmrFullDepthMoves": 4,
  "parameters": {
    "searchDepth": {
      "type": "int",
      "bounds": [4, 4]
    }
  }
}
```

Les champs de recherche sont :

| Champ | Sens |
|---|---|
| `searchMode` | Mode commun à tous les individus |
| `lmrMinPly` | Premier niveau où une LMR est autorisée |
| `lmrFullDepthMoves` | Nombre de premiers coups protégés |

Les paramètres à bornes sont :

| Paramètre | Catégorie |
|---|---|
| `searchDepth` | Coût de recherche |
| `pawnValue` à `kingValue` | Matériel |
| `doubledPawnPenalty` | Structure de pions |
| `isolatedPawnPenalty` | Structure de pions |
| `passedPawnBonus` | Structure de pions |
| `protectedPawnBonus` | Structure de pions |
| `mobilityBonus` | Activité |
| `bishopPairBonus` | Coordination |
| `kingShieldBonus` | Sécurité du roi |
| `undevelopedMinorPenalty` | Développement |

Deux bornes égales figent un paramètre. Une borne d’expérience ne peut pas
dépasser efficacement les bornes absolues codées dans
`clampParameters` : le modèle final est toujours projeté dans les deux
ensembles.

## Génome actuel

Le génome est volontairement petit. Il ne contient plus :

- `pieceSquareTable` ;
- `PieceSquareBonus` ;
- 768 valeurs pièce-case ;
- un historique de positions propre à l’individu.

Le mode instinctif est une formule calculée par le moteur. L’entraînement
ajuste certains poids qu’elle partage avec l’évaluation, mais ne mémorise pas
une liste de « bons coups » position par position.

## Démarrer une expérience

### Compilation

```bash
cmake --preset release
cmake --build --preset release --target train_genetic --parallel
```

### Lancement normal

```bash
build/release/train_genetic \
  --config config/training/genetic.json
```

### Surcharges ponctuelles

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

Options :

| Option | Remplacement |
|---|---|
| `--generations <n>` | Nombre de générations |
| `--population <n>` | Population initiale |
| `--seed <n>` | Graine |
| `--max-halfmoves <n>` | Limite de partie |
| `--quiescence-depth <n>` | Borne de quiescence |
| `--threads <n>` | Concurrence |
| `--fitness-mode <mode>` | Mode de fitness |
| `--corpus <fichier>` | Corpus |
| `--training-positions <fichier>` | FEN de départ |
| `--search-space <fichier>` | Espace de recherche |
| `--run-root <dossier>` | Parent des résultats |
| `--log-dir <dossier>` | Dossier de journal explicite |
| `--output <fichier>` | Modèle de sortie explicite |

Les chemins passés directement par la CLI sont interprétés depuis le
répertoire courant.

### Sans positions d’ouverture

En mode `matches`, les valeurs `none` ou `-` désactivent le fichier de
positions :

```bash
build/release/train_genetic \
  --config config/training/genetic.json \
  --training-positions none
```

Les parties commencent alors depuis la position initiale.

## Mode `matches`

### Première sélection par paires

La population est mélangée puis divisée par paires.

Chaque paire joue deux parties rapides :

1. A avec les Blancs contre B ;
2. B avec les Blancs contre A.

Ces premières parties sont limitées à :

```text
min(max_halfmoves, 30)
```

Les couleurs inversées réduisent l’avantage du premier trait.

### Score d’une paire

Pour chaque individu, le moteur accumule :

- `points` : 1 pour une victoire, 0,5 pour une nulle, 0 pour une défaite ;
- `dominance` : avantage final compressé par une tangente hyperbolique ;
- `conversionSpeed` : bonus pour une victoire rapide ;
- `resilience` : maintien d’un score matériel en défaite ou en nulle.

La qualité secondaire est :

```text
gameQuality =
    0,50 × dominance
  + 0,30 × vitesse
  + 0,20 × résilience
```

La fitness de paire est :

```text
fitness =
    1
  + 1,60 × résultat
  + 0,25 × gameQuality
  + 0,15 × efficacité de profondeur
```

Elle est bornée entre 1 et 3.

Le résultat des parties domine volontairement les signaux secondaires. Un
individu ne doit pas être sélectionné pour une « belle » défaite.

### Évaluation objective du tournoi

Les scores finaux des parties utilisent des valeurs fixes :

- pion : 100 ;
- cavalier : 320 ;
- fou : 330 ;
- tour : 500 ;
- dame : 900.

Un mat vaut ±100 000.

Ces valeurs ne viennent pas des individus. Un candidat ne peut donc pas
augmenter sa dominance en déclarant lui-même qu’une pièce vaut davantage.

### Validation des survivants

La moitié gagnante est d’abord classée par fitness de paire. La meilleure
moitié de ces survivants, avec au minimum deux finalistes si possible, joue :

- un mini-tournoi entre finalistes ;
- des duels contre au plus deux champions antérieurs.

Ces parties sont limitées à :

```text
min(max_halfmoves, 60)
```

Le score de validation remplace la fitness des finalistes sur une échelle
supérieure, puis la liste est triée de nouveau.

### Mémoire de champions

Le meilleur survivant de chaque génération rejoint le début d’une liste
limitée à trois individus. Jusqu’à deux champions servent d’ancrages pendant
les validations futures.

Cette mémoire limite les régressions purement relatives : une génération doit
résister à des adversaires antérieurs, pas seulement battre ses contemporains.

### Reproduction

Tous les survivants sont copiés dans la génération suivante. Il s’agit de
l’élitisme.

Les parents des enfants sont tirés avec un poids dépendant de leur rang :

```text
meilleur survivant : poids N
suivant             : poids N-1
...
dernier              : poids 1
```

Le croisement est uniforme : chaque gène vient indépendamment du parent A ou B
avec une probabilité de 50 %.

La mutation est ensuite appliquée selon les réglages de la période.

### Validation finale

Après la dernière génération, chaque individu affronte jusqu’à quatre voisins
circulaires, avec couleurs inversées et `max_halfmoves` complet.

En cas d’égalité, un gagnant est tiré avec le générateur de l’expérience. La
graine rend ce choix reproductible dans le même environnement.

## Mode `corpus`

### Format

Une ligne non commentée contient :

```text
score_cible<TAB>FEN
```

Le score cible est du point de vue des Blancs.

### Fitness

Pour chaque position :

```text
erreur = |évaluation_agent - score_cible|
```

La moyenne absolue est transformée en fitness :

```text
fitness = 1 + 2 / (1 + erreur_moyenne / 100)
```

La fitness appartient à l’intervalle `(1, 3]` :

- erreur nulle : 3 ;
- erreur croissante : fitness qui se rapproche de 1.

La moitié supérieure survit. Il n’y a ni partie, ni inversion de couleurs, ni
mémoire de champions dans ce mode.

### Lancement

```bash
build/release/train_genetic \
  --config config/training/genetic.json \
  --fitness-mode corpus \
  --corpus data/positions/training_corpus.tsv
```

### Limite du corpus livré

Le corpus d’entraînement fourni contient peu de positions et sert surtout à
valider le pipeline. Il n’est pas assez grand pour apprendre une évaluation
générale robuste.

`holdout_corpus.tsv` doit rester séparé. L’utiliser pendant l’entraînement
annulerait sa valeur de contrôle.

## Positions de départ

`data/openings/generated/training_positions.fen` contient 2 905 positions
uniques extraites des lignes d’ouverture après au moins six demi-coups.

Pour chaque paire, l’indice sélectionné dépend de son indice dans la génération.
Si le calendrier dépasse la taille du corpus, un modulo réutilise les positions.

Les positions :

- diversifient les ouvertures ;
- évitent que tous les agents rejouent uniquement la position initiale ;
- sont identiques pour les deux couleurs d’une paire.

## Sorties

### Répertoire

Sans chemin explicite :

```text
runs/20260729-005656-479_seed42/
├── best_model.json
├── generations.csv
└── run_metadata.json
```

Le nom contient date, heure, millisecondes et graine.

### `best_model.json`

Le meilleur survivant est sauvegardé au cours de l’expérience, puis remplacé
par le gagnant de la validation finale.

Le JSON contient uniquement les champs compris par le moteur.

### `generations.csv`

Chaque ligne contient :

- numéro de génération ;
- identifiant ;
- fitness ;
- victoires, nulles et défaites ;
- profondeur et mode ;
- paramètres LMR applicables ;
- valeurs des pièces ;
- poids positionnels.

Le journal est ouvert en ajout. Son en-tête est écrit uniquement si le fichier
n’existe pas.

Le compteur CSV actuel commence à zéro, tandis que l’affichage utilisateur des
générations commence à un.

### `run_metadata.json`

Les métadonnées contiennent :

- version du schéma ;
- version logique de la recherche ;
- graine ;
- limites des parties et de quiescence ;
- population et nombre de générations ;
- nombre de threads ;
- nombre de positions ;
- mode de fitness ;
- chemins du corpus et de l’espace ;
- inversion des couleurs ;
- système et architecture ;
- compilateur ;
- type de build.

Ce fichier est indispensable pour interpréter un résultat plus tard.

## Suivre une expérience

La sortie standard affiche :

- génération courante ;
- nombre de paires terminées ;
- pourcentage ;
- temps écoulé ;
- estimation du temps restant ;
- meilleure et moyenne des fitness ;
- profondeur moyenne ;
- valeurs moyennes ;
- paramètres du meilleur individu.

La durée estimée est indicative, surtout au début d’une génération ou lorsque
les positions ont des coûts très différents.

## Reproductibilité

### Éléments contrôlés

- graine enregistrée ;
- positions fixes ;
- couleurs inversées ;
- espace de recherche explicite ;
- calendrier explicite ;
- métadonnées du système et du compilateur ;
- modèle sauvegardé séparément.

### Éléments non entièrement figés

- ordonnanceur des threads ;
- bibliothèque standard ;
- version exacte du compilateur si l’environnement change ;
- performances thermiques et charge de la machine ;
- contenu externe des données si les sources sont remplacées.

Pour une comparaison stricte :

1. conserver le code ;
2. conserver les configurations ;
3. conserver les données ;
4. conserver la graine ;
5. utiliser le même nombre de threads ;
6. utiliser le même compilateur et le même preset.

Un verrou `conda-lock` par plateforme reste optionnel. Il ne figerait pas à lui
seul le compilateur C++ du système.

## Comparaison d’un candidat

Commande recommandée :

```bash
scripts/compare_model.sh runs/<experience>/best_model.json
```

Elle compile `compare_models` puis :

1. compare la fitness du candidat et de la référence sur
   `holdout_corpus.tsv` ;
2. joue une partie candidat blanc ;
3. joue une partie candidat noir ;
4. affiche résultats et longueurs.

Pour une décision importante, deux parties restent insuffisantes. Il faut
répéter avec plusieurs positions de départ et examiner les cas tactiques.

## Promotion du modèle

La promotion est volontaire :

```bash
scripts/promote_model.sh runs/<experience>/best_model.json
```

Elle copie le candidat vers :

```text
data/models/current.json
```

Avant promotion :

- vérifier que le fichier se charge ;
- exécuter les tests ;
- comparer au modèle actif ;
- contrôler le corpus holdout ;
- jouer avec couleurs inversées ;
- conserver les métadonnées ;
- calculer une empreinte SHA-256 ;
- mettre à jour `data/models/README.md`.

Une expérience ne doit jamais écrire directement dans le modèle actif.

## Modèle livré

Le modèle actuel provient de :

```text
runs/20260729-005656-479_seed42/best_model.json
```

Il a été entraîné avec :

- graine 42 ;
- 100 générations ;
- profondeur normale 4 ;
- quiescence maximale 10 ;
- 2 905 positions ;
- mode `instinct`.

La GUI surcharge actuellement le mode par `instinct_lmr` au démarrage, tout en
conservant les poids appris. Cette surcharge ne modifie pas
`data/models/current.json`.

Voir [`data/models/README.md`](../data/models/README.md) pour l’empreinte et la
provenance.

## Limites méthodologiques

### Coût

Une partie contient plusieurs recherches. Une génération de 32 individus avec
validations peut représenter un calcul important, même à profondeur 4.

### Bruit de sélection

Deux agents proches peuvent obtenir des résultats différents selon :

- ouverture ;
- couleur ;
- longueur maximale ;
- tactique située après la limite ;
- ordre des adversaires.

Les couleurs inversées et les validations réduisent ce bruit sans le supprimer.

### Surapprentissage

Un modèle peut devenir très bon sur les positions de départ ou scores connus et
moins bon ailleurs. Les données de contrôle et les parties externes sont donc
obligatoires.

### Profondeur comme gène

Une profondeur élevée augmente souvent la force, mais aussi le coût. Si elle
varie librement, la sélection peut surtout apprendre à consommer davantage de
ressources. La configuration actuelle la fixe pour comparer les poids à budget
semblable.

### Pas de reprise automatique

Le programme ne recharge pas actuellement une population depuis un journal
interrompu. `best_model.json` préserve le meilleur résultat déjà écrit, mais ne
constitue pas un point de reprise complet de la population et du générateur
aléatoire.

## Protocole expérimental recommandé

1. définir une hypothèse précise ;
2. figer la recherche ;
3. figer l’espace de recherche ;
4. choisir les données ;
5. faire un petit test de bout en bout ;
6. lancer l’expérience principale ;
7. conserver le dossier complet ;
8. comparer plusieurs finalistes ;
9. valider sur des données séparées ;
10. jouer avec couleurs inversées ;
11. promouvoir explicitement ;
12. documenter le modèle retenu.

Modifier la recherche, les bornes ou les données au milieu d’une expérience
rend ses générations difficilement comparables.
