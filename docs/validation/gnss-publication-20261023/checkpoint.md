# Publication de revue GNSS MAVROS — autorisée, avant push

2026-10-23. Dépôt autonome Pepeuch/mowglimavros ; branche review/gnss-interface-20261023, cible main.

Code validé exclusivement dans 7a2a1a0963886dd19f151681ccc4eae445600fc8 et 61d3e37b4bc2f7081b317e955877bafcf2f0b5db ; base 4673f43cb6b2b6b9df0685d36f8f8bb43e6e1456. Le commit suivant ne finalise que les métadonnées et preuves de revue. Aucun changement de production supplémentaire.

Les rapports interface/Docker sont des captures historiques immuables : leur état « local, non poussé » correspond à leur date d'acquisition, pas au futur état de la branche. review-artifacts.json identifie le sous-ensemble publié et les pièces intégrales conservées localement ; les manifestes historiques ne prétendent pas que toutes leurs entrées sont distribuées dans Git. Aucun bytecode, SDK, arbre de build ou capture d'environnement n'est ajouté.

Validation acceptée : 224 cas C++/ROS par architecture, dont 17 GNSS ; huit RIHS conformes ; ARM64 via QEMU, AMD64 natif ; recettes locales non-root explicites. Contrat GNSS RIHS01_ca48e4f1007c03b476e559280a5d9a5b1b03dd4061b2f8956aea93ad22a17fdf. Les IDs Docker locaux restent distincts des digests GHCR inconnus.

MowgliNext PR #1017 / release v1.6 BLOQUÉE : publication d'images compatibles, choix de digests immuables de la paire, intégration/gates approuvés séparément, puis validation passive sur le robot. HARDWARE_REQUIRED reste ouvert. Voir ../gnss-docker-20261023/release-plan.md pour baseline, procédure, critères et prérequis. Réserve Fleet inchangée.

Autorisation courante : push de la branche et ouverture d'une PR main, puis STOP. Aucune interrogation, attente, relance, collecte de logs ou déclenchement CI. Aucun merge, déploiement, SSH, flash, action moteur ou changement de gitlink MowgliNext. Le résultat effectif de publication sera consigné dans le checkpoint local ignoré ; ce fichier décrit l'état vérifié avant publication, sans inventer une URL ou un SHA de commit documentaire.
