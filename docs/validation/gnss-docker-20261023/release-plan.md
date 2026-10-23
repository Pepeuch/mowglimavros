# Paire de release v1.6 — dépendance bloquante de PR #1017

**PR #1017 reste NON PRÊTE À FUSIONNER.** Le correctif MAVROS n'est pas un chantier à reporter après la release. La compatibilité d'interface, la publication des images appariées, l'intégration MowgliNext et HARDWARE_REQUIRED sont des critères distincts et tous doivent être traités avant l'acceptation finale demandée.

## État des dépôts

- MowgliMAVROS autonome : branche review/gnss-interface-20261023 ; commits locaux `7a2a1a0963886dd19f151681ccc4eae445600fc8` (14 chemins approuvés) puis `61d3e37b4bc2f7081b317e955877bafcf2f0b5db` (deux lignes : chemin Docker et sa régression). Aucun amend/rebase/push. Le second commit est nécessaire : le vrai Docker build a révélé le défaut de chemin non visible dans le workspace natif.
- MowgliNext : fix/gui-warnings-errors@9686b37556f1d2ba7f7a295eb006f870c86e8095, inchangé. Gitlink MAVROS encore4673f43 ; aucun changement de branche, pin ou gitlink appliqué.
- Image MowgliNext fournie : index OCI7489a4a6..., source59051b41. Les huit contrats installés AMD64/ARM64 ont cette fois été extraits directement de ses couches par digest. Ils sont compatibles avec le contrat ca48e4f1 attendu, mais cette ancienne image ne devient pas automatiquement le candidat final v1.6.

## Proposition d'intégration préparée, sans application à MowgliNext

`mowglinext-proposal/` contient un gate Python hors ligne, un wrapper d'extraction d'images sans exécuter leurs entrypoints, une régression locale sur les artefacts réels et le patch du workflow de source. Le gate refuse tags mutables, digest absent, contrat incomplet, fichier manquant, ancienne génération et nouveau contrat canonique sans mise à jour sidecar. Les refs synthétiques dans les seuls tests unitaires ne représentent aucune image publiée.

Le workflow proposé déclenche aussi la validation sur les interfaces et le gitlink MAVROS, puis compare le contrat réellement épinglé au source canonique. L'ancien workflow n'observait pas ces chemins. Le gate d'images doit en plus être appelé sur les **deux images issues du build de release**, pour chaque architecture, avant de publier leur descriptor de release comme accepté. L'appel exige le SHA source MowgliNext attendu fourni par le build, pas un SHA auto-référencé dans un fichier versionné.

Le fichier `release-pair.pending.json` reste intentionnellement incomplet et bloquant tant qu'aucune image n'a été publiée. Il n'existe pas de faux digest de remplacement ou de mode passant silencieusement sur latest.

## Suite après la présente revue

1. Examiner les deux commits locaux, le complément RED/GREEN Docker, les recettes non-root et les résultats des deux architectures. Aucun push/publication dans la présente autorisation.
2. Après GO explicite, publier les commits MAVROS sur la branche revue par fast-forward, puis construire/publier les images qualifiées par le SHA final approuvé. L'ARM64 compatible est une dépendance de v1.6 ; une validation d'émulation ne remplace pas la réception sur Rock5B.
3. Ne publier ni latest ni un alias production par inadvertance : le workflow actuel publie latest sur main. Choisir explicitement le mode/chemin de publication approuvé. Conserver SBOM/provenance et digest de chaque manifest d'architecture, puis l'index multiarchitecture. Les IDs locaux Docker ne sont pas des digests GHCR.
4. Préparer puis faire revoir dans MowgliNext le pin **par digest réel** dans `sensors/mavros/image.env` (source autoritaire utilisée par install/lib/config.sh), la documentation et les exemples cohérents. `sensors/mavros/test_integration.py` contient aujourd'hui une attente explicite sur latest : la remplacer par une assertion de pin immuable, sans retirer les autres assertions.
5. Si le sourcegate utilise le sous-module, avancer explicitement le gitlink MAVROS vers le commit publié et `.github/submodule-pins.txt` dans le même changement autorisé. Aucun de ces pins n'a été touché ici.
6. Intégrer les gates préparés dans la CI MowgliNext : comparaison des sources avant build ; extraction par digest des deux candidats, hashes installés et smoke DDS après build ; échec bloquant sans fallback. Rendre ces contrôles requis avant merge de #1017 et avant publication du bundle accepté.
7. Produire un descriptor de paire après construction (éviter l'auto-référence source/image), avec les deux commits, deux index immuables et manifests d'architecture, source-interface/fingerprint/8hashes, pins MAVROS/UG/MAVLink et résultats. `install/deployment.json` n'inclut actuellement pas de composant MAVROS externe : vérifier explicitement son intégration au bundle/updater de v1.6, au lieu de supposer que l'alias d'installation est suivi par la release. Ce point est une action d'intégration non appliquée, pas une refonte updater dans cette mission.
8. Valider les statuts des images exactes vers le GUI sans changer ses seuils/masques : identités opaques, séquence nouvelle, ancien snapshot, no-fix, FLOAT et FIX seulement quand déclarés et disponibles. Les tests logiciels livrés couvrent la classification ; le navigateur du robot reste à valider.
9. Après GO matériel distinct, exécuter la procédure **passive** HARDWARE_REQUIRED du rapport initial, augmentée des deux digests finaux, unité Rock5B, firmware Pixhawk/récepteur, topologie et configuration exactes. Aucun flash, commande moteur ou tonte. Vérifier hash endpoint, réception `/gps/status`, identité/incarnation/séquence, péremption/reconnexion et affichage réel cohérent avec le récepteur. Ne pas exiger FLOAT/FIX si le récepteur ne les a pas réellement acquis ; ne pas les fabriquer pour accepter l'image.
10. N'autoriser l'état prêt à fusionner #1017 que lorsque les critères de la paire et la validation matérielle effective sont satisfaits ou qu'un arbitrage utilisateur explicite en modifie le contrat. Aucun de ces GO n'est inféré de la réussite des builds locaux.

## Limite d'exécution de cette validation

Le Dockerfile committé utilise root pour ses compilations historiques. Afin de respecter la règle locale non-root, une copie de recette ajoute uniquement ownership, USER1000 et cache UID/GID, avec sources/pins/commandes de compilation et tests conservés. Le diff et les SHA256 exacts sont livrés. Cette adaptation doit rester visible dans la décision de publication ; elle ne prouve pas une construction octet-identique par le Dockerfile original. Le runtime conserve ses instructions et son utilisateur de production. Les bases et paquets apt historiques ne sont pas intégralement gelés : versions réellement installées et matériaux de build sont enregistrés ; la reproductibilité du contrat ne promet pas la reproductibilité bit-à-bit des binaires.
