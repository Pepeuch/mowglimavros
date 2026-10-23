# Checkpoint — GNSS MAVROS, proposition prête pour revue

23 octobre 2026. Dépôt propriétaire autonome : `/home/ubuntu/mowglimavros-gnss-contract-20261023`, branche `review/gnss-interface-20261023`, HEAD `4673f43cb6b2b6b9df0685d36f8f8bb43e6e1456`. Aucun commit/push, index vide. Treize fichiers suivis modifiés, un nouveau checkpoint partagé, rapports/preuves dans ce dossier ; `files.json` distingue le patch des artefacts de validation.

- Image ancienne c57fd393 : provenance source4673, interfaces e789ccc2, hash GNSS6765a75d confirmés par OCI et fichiers installés des deux architectures.
- Image Mowgli 7489a4a6 : source59051b41 identifiée par config OCI ; contrat actuel ca48e4f1 corroboré par génération et réception DDS MowgliNext.
- Patch : IDL canonique9686, identités opaques copiées, pas de nouvelle vérité GNSS ; gates canonique/peer/install et gtests avant publication proposés.
- RED : dérive de copie/hash et trois cas d'identité. GREEN : six paquets compilés, 17 gtests GNSS, 224 cas JUnit actuels, GPS1/GPS2/direct et abonné DDS MowgliNext ; Python8+10, Go8, frontend78 ; syntaxe, format et diff PASS. Premiers essais et dashboards échoués conservés, aucune suite globale initiale présentée comme verte.
- Identité candidate dans future-image.json et source-sha256.json. Image non construite/non publiée : digest OCI et SHA de futur commit encore inconnus, non inventés. ARM64 et build Docker complet non validés ici.
- Préservation : root `fix/gui-warnings-errors@9686b37556f1d2ba7f7a295eb006f870c86e8095`, index vide, aucune modification suivie ; 136 historiques, 1 827 preuves et trois sous-modules intacts. Les rapports spécifiques restent dans MowgliMAVROS, conformément à AGENTS.

Lire analysis.md, proposal.md, review.md, red-green.md, final.diff et les manifestes. HOLD pour revue : aucun commit, push, modification de gitlink, remplacement d'image, SSH, flash, commande moteur ou CI distante. L'intégration du gate apparié dans MowgliNext reste une proposition nécessitant une autorisation distincte. HARDWARE_REQUIRED et réserve Fleet maintenus ; procédure passive et critères dans proposal.md.
