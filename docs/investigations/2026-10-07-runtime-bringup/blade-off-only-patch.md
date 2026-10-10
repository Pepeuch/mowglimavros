# Patch minimal lame OFF — avant code,8octobre2026

Autorisation explicite « oui vas y » : préparer localement OFF→neutre,
ONinhibé, sans déploiement, aucun nouvel essai physique, commit/push.
Submodule main9f62ba4b ; testsRPM/checkpoints préexistants conservés.
AGENTS/index et checkpointsESC/safety/refresh/runtime relus dans cette reprise.

Avant : on_mower_control traduit enable/disable enARM/DISARM.
Après visé : ONfalse(successfalse), aucuneARM/DISARM depuis ce service ;
OFF183/servochannel3/PWM1500 configurable, acquittement et déduplication.
Contrat success conservé comme acceptation locale/transport, pas preuveRPMzéro.
ACKrefusé conservé enéchec sans boucler à chaque tick ; reconnexion réinitialise
la déduplication, callbacksancienneconnexion ignorés. Garder interventions
safety/HOLD/DISARM existantes inchangées, sans refonteautoritétraction.
Pas de sensforward/reverse/nominal100% arbitraire ; ONresteinhibé mêmearmé.
Pas deGUI/publicStatusnouveau, startupautomatique produit différé au patchcomplet.

Fichiers ciblés : node/header, petit étatneutre testable, config, testsprovider
et régressionOFF mockROS, CMake, README/TODO. Aucuncodeparent.
Validation pure/statique puis graph/build si dépendanceslocales disponibles ;
aucunbuildrobot, installationdépendances ou deployment implicite.
FailsafeFCUpertecompanion NON RÉSOLU ; acceptationsensESC2 HARDWARE_REQUIRED.

## Après patch — préparé pour revue, non déployé

Architecture : on_mower_control ne possède plus aucuneARM/DISARM. ON retourne
successfalse pour toutes directions, même mowing_enabledtrue/FCUarmé ; intent
mow_enabledfalse et direction0. OFF sur providerArduPilot connecté prépare
CommandLong183, param1=blade_servo_channel(default3), param2=blade_neutral_pwm
(default1500), broadcastfalse/autreschampsdéfaut. PX4 : primitive non validée
refusée, sans fallbackARM. Service indisponible/déconnecté : false, pas deRPC.

Paramètres ROS startup read_only, canal1..32, PWM1000..2000 validés ; aucun
paramètreArduPilot écrit. Ces paramètres ne remplacent pas la vérification
SERVO3_FUNCTION0 et du neutre physique avant déploiement/essai.

État OFF séparé Pending/Confirmed/Failed : une seule transaction par connexion,
pas d'envoi par tick si déjàqueued/confirmé ; ACKsuccess ET result0 exigés.
Rejet/exception conservé enFailed, commandes répétéesfalse, pas de retryboucle.
Connexion change→nouvelle génération/Idle ; ACKanciennegénération ignoré ;
prochaine requêteOFF explicite peut envoyer de nouveau. Paramètres figés pour
ne pas rediriger une transaction en vol.

Limites assumées de ce correctif minimal :
- success indique acceptationlocale/pending ouconfirmationACK, pas RPMzéro.
- Un futur sans réponse restePending (pas de retry ni preuve d'arrêt) ; le
  patch complet devra traiter la durée des transactions et les diagnostics.
- Déduplication décrit notre commande envoyée, pas ownership d'une commande
  externe ultérieure. Ne pas utiliser le cache comme preuve physique au banc.
- Pas decommandeautomatique startup/reconnexion ; prochaineOFF explicite,
  pas le contrat complet startupneutral demandé pour la future lameON.
- Emergency/lift/tilt etHOLD/DISARM actuels restent byte-for-byte inchangés ;
  neutralisationindépendante de ceschemins sera à fermer dans patchcompletlame.
- Aucunforward/reversePWM, sensvisuel, nominal100%, leaseFCU ouStatusnouveau.
  FailsafeFCUtotalcompanion NON RÉSOLU, aucuneproduction/autonomie revendiquée.

## Fichiers de ce patch

MowgliMAVROS uniquement :
- ros2/src/mowgli_mavros_bridge/src/mavros_hardware_bridge_node.cpp
- ros2/src/mowgli_mavros_bridge/include/mavros_hardware_bridge_node.hpp
- ros2/src/mowgli_mavros_bridge/include/mowgli_mavros_bridge/blade_off_control.hpp
- ros2/src/mowgli_mavros_bridge/config/hardware_bridge_mavros.yaml
- ros2/src/mowgli_mavros_bridge/test/test_blade_off_control.cpp
- ros2/src/mowgli_mavros_bridge/test/test_blade_off_contract.py
- ros2/src/mowgli_mavros_bridge/test/test_provider_graph.py
- ros2/src/mowgli_mavros_bridge/CMakeLists.txt / README.md, TODO.md.
Checkpoint uniquement dans ce dossier. Changements/testsRPM/checkpoints
préexistants conservés. AucuncodeMowgliNext modifié, AGENTSuser intact.

## Validation exécutée localement, utilisateurubuntu UID1000

PASS :5 gtests C++17 -Wall/-Wextra/-Wpedantic/-Werror, native183/configuration,
ONinhibé, déduplicationpending/confirmé, reje tsansboucle, générationreconnexion.
PASS :2 contratsstatiques complémentanttestsC++, absenceARM/mode dans service,
payloadneutre etACKresult0 ; ce ne sont pas des testsROS runtime.
PASS :10contratsexternalbackend,3interfacespubliques ; syntaxeASTgraphPython.
PASS :clang-format18 fichiersnouveaux, formatage/checkligneschangées node/header
via clang-format-diff18 (sortievid e), gitdiff--check. git-clang-format18 ne peut
pas créer son index temporaire dans .git readonly ; aucuneexceptionpermission
demandée, contrôlemécanique équivalent sansécritureindex utilisé.

ENVIRONMENT_PENDING : compilationcomplètenode etgraphROS. Dépendanceslocales
MAVROS/mavros_msgs/libmavconn absentes sous/opt/ros/lyrical et aucunconfigCMake
localtrouvé ; aucuneinstallationdépendances, buildrobot ouvalidationancienne
substituée. Graphprovideradapté pour4variantesArduPilot/PX4×mowingenabled,
ONfalse, OFF183/3/1500ArduPilot, OFFPX4false, pas de10répétitionsOFF,
emergencyHOLD/DISARMinchangé ; syntaxePASS, exécution NON EFFECTUÉE.

## Reprise exacte

Relire ce patch, compiler/exécutergraph dans environnementMAVROS épinglé,
revuepuisautorisationexplicite de déploiement sur robot. Seulementensuite
reprendre acceptationsigneESC2, bridge/sécurité restantactifs (pasSIGSTOP),
avec préflight/restaurationopérateur. Pas decommit/push ouSSH dans ce passage.
