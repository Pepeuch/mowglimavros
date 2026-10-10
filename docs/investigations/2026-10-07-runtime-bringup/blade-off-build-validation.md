# Validation compilation/graph OFF-only — 8 octobre 2026

Avant build : patch OFF-only accepté, aucune extensionFWD/REV, aucun
déploiement/essai physique/commit/push autorisé. Submodule main9f62ba4b
avec patch non commité et changementsRPM préexistants conservés.
Règles/index et checkpointsproviders/safety/refresh/OFF relus.

Image locale retenue :
sha256:d98e8e9899e57cdcfb9b3b1aec5588bd0c64fc31956dcc726ec5c31ca6825316.
Vérifié : /mavros_ws/src/mavros HEAD5c68b905ab30de6ce630822dc46c33467e8f23ea,
package installéMAVROS2.16.0, sous-couche/opt/mowgli/mavros avecmavros_msgs.
Images historiques2.15.1 et image runtime avecUGpin différent non utilisées.

Conteneur de validation isolé --networknone, aucun device/volumehost/socket,
entrypointbash remplace launchrobot, utilisateur1000:1000. Sourcesactuelles
copiées seulement dans conteneurtemporaire ; build/install/log en/tmp etnonroot.
Compiler packages-up-to mowgli_mavros_bridge (interfaces+ESC+bridge), puis
testsbridge etESC, graphprovider/safety/ESC, formatagediff etcontrats.
Ne jamais utiliser le binaire ancieninstallé comme preuve du patch.
Résultatsàcompléter ; aucunecommandeFCU/SSHrobot nécessaire.

## Résultat final : PASS logiciel, arrêt avant déploiement

Compilation des trois packages avec BUILD_TESTING=ON / RelWithDebInfo :
mowgli_interfaces, mavros_esc_wheel_odometry, mowgli_mavros_bridge.
Les trois colcon_build.rc valent0 ; nouveau bridge compilé et installé.
Utilisateur ubuntu UID1000, GCC15.2, Python3.14.4, ROS2Lyrical,
MAVROS2.16.0 commit5c68b905ab30de6ce630822dc46c33467e8f23ea,
MAVLink2026.9.9 et libmavconn de cette sous-couche MAVROS.

Provenance source node (identique snapshot conteneur/fichier workspace) :
a687fe84cb715080f2787a0e5cca86fa9093c36f26787ac675f1350ea1347ca8.
SHA256 binaire installé testé explicitement :
ddea2c37b8b767c8d4e5c0ed47a62e1e74c59bb7485cc1a0963d01cf32a80922.
Le binaire build-tree et le binaire installé ont des SHA différents après
installation CMake/RPATH ; aucun raccourci d'identité de hash utilisé : les
deux chemins ont été soumis au graph provider avec succès.

## Préparation de l'environnement CycloneDDS

Premier passage :9/13 suites bridge PASS, quatre tests nécessitantROS échouent
immédiatement car librmw_cyclonedds_cpp.so absente dans l'image SDK. Ce n'était
pas une preuve d'échec du contrat OFF. Pas de changement de code/assertions,
de timeout ou de middleware pour faire passer ces tests.

Sous-couche runtime Cyclone importée uniquement dans /tmp/blade-off-cyclone/lib
depuis image locale
sha256:64027b767b3cc1b15203695eca4b15f7047922c2b18d1073ade4cd91e043e600 :
librmw_cyclonedds_cpp.so et libddsc.so.11, fichiers importés UID1000.
Paquets source : rmw_cyclonedds_cpp4.1.5-1resolute.20260915.051853,
cyclonedds11.0.1-4resolute.20260728.175307.
ABI ROS communes vérifiées identiques entre image SDK et donneuse :
rcl10.4.5-1resolute.20260915.060844,
rmw7.10.2-3resolute.20260901.091148,
rmw_implementation3.1.6-1resolute.20260915.053750.
Les autres bibliothèques viennent du SDK, pas de l'image donneuse ; aucun
binaire MAVROS/UG de l'image donneuse utilisé. Pas d'installation apt/hôte.
RMW_IMPLEMENTATION=rmw_cyclonedds_cpp, ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST,
domaines203 et domaines dédiés des tests ; conteneur --networknone.

## Tests exécutés avec les sources actuelles

13/13 suites CTest bridge PASS,0 échec, durée58,64s :
- test_blade_off_contract (2 vérifications statiques internes)
- test_blade_off_control (5 gtests)
- test_gnss_launch, test_firmware_provider, test_firmware_detection
- test_provider_graph, test_esc_graph, test_safety_graph
- test_readiness_state, test_esc_telemetry_tracker, test_command_provider
- test_rover_manual_control, test_safety_state.

5/5 suites CTest ESC PASS,0 échec : motor_tick_integrator(7),
observation_engine(49), signed_esc_contract(2), wheel_odometry_core(20),
wheel_tick_projection(6).

test_provider_graph.py également exécuté contre le binaire INSTALLE, via
ament run_test.py, résultat0 et rapport XML0failure/0error :
ArduPilot/PX4 × mowing_enabled true/false (quatre variantes PASS).
19 rapports XML finaux vérifiés,0failure/0error/0skipped. Les rapports ament
wrappers ne comptent pas séparément leurs cas Python internes : ne pas
présenter le total des attributs XML comme nombre exact de scénarios physiques.

Tests complémentaires hôte :10 external_backend_contract et3interfaces PASS.
clang-format18 fichiers C++ nouveaux PASS ; ligneschangées node/header via
clang-format-diff18 : sortie vide. git diff --check PASS. Aucune correction
du patch ou des tests n'a été nécessaire dans cette validation.

Le workspace complet6packages et le graphGNSS optionnel ne sont pas revendiqués :
la cible autorisée utilisée est packages-up-to bridge (3packages) ; adapterGNSS
optionnel non configuré. Les regressions provider/safety/ESC liées au patch
ont toutes été exécutées, incluant le vrai MAVROS local dans les graphs ESC/safety.

## Contrat vérifié dans le graph résultant

| Requête/événement | Observation et assertion |
|---|---|
| mower_control(false), ArduPilot | Client MAVROS CommandLong : command183, param1=3, param2=1500 |
| mower_control(true), toutes variantes | successfalse, aucune nouvelle demande ARM/DISARM ou servo |
| OFF répété10fois | Une seule demande183 totale, aucun DISARM |
| mower_control OFF/ON | Aucun ARM/DISARM depuis le service lame |
| Emergency | HOLD + DISARMfalse, traction immédiatement neutre et maintenue neutre jusqu'à libération |
| Double lift/safety | Graph safety et gtests existants PASS ; comportement retenu |

Comparaison exacte des corps de fonctions au HEAD9f62ba4b :
on_emergency_stop, on_mavros_sys_status, on_mavlink_source, on_mavros_imu,
request_blade_disarm, request_hold_and_blade_disarm, send_arm_command,
send_mode_command, on_high_level_status et on_cmd_vel sont inchangés.
Cette preuve est logicielle ; aucun effet ESC réel ni sens mécanique revendiqué.

## Artefacts et reprise

Conteneur local mowgli-blade-off-validation-20261008,
IDf5a4bbfc1515f86115bf69d4597fbf91a1bc4fb9097d4482bca7277b483550ec.
Artefacts conservés dans sa couche writable :
/tmp/blade-off-src, /tmp/blade-off-build, /tmp/blade-off-install,
/tmp/blade-off-log, /tmp/blade-off-test-log-cyclone.
Preuve installée : /tmp/blade-off-build/mowgli_mavros_bridge/installed_provider_graph.txt
et test_results/mowgli_mavros_bridge/test_provider_graph_installed.xml.

Aucun déploiement, commandeFCU, SSHrobot, essai ESC2, FWD/REV, commit/push.
Source applicative inchangée durant cette phase ; seul ce checkpoint est ajouté.
Le précédent ENVIRONMENT_PENDING compilation/graph du checkpoint OFF-only est
levé pour cette baseline logicielle. Signe mécanique ESC2 HARDWARE_REQUIRED et
failsafeFCUtotalcompanion NON RÉSOLU restent distincts et non validés.

STOP après validation. Prochaine action uniquement sur autorisation explicite
opérateur : déploiement robot contrôlé, jamais déduit de ce PASS. L'acceptation
ESC2signé ne reprend pas dans cette étape.
