# Modèle actif

`current.json` est le résultat retenu de l'entraînement terminé le 29 juillet
2026. Il a été promu explicitement depuis :

```text
runs/20260729-005656-479_seed42/best_model.json
```

Les deux fichiers étaient strictement identiques lors de la promotion
(SHA-256 :
`df5ba8c4584b51443ea4d9c60957cfe64e8cebff518a758f94edded4d6d78405`).

L'expérience utilisait :

- la graine `42` ;
- `100` générations ;
- une profondeur normale de `4` ;
- une profondeur maximale de quiescence de `10` ;
- `2 905` positions d'entraînement ;
- le mode de recherche `instinct`.

Le JSON reste volontairement limité aux paramètres compris par le moteur. Les
informations de provenance sont conservées dans ce fichier et dans le
`run_metadata.json` original.

La GUI peut appliquer `instinct_lmr` au chargement sans modifier les poids
appris ni le contenu de `current.json`.
