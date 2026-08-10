# Protocole textuel du moteur

## Portée

`chess_engine` expose un protocole propre au projet, orienté ligne, sur l’entrée
et la sortie standard.

Il utilise la notation UCI pour représenter les coups, mais il ne met pas en
œuvre le protocole UCI complet. Un client UCI tiers ne peut donc pas piloter
directement le moteur sans adaptateur.

Le [glossaire](glossary.md) définit FEN, UCI, demi-coup et les états de partie.

## Démarrage

Commande minimale :

```bash
build/release/chess_engine --interactive
```

Commande complète typique :

```bash
build/release/chess_engine \
  --interactive \
  --params data/models/current.json \
  --depth 4 \
  --search-mode instinct_lmr \
  --book data/openings/generated/book.txt \
  --book-mode chill \
  --time-ms 1000 \
  --diagnostics
```

Options :

| Option | Signification |
|---|---|
| `--interactive` | Obligatoire ; active la boucle de commandes |
| `--params <fichier>` | Modèle JSON chargé |
| `--depth <n>` | Remplace la profondeur du modèle |
| `--search-mode <mode>` | `classic`, `instinct` ou `instinct_lmr` |
| `--book <fichier>` | Bibliothèque d’ouvertures |
| `--book-mode <mode>` | `chill` ou `competition` |
| `--no-book` | Désactive la bibliothèque |
| `--time-ms <n>` | Limite d’un calcul IA ; zéro signifie aucune limite |
| `--diagnostics` | Active les statistiques après les coups IA |

Le moteur écrit les erreurs de démarrage sur la sortie d’erreur et retourne un
code non nul.

## Principes d’échange

- le client écrit une commande suivie de `\n` ;
- le moteur traite une seule commande à la fois ;
- la commande produit zéro, une ou plusieurs lignes ;
- toutes les réponses sont terminées par `\n` ;
- les champs d’une ligne sont séparés par des espaces ;
- les FEN restent sur une seule ligne ;
- une ligne d’entrée vide est ignorée sans réponse ;
- `quit` produit `ok` puis termine la boucle.

Un client synchrone doit lire le nombre de lignes prévu pour la commande. Pour
les commandes de position, la ligne `status` marque la fin de la réponse
agrégée.

## Notation des coups

Les coups sont représentés par :

```text
<origine><destination>[promotion]
```

Exemples :

```text
e2e4
g1f3
e1g1
e7e8q
```

Le suffixe de promotion vaut :

- `q` : dame ;
- `r` : tour ;
- `b` : fou ;
- `n` : cavalier.

Une notation syntaxiquement invalide produit généralement `error <message>`.
Une notation valide mais illégale dans la position produit `illegal_move`.

## Commandes

### `new_game`

```text
new_game white
new_game black
```

Réinitialise la position et définit le camp humain.

Si l’humain choisit les Blancs :

```text
board_fen <FEN initiale>
status playing
```

Si l’humain choisit les Noirs, le moteur joue immédiatement le premier coup :

```text
ai_move <coup>
[search_stats ...]
board_fen <FEN après le coup>
status playing
```

La couleur absente est remplacée par `white`.

### `human_move`

```text
human_move e2e4
```

Joue un coup uniquement si :

- la partie n’est pas terminée ;
- c’est le trait du camp humain ;
- le coup est légal.

Succès :

```text
ok
board_fen <FEN>
status <état>
```

Échec positionnel :

```text
illegal_move
```

Cette commande ne demande pas automatiquement la réponse de l’IA. Le client
doit envoyer ensuite `ai_move`.

### `free_move`

```text
free_move e2e4
```

Joue un coup légal sans vérifier que le camp au trait correspond au camp
humain. Cette commande est utilisée par le mode « Deux humains ».

Succès :

```text
ok
board_fen <FEN>
status <état>
```

Échec :

```text
illegal_move
```

### `ai_move`

```text
ai_move
```

Si la partie continue, demande à l’agent de choisir puis d’appliquer un coup.

Réponse habituelle :

```text
ai_move e7e5
[search_stats ...]
board_fen <FEN>
status <état>
```

Si la partie est déjà terminée, aucun coup n’est joué :

```text
board_fen <FEN>
status <état terminal>
```

Un coup issu de la bibliothèque ne visite aucun nœud. Les statistiques du
dernier calcul sont alors remises à zéro.

### `undo_turn`

```text
undo_turn
```

Annule des demi-coups jusqu’à rendre le trait au joueur humain. En situation
normale humain/IA, cela retire le coup de l’IA puis celui de l’humain.

Succès :

```text
ok
board_fen <FEN>
status <état>
```

Sans historique suffisant :

```text
cannot_undo
```

### `undo_move`

```text
undo_move
```

Annule exactement un demi-coup.

Succès :

```text
ok
board_fen <FEN>
status <état>
```

Sans historique :

```text
cannot_undo
```

### `get_fen`

```text
get_fen
```

Réponse :

```text
fen <FEN>
```

Cette commande retourne uniquement la FEN, sans ligne `status`.

### `get_legal_moves`

```text
get_legal_moves
```

Réponse :

```text
legal_moves e2e4 e2e3 g1f3 ...
```

S’il n’existe aucun coup :

```text
legal_moves
```

### `status`

```text
status
```

Retourne une seule des formes décrites dans la section
[États de partie](#états-de-partie).

### `diagnostics on`

Active la ligne `search_stats` après chaque coup IA calculé.

```text
diagnostics on
```

Réponse :

```text
ok
```

### `diagnostics off`

Désactive l’émission automatique :

```text
diagnostics off
```

Réponse :

```text
ok
```

### `diagnostics`

Sans argument, retourne immédiatement les dernières statistiques :

```text
diagnostics
```

Réponse :

```text
search_stats depth 4 nodes 123 qnodes 45 time_us 1000 nps 168000 tt_probes 80 tt_hits 20 tt_entries 75 cutoffs 30
```

Les valeurs exactes dépendent de la recherche précédente.

### `quit`

```text
quit
```

Réponse :

```text
ok
```

Le processus termine ensuite normalement.

## États de partie

### Partie en cours

```text
status playing
```

### Échec

```text
status check white
status check black
```

La couleur indique le camp en échec.

### Mat

```text
status checkmate white
status checkmate black
```

La couleur indique le camp vainqueur, et non le camp au trait.

### Pat

```text
status stalemate
```

### Partie nulle

```text
status draw
```

Cette forme couvre :

- règle des cinquante coups ;
- matériel insuffisant ;
- répétition triple.

## Statistiques de recherche

Forme générale :

```text
search_stats \
  depth <n> \
  nodes <n> \
  qnodes <n> \
  time_us <n> \
  nps <n> \
  tt_probes <n> \
  tt_hits <n> \
  tt_entries <n> \
  cutoffs <n>
```

| Champ | Signification |
|---|---|
| `depth` | Dernière profondeur entièrement terminée |
| `nodes` | Nœuds de la recherche principale |
| `qnodes` | Nœuds de quiescence |
| `time_us` | Temps en microsecondes |
| `nps` | Nœuds totaux par seconde |
| `tt_probes` | Consultations de la table |
| `tt_hits` | Entrées trouvées |
| `tt_entries` | Emplacements occupés |
| `cutoffs` | Coupures bêta |

## Erreurs

### Commande inconnue

```text
error unknown command: <commande>
```

### Erreur de validation

```text
error <message>
```

Le moteur capture les exceptions de traitement et reste généralement
disponible pour la commande suivante.

Une fin de fichier sur l’entrée standard termine la boucle sans ligne `ok`
supplémentaire.

## Séquences complètes

### Humain avec les Blancs

Entrée :

```text
new_game white
human_move e2e4
ai_move
quit
```

Sortie possible :

```text
board_fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
status playing
ok
board_fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1
status playing
ai_move e7e5
board_fen rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2
status playing
ok
```

### Deux humains

Entrée :

```text
new_game white
free_move e2e4
free_move e7e5
undo_move
get_fen
quit
```

Le client utilise `free_move` et `undo_move`, car le concept de camp humain ne
doit pas bloquer le second joueur.

### Partie avec diagnostics

Entrée :

```text
diagnostics on
new_game black
quit
```

Le moteur peut répondre à `new_game black` avec `ai_move`, puis
`search_stats`, `board_fen` et `status`.

## Conseils d’implémentation d’un client

- vider le tampon d’écriture après chaque commande ;
- ne jamais envoyer deux commandes en supposant que leurs réponses se
  mélangeront correctement ;
- considérer `status` comme fin d’une réponse de position ;
- accepter la ligne optionnelle `search_stats` ;
- traiter `error`, `illegal_move` et `cannot_undo` séparément ;
- détecter une sortie standard fermée ;
- fermer le processus avec `quit`, puis utiliser une terminaison forcée
  uniquement s’il ne répond pas ;
- ne pas reconstruire la légalité côté client.

## Compatibilité

Les anciens alias suivants ont été retirés :

- `uci_move` ;
- `get_board` ;
- `go` ;
- `undo`.

Ils retournent maintenant :

```text
error unknown command: <alias>
```

Cette suppression évite de maintenir plusieurs contrats concurrents. Toute
évolution future doit mettre à jour simultanément :

- `src/protocol/command_processor.cpp` ;
- les tests d’intégration ;
- ce document ;
- les clients concernés.
