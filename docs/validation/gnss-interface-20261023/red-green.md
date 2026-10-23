# RED/GREEN — GNSS MAVROS

Les commandes exactes, dates UTC, durées et codes de retour figurent dans les `evidence/*-result.json`. Utilisateur ubuntu (UID 1000), jamais de build root. Dépendances natives extraites de l'image amd64 exacte fournie ; MAVLink header-only 2026.9.9 et Asio 1.30.2. Les SDK/builds restent sous /tmp, hors patch.

| Preuve | Résultat réel |
|---|---|
| `baseline-false-assurance` | Ancien outil + copie/verrou anciens + `--source-root` actuel : exit 0. Démonstration du trou de contrôle. |
| `red-contract-copy` | Nouveau verrou canonique avec copie GnssStatus encore ancienne : exit 1. |
| `red-installed-old` | Ancien paquet ROS généré contre nouveau verrou : exit 1, hash GNSS ancien refusé. |
| `red-provenance` | Trois nouveaux gtests, nouvelle IDL mais mapper de baseline : **3 FAIL**, identités vides. Isole le défaut sémantique après correction du seul contrat. |
| `aligned-sidecar-build` | Six paquets reconstruits avec tests : PASS. Pas de greffe de bibliothèque IDL seule dans un ancien plugin. |
| `final-static-checks` | Contrat source exact + consommateur réel + huit types installés PASS ; **8 tests Python contrat** et **10 contrats backend** PASS ; diff et clang-format 18 des lignes C++ modifiées PASS. |
| `consumer-installed-contract` | Les huit hashes attendus correspondent aussi au préfixe MowgliNext installé : PASS. |
| gtest adaptateur | **17 PASS**, dont 3 nouveaux de provenance. Identités opaques/vides, enrichissement identique, expiration, restart et changement de source. Toutes les assertions historiques conservées. |
| graphes corrigés de leur environnement | GNSS, ESC et safety PASS avec véritable fichier EGM96 existant. GNSS couvre GPS1/GPS2 et les deux modes direct. Aucune assertion assouplie. |
| `final-current-tests` | Résultats JUnit/ament actuels : **224 cas, 0 erreur, 0 échec, 0 skip**, dans 26 fichiers ; paquet interfaces sans test exécutable distinct. |
| `cross-mowglinext-consumer` | Vrai plugin -> DDS -> types MowgliNext dans un second processus : PASS. Aucun FCU ou robot. |
| `gui-gnss-go-recheck` | **8 tests Go existants** providers/Foxglove PASS, code GUI inchangé. |
| `gui-gnss-frontend` | **78 tests gnssTelemetry existants** PASS, code GUI inchangé. |

## Premiers essais et contre-preuves conservés

`adapter-red-build` a d'abord manqué la configuration CMake MAVLink, puis `adapter-red-build-recheck` les en-têtes Asio. Le binaire gtest a néanmoins été construit et a exécuté le RED sémantique. Dépendances corrigées dans /tmp, aucune modification système ou de source de production pour contourner les builds.

`green-sidecar-tests` : les tests unitaires passent, mais trois graphes échouent au démarrage MAVROS avec le message explicite GeographicLib `egm96-5.pgm` introuvable. Le fichier réel existait sous `/tmp/manual-provider-fix/geographiclib/geoids/`, 18 671 448 octets, SHA256 `c4b25a03ec5845cec4778a54b580aeda676363f2205a89137e8677b2337af3ec`. Après définition de `GEOGRAPHICLIB_GEOID_PATH`, seuls ces trois graphes et le gtest reformatté ont été relancés : les quatre sont PASS.

Le résultat global de `green-graphs-recheck` reste exit 1 : sa dernière commande `colcon test-result` parcourt aussi l'ancien dashboard CTest et retrouve ses trois échecs historiques. Ils n'ont pas été supprimés. `final-current-test-results` s'est ensuite arrêté sur le paquet interfaces sans dossier test_results (exit 2). `final-current-tests` inspecte précisément les cinq dossiers de résultats JUnit actuels, tous verts. Le rapport ne transforme pas ces commandes échouées en PASS.

Le premier test Go était bloqué par le cache utilisateur en lecture seule dans le sandbox ; le recheck autorisé sur les mêmes sources passe. Aucun test de publication Docker, job GitHub ou image ARM64 candidate exécuté ici ; les nouvelles étapes du workflow sont vérifiées statiquement (YAML et syntaxe shell) et leurs commandes contractuelles sont exécutées localement. La suite frontend complète n'a pas été relancée : aucun code GUI modifié.
