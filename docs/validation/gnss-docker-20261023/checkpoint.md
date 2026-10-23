# Checkpoint — validation Docker GNSS livrée pour revue

MowgliMAVROS autonome /home/ubuntu/mowglimavros-gnss-contract-20261023, review/gnss-interface-20261023@61d3e37b4bc2f7081b317e955877bafcf2f0b5db. Commits locaux7a2a1a0 puis61d3e37, aucun push. Quatorze chemins au total ; complément limité au chemin Docker et son test. Voir final.diff et docker-path-complement.diff.

Résultats exacts et identités : review.md, images.json, build-results.json, test-summary.json, interface-comparison.json. Recettes non-root et QEMU explicités. Les tests et échecs antérieurs sont conservés, pas effacés pour présenter un faux historique vert.

PR1017 / v1.6 BLOQUÉE : paire de release pas publiée/intégrée, gate MowgliNext seulement préparé, matériel non validé. Aucun digest GHCR futur inventé. MowgliNext9686b375,136 historiques,1827 preuves et trois sous-modules inchangés. Préservation dans evidence/verification.json et ancien manifeste de livraison contrôlé.

HARDWARE_REQUIRED et réserve Fleet maintenus. Aucun SSH, robot, flash, moteur, tonte, déploiement, push ou CI distante. Lire release-plan.md pour l'ordre des GO explicites. STOP revue ; ne pas publier, déployer ou fusionner #1017 sur la seule base de ces tests locaux.

Les builds AMD64 et ARM64 et les trois validations par architecture sont PASS.224 cas JUnit chacun,0échec/erreur/skip,17GNSS chacun ;175tests GUI existants+6sur exports Docker PASS ; gateproposé8cas PASS, descriptor pending refuse correctement. Test interimagesAMD6422statutsPASS. Indexvide ; seules métadonnées TODO/index suivies modifiées et rapports/checkpoint nouveaux noncommittés pour revue. Code de production identique au commit61d3e37.
