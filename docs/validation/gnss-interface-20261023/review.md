# Revue — synchronisation GNSS MAVROS proposée

23 octobre 2026. Correction logicielle et proposition de publication prêtes pour revue ; **aucun commit, push, build OCI candidat ou déploiement**.

L'image MAVROS fournie est reliée par ses attestations à `Pepeuch/mowglimavros@4673f43c`, build 39382305156 du 20 octobre matin. Son sous-ensemble `mowgli_interfaces` vient de `e789ccc2` ; son hash GNSS 6765a75d est confirmé dans les paquets installés amd64/ARM64. Les champs ont été ajoutés à MowgliNext le même soir, au commit 1e1af899. Le contrôle antérieur vérifiait le verrou contre lui-même ; reconstruire sans changer ce contrat aurait conservé le problème.

Le patch aligne exactement les huit définitions requises sur MowgliNext `9686b375`, hash GNSS ca48e4f1 ; sept définitions restent byte-identiques. Il copie source_id/incarnation sans les générer, conserve séquence et règles de fraîcheur/RTK, installe le verrou et ajoute les contrôles source canonique, consommateur et types ROS installés. Le workflow proposé exécute les contrôles et le build Docker exécute les gtests avant publication, avec labels et tag à SHA complet.

## Périmètre exact

Treize fichiers suivis modifiés et un nouveau checkpoint partagé, tous dans le dépôt MAVROS autonome. Liste et SHA256 dans `files.json` / `source-sha256.json` :

- `.agent/shared/checkpoints/INDEX.md` et nouveau `active/MM-GNSS-INTERFACE-20261023.md` : mémoire de reprise imposée par AGENTS.
- `TODO.md`, `ros2/src/mavros_gnss_adapter/README.md` : état actuel du contrat et acceptation image encore ouverte, historique conservé.
- `.github/workflows/docker.yml`, `ros2/Dockerfile` : gates et identité du build, sans changement des pins MAVROS/UG.
- `ros2/src/mowgli_interfaces/CMakeLists.txt`, `interface-contract.lock.json`, `msg/GnssStatus.msg` : contrat aligné et traçabilité installée.
- `ros2/src/mavros_gnss_adapter/include/mavros_gnss_adapter/status_mapping.hpp` : deux affectations seulement en production.
- `ros2/src/mavros_gnss_adapter/test/test_gnss_adapter.cpp`, `test/test_gnss_graph.py` : identité, cache, expiration et remplacement.
- `tools/mowgli_interface_contract.py`, `tools/test_mowgli_interface_contract.py` : gate renforcé et régressions.

`final.diff` contient ces quatorze fichiers ; il exclut uniquement les rapports/preuves de ce dossier pour éviter une référence circulaire. Ceux-ci sont conservés dans leur dépôt propriétaire et inventoriés séparément. Aucun ancien checkpoint supprimé ou réécrit. Aucun changement de MowgliNext ni de gitlink, GUI, backend concurrent, firmware, gain ou seuil. Le plugin, l'état de fraîcheur et le code d'altitude sont identiques à la baseline ; les assertions C++ historiques aussi (`invariants.json`).

## Validation réelle

RED démontré : ancien check source-root faussement vert, ancien contrat/hash refusés, trois tests sémantiques en échec avant les copies d'identité. GREEN : six paquets amd64 compilés ; 17 gtests adaptateur ; 224 cas JUnit actuels dans cinq paquets ; graphes réels GPS1/GPS2/direct, ESC et safety ; 8 tests de contrat Python, 10 contrats backend ; abonné MowgliNext séparé recevant 20 statuts DDS avec hash attendu et identités intactes ; 8 tests Go et 78 tests télémétrie GUI existants. Format des lignes C++ avec clang-format 18, Python/YAML/shell et diff check réussis.

Les premiers échecs de dépendances, chemin géoïde, cache Go et dashboard CTest historique restent dans les preuves. La commande globale initiale n'est **pas** déclarée verte ; les résultats finaux des tests concernés sont distingués. Voir `red-green.md` pour les sorties et limites exactes.

## Réserves et prochain GO

Le problème de contrat et l'omission du mapper ont une correction locale démontrée. Aucune future image OCI n'a été construite : identité source complète et hash attendu dans `future-image.json`, digest et commit candidat null jusqu'au GO build/publication. Le build complet Docker, le candidat ARM64 et les workflows distants n'ont pas été exécutés. Le test DDS local Fast DDS ne certifie ni la configuration DDS physique du robot ni son navigateur.

Un pin sidecar seul ne prévient pas une nouvelle évolution indépendante de MowgliNext : gate apparié avant release/déploiement et association par digests nécessaires. Cette intégration MowgliNext est proposée dans `proposal.md`, pas appliquée ni prétendue active. Aucun remplacement de l'image actuellement sur le robot avant validation de cette paire.

HARDWARE_REQUIRED conservé, procédure passive, identité de baseline et critères dans `proposal.md`. Aucun statut RTK inventé ou seuil abaissé pour afficher FLOAT/FIX. Réserve Fleet indépendante inchangée. Arrêt pour revue, sans déploiement ni opération matérielle.

## Préservation

`evidence/verification.json` contrôle SHA256 : 3 055 fichiers suivis MowgliNext, 136 historiques, 1 827 preuves initiales et 358 fichiers suivis du sous-module MAVROS inchangés ; trois sous-modules propres, gitlinks identiques. MowgliNext reste `fix/gui-warnings-errors@9686b37556f1d2ba7f7a295eb006f870c86e8095`, index vide. La copie autonome de revue a également un index vide. Aucun accès SSH/robot, flash, commande moteur, CI déclenchée ou surveillance distante.
