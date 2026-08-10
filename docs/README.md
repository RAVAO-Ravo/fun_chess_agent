# Documentation du projet

Ce dossier contient les références techniques qui complètent le
[`README principal`](../README.md). Le README explique l’ensemble du projet ;
les documents ci-dessous approfondissent chacun un domaine et constituent la
source à consulter lorsqu’un détail d’implémentation, de mesure ou de
configuration est nécessaire.

## Parcours conseillé

### Découvrir le projet

1. lire le [résumé et l’installation](../README.md) ;
2. consulter le [glossaire](glossary.md) pour les termes inconnus ;
3. lire l’[architecture](architecture.md) pour comprendre les composants ;
4. lancer la GUI ou le moteur selon les exemples du README.

### Modifier la recherche

1. lire les invariants de [`architecture.md`](architecture.md) ;
2. établir une référence avec [`performance.md`](performance.md) ;
3. modifier un seul mécanisme identifiable ;
4. exécuter les tests Debug et Sanitize ;
5. comparer les nœuds, le coup et le temps sur les mêmes positions.

### Lancer un entraînement

1. lire [`training.md`](training.md) ;
2. figer `config/training/search_space.json` ;
3. choisir le coût dans `config/training/genetic.json` ;
4. conserver la graine et les positions pendant toute l’expérience ;
5. valider le résultat avant toute promotion du modèle actif.

### Créer un autre client

1. compiler `chess_engine` ;
2. lire le contrat dans [`protocol.md`](protocol.md) ;
3. lancer le moteur avec `--interactive` ;
4. échanger une commande complète à la fois ;
5. traiter explicitement les lignes `error`, `status` et la fin du processus.

## Index

| Document | Public visé | Question principale |
|---|---|---|
| [`architecture.md`](architecture.md) | Développeur | Où se trouve chaque responsabilité ? |
| [`performance.md`](performance.md) | Développeur moteur | Comment mesurer une optimisation sans se tromper ? |
| [`protocol.md`](protocol.md) | Développeur de client | Quelles commandes et réponses sont garanties ? |
| [`training.md`](training.md) | Expérimentateur | Comment configurer, comparer et promouvoir un modèle ? |
| [`glossary.md`](glossary.md) | Débutant | Que signifie un terme technique du projet ? |

## Sources de vérité

La documentation décrit le comportement actuel, mais certains fichiers font
autorité lorsqu’une valeur précise est recherchée :

| Information | Source de vérité |
|---|---|
| Cibles et dépendances de compilation | `CMakeLists.txt` |
| Presets disponibles | `CMakePresets.json` |
| Dépendances reproductibles | `environment.yml` |
| Modèle utilisé par défaut | `data/models/current.json` |
| Provenance du modèle actif | `data/models/README.md` |
| Bornes entraînables | `config/training/search_space.json` |
| Calendrier d’entraînement | `config/training/genetic.json` |
| Contrat du protocole | `src/protocol/command_processor.cpp` et `protocol.md` |
| Résultat de benchmark courant | `benchmarks/results/latest.csv` |
| Licence principale | `LICENCE` |

## Maintenance documentaire

Lorsqu’un comportement change :

- mettre à jour le document spécialisé concerné ;
- corriger le résumé correspondant dans le README ;
- vérifier les exemples de commandes ;
- ne pas conserver deux descriptions contradictoires ;
- supprimer les documents historiques qui ne décrivent plus le projet actuel ;
- contrôler les liens Markdown avant validation Git.
