# FCU Session Controller — bilan logiciel du 11 octobre 2026

Dépôt propriétaire : MowgliMAVROS. Branche `feat/fcu-session-controller`,
base `e7d516cb0adca1872dd9d10fb66412857d38b0c8`. Aucun commit/push/déploiement.
MowgliNext reste l'unique autorité fonctionnelle ; son code et son gitlink sont
inchangés. Aucun changement obligatoire du BT/GUI ni d'interface ROS publique.

## Analyse et architecture retenues

- [Architecture, ordre des commandes et validations Pixhawk restantes](../../architecture/fcu-session-controller.md).
- [Checkpoint durable](../../../.agent/shared/checkpoints/retained/MM-FCU-ADAPTER-20261011.md).
- [Diff de revue archivé](review.diff) et [liste des fichiers de ce snapshot](modified-files.txt).

Le diff est le snapshot logiciel fourni avant ce regroupement documentaire.
Il ne contient pas ce dossier d'archives ni les mises à jour des liens de preuves.
Le manifeste SHA256 délimite exactement les sources et tests validés ; déplacer
les preuves et mettre à jour la documentation ne modifie pas ce périmètre.

## Résultats

| Vérification | Résultat |
| --- | --- |
| Compilation ROS2 dans le SDK isolé | 6/6 paquets |
| Bridge, dont 24 cas du séquenceur | 17/17 suites, 104 cas XML |
| ESC / odométrie | 5/5 suites, 84 cas XML |
| Batterie | 1/1 suite, 8 cas XML |
| GNSS | 1/1 suite, 14 cas XML |
| Contrats publics / backend | 3 + 10 contrôles PASS |
| Contrat de délégation de lame / OFF | 2 contrôles PASS |
| Binaire installé : graphe ArduPilot mocké | PASS |
| Formatage clang-format18, syntaxe, diffcheck, tableau de progression | PASS |

Total des rapports XML : 24 fichiers, 210 cas comptabilisés, zéro échec,
erreur ou cas sauté. [Résumé JSON](results-summary.json) ; rapports dans
`bridge-results/`, `esc-results/`, `battery-results/`, `gnss-results/`.
Tous les builds/tests/installations ont utilisé UID1000, sans réseau ni devices,
avec domaines ROS isolés et services FCU simulés.

## Provenance des preuves

- [Manifeste des sources validées](source-manifest.sha256), SHA256
  `dbf918a0cf9990b781beb02e30bed88833c571eae0535cc4ada2a8639238ec81`.
- [Empreintes des binaires de build et installés](binary-proof.txt).
- [RPATH](rpath-proof.txt) et [dépendances dynamiques résolues](ldd-proof.txt).
- [Journal complet de la session de validation](validation.log).

Le journal conserve aussi les erreurs de la première architecture abandonnée
et une commande auxiliaire `rg` absente du SDK, corrigée ensuite. Elles ne sont
pas des échecs du code final : les XML finaux et les derniers passages des
graphes attestent la refonte simplifiée. Les chemins absolus présents dans les
preuves décrivent l'environnement SDK de cette exécution, pas une installation
sur le robot. Aucun binaire ni image de production n'est archivé ici.

## Limites physiques

`HARDWARE_REQUIRED` : sorties servo réelles en HOLD, neutralité et directions,
unités/signes GUIDED, courses MODE/ARM/DISARM et arrêt indépendant après perte
totale du companion. La procédure, le baseline à relever et les prérequis de
sécurité figurent dans le document d'architecture. Aucune aptitude à la
production n'est déclarée et aucun essai matériel n'a été exécuté.
