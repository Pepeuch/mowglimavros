# P0 manuel — checkpoint obligatoire avant code

2026-10-07. Submodule main@9f62ba4b1afbccbdaa8b22899ecd4874bc28b204,
MowgliNext local4c461ba3 ; seuls les checkpoints runtime sont non suivis.
Demande opérateur : implémentation locale, aucun déploiement/essai physique,
aucun paramètre FCU, GUI, commit ou push. Tous les checkpoints dans ce dossier.

Avant : mower_control ON/OFF appelle ARM/DISARM ; HLS actifs sélectionnent
MANUAL mais n'ont pas d'autorité d'armement. Emergency natif/ROS dissociés.
Après visé : autorité manuelle séparée, heartbeat connecté/armé/MANUAL requis,
OFF lame par183 neutre uniquement, commandes dédupliquées et ACK suivis,
neutralisation startup/reconnexion, HLS emergency prioritaire, HOLD/DISARM.
Autonome GUIDED explicitement différé et inhibé plutôt que transmis en MANUAL.

Réutiliser FirmwareProvider, SafetyState, EscTelemetryTracker et clients MAVROS.
Convention publique direction0=forward,1=reverse conservée, PWM configurables.
Aucune valeur nominale ni sens physique par défaut : activation développement
explicite et configuration distincte nécessaires. Ne pas simuler STM32 compatible.
Mapping SERVO3→ESC2 observé ; moteur seulement entendu par opérateur,
rotation/sens mécanique et échelleRPM restent HARDWARE_REQUIRED.

Sécurité : annuler ON en emergency/perte autorité ; neutralisation avant
inversion, délai et observations ESC distinctes à zéro. Pas de réarmement sur
ancien HLS après reconnexion : nouvelle transition idle→manuel nécessaire.
Tests isolés ROS_LOCALHOST/domain dédié, faux services FCU, jamais le robot.

Failsafe FCU lame NON RÉSOLU. Source épinglée AP_ServoRelayEvents identifie
DO_REPEAT_SERVO184 cycles1 comme candidat temporisé vers TRIM, mais moteur
global partagé servo/relais et exclusivité/firmware custom non validés.
Ni GCS heartbeat MAVROS ni timer companion ne garantissent mort du bridge.
DO_SET_SERVO développement sous supervision uniquement, autonomie lame inhibée.

Fichiers visés : bridge node/header, contrôleur d'autorité/actionneur testable,
config YAML, tests C++/graph, CMake, README bridge et TODO submodule.
MowgliNext applicatif et GUI inchangés. Baseline physique détaillée dans
servo3-isolated-actuation-results.md. Validation logicielle à effectuer.
