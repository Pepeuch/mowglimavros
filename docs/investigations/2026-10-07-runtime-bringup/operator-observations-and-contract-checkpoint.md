# Runtime bringup — nouvelles observations opérateur, 7 octobre 2026

Disposition : ACTIVE. Investigation uniquement ; aucune correction autorisée.

## Périmètre et baseline

Ce checkpoint est distinct du rapport initial MowgliNext du 7 octobre 2026.
Il ne modifie ni ce rapport ni sa baseline historique.

- Workspace MowgliNext : `/ros2_ws/src/mowglinext`, HEAD `4c461ba3`.
- Submodule MowgliMAVROS : `ros2/src/external/mowglimavros`, branche `main`,
  HEAD/gitlink `9f62ba4b1afbccbdaa8b22899ecd4874bc28b204`.
- Robot à réidentifier : `pepeuch@192.168.10.32`, dépôt attendu
  `/home/pepeuch/mowglinext`, Rock 5B / Pixhawk5X / ArduRover 4.7.1.
- La baseline remote et le digest déployé seront relevés à nouveau : ne pas
  attribuer automatiquement le runtime actuel au gitlink local.
- Seules les écritures documentaires dans ce dossier sont autorisées.
  Aucun commit/push, changement de paramètre, restart, commande de mouvement,
  ARM/DISARM ou commande de lame.

## Observations opérateur reçues — non reproduites par l'agent

- Enregistrement de zone : impossible de faire avancer le robot depuis le GUI.
- Tonte manuelle GUI : les roues répondent ; le moteur de coupe ne démarre pas.
- L'API lame expose avant `(true,0)`, arrière `(true,1)` et arrêt `(false,0)`.
- Vitesses roues AV/AR asymétriques : collecte uniquement, aucun ajustement
  compensatoire dans le bridge ou MowgliNext.

Ces observations invalident la conclusion générale « les DISARM périodiques
expliquent à eux seuls le défaut de traction ». Le couplage lame/ARM constaté
dans le précédent runtime reste une preuve de ce runtime, pas une démonstration
de la cause de l'échec RECORDING.

## Questions prioritaires

1. Capturer passivement HighLevelStatus, FCU state, cmd_vel, MANUAL_CONTROL,
   outputs et ESC sur une même fenêtre ; comparer RECORDING et MANUAL_MOWING
   seulement si l'opérateur réalise ces états sans commande de l'agent.
2. Tracer GUI → teleop → mux → bridge → MAVLink et la politique de mode/ARM.
3. Vérifier dans ArduRover 4.7.1 la sémantique de MANUAL_CONTROL et des cibles
   natives GUIDED ; MANUAL → GUIDED seul n'est pas une correction établie.
4. Tracer GUI/API MowerControl → bridge → primitive FCU ; lire les paramètres
   SERVO/DroneCAN et la télémétrie pour identifier l'actionneur lame.

## Validation physique restante

HARDWARE_REQUIRED : correspondance ESC → moteur physique, sens réel et
fonctionnement OFF/FWD/REV. Avant tout essai susceptible d'énergiser une sortie,
confirmation opérateur obligatoire, lame retirée/déconnectée ou moteur neutralisé,
robot immobilisé et E-stop accessible. Procédure prévue : OFF → faible avant →
OFF → faible arrière → OFF ; seul l'ESC cible doit répondre, aucun mouvement
traction parasite, RPM/courant/sens vérifiés et retour à zéro.

## Baseline remote revalidée

Audit SSH réalisé sur `192.168.10.32` : utilisateur pepeuch UID 1000, hostname
`rock-5b`, Armbian 26.8.3 trixie / Debian 13.6, kernel
`7.1.8-edge-rockchip64`, ARM64. PWD initial `/home/pepeuch`.

Le dépôt remote est sur `feat/mavros-refresh`, HEAD `9fb33e59`, avec seulement
`docker/config/mavros/` non suivi. Le robot n'a pas encore le submodule
MowgliMAVROS intégré dans son checkout MowgliNext. Le gitlink local ne constitue
donc pas une preuve de révision du binaire distant.

Images actives (IDs Docker, distincts des digests de manifests registry) :

- MAVROS : `sha256:cfb4c921cc2ea23cf05b45843b8941af7a838ddf18a02091eeacc0dd460acf07`,
  tag `ghcr.io/pepeuch/mowglimavros/mowgli-mavros-sidecar:latest`,
  démarré `2026-10-07T18:04:18.7915975Z`.
- ROS2 : `sha256:e70db04697af95a98f4497b6e5184a0796dae76105d5fac9143b0011944c655c`,
  tag `feat-mavros-refresh`, démarré `2026-10-07T18:08:39.674448552Z`.
- GUI : `sha256:8111dd56fa8df7d223235eb667ad55a76d14afed2c14adb19e097d73e8de469e`,
  démarré `2026-10-07T17:52:56.439482393Z`.
- `/app/web/web-build.json` dans le GUI : révision
  `fb523ead7453d3e2792dd427dc0731832f8f9adb`, version `feat-mavros-refresh`,
  build `2026-10-07T16:20:52.924Z`.

Transport FCU USB Pixhawk5X if00 à 921600, MAVROS Lyrical / Cyclone DDS.
Firmware annoncé : ArduRover V4.7.1 `dbe79216`, Pixhawk5X
`002F002F 59425010 20333537`. Les sources ArduPilot ci-dessous sont épinglées
au commit complet `dbe792162d06cab66c3475fd5556bf7a120f119e`.
Le hash source FCU ne prouve pas l'identité complète du firmware custom installé.

## RB-01 — Mapping actuel confirmé par code et binaire

MowgliMAVROS `ros2/src/mowgli_mavros_bridge/src/mavros_hardware_bridge_node.cpp` :

- `high_level_motion_active()` regroupe AUTONOMOUS=2, RECORDING=3,
  MANUAL_MOWING=4.
- `on_high_level_status()` demande MANUAL uniquement quand on entre dans ce
  groupe depuis un état extérieur. Un passage RECORDING → MANUAL_MOWING ou
  MANUAL_MOWING → AUTONOMOUS ne provoque pas de sélection différente de mode.
- `on_cmd_vel()` convertit systématiquement `linear.x` en throttle `z` et
  `angular.z` en steering `y` du MANUAL_CONTROL, sauf neutralisation sécurité.
- Échelles runtime : linéaire 282.135, yaw 1000.0. Pas de gate de traction
  spécifique à RECORDING dans ce callback.
- `on_mower_control()` envoie `send_arm_command(blade_authorized)` ;
  `mow_direction` est mémorisé mais aucune sortie lame n'est commandée.
- `request_hold_and_blade_disarm()` demande HOLD, publie le neutre et désarme.
  Le STOP normal du BT envoie du zéro et lame OFF ; HOLD n'est pas demandé
  automatiquement à chaque passage IDLE/STOP.

Binaire distant vérifié statiquement :

- `high_level_motion_active()` à `0x23ab20` teste bien les états 2..4.
- `on_high_level_status()` à `0x2447c4` teste le changement de groupe et charge
  la chaîne `MANUAL` à `0x34fc10` avant `send_mode_command()`.
- `on_mower_control()` à `0x249200` appelle `send_arm_command()` à `0x249280`
  (preuve du rapport initial conservée ; image active inchangée).

## RB-02 — Chaîne téléop et différence entre les essais existants

Chemin du code MowgliNext lu dans le checkout remote :

`JoystickOverlay` → `useManualMode.handleJoyMove()` → WS
`/api/mowglinext/publish/joy` → `PublisherRoute` → `RosProvider.Publish()` →
relay WS port 8766 (Foxglove en fallback) → `/cmd_vel_teleop` → twist_mux →
`/cmd_vel` → bridge → `/mavros/manual_control/send` → MAVLink MANUAL_CONTROL.

`useMapStreams` ouvre le joystick aussi bien pour RECORDING que MANUAL_MOWING.
`MapPage` affiche le joystick pour ces deux états et emploie les mêmes handlers.
`handleJoyMove()` ne teste pas `manualMode` avant d'envoyer. Il répète la dernière
commande toutes les 100 ms. Les plafonds GUI sont ±0.25 m/s et ±0.6 rad/s.
L'audit statique ne démontre donc pas une absence systématique d'émission en
RECORDING. L'état du WebSocket du navigateur lors de l'échec reste inconnu.

Runtime : `/cmd_vel_teleop` a un publisher `cmd_vel_ws_relay` et un subscriber
`twist_mux`. Priorités mux lues : emergency 100 (timeout 0.2 s), tuning 30,
teleop 20 (0.5 s), docking 15, navigation 10. Un flux zéro emergency actif
peut masquer une commande teleop pourtant correctement émise : capturer les
deux entrées et la sortie pendant l'essai, pas seulement MANUAL_CONTROL.

Les journaux existants, rapprochés par les timestamps ROS et non les timestamps
d'émission Docker (sorties bufferisées), donnent :

| Fenêtre UTC du 7 octobre | Preuve BT | Preuve bridge |
| --- | --- | --- |
| 19:09:09–19:09:24 | command=7 à 19:09:14.831729 ; 84 demandes lame ON et 5 OFF | MANUAL accepté à 19:09:15.658113 ; 84 ARM et 5 DISARM acceptés |
| 19:10:55–19:11:09 | command=3 à 19:10:58.547560 ; RecordArea démarré à 19:10:59.856140 ; 0 ON, 41 OFF | MANUAL accepté à 19:10:59.858070 ; 0 ARM, 41 DISARM acceptés |
| 19:11:23–19:11:38 | command=7 à 19:11:27.279383 ; 101 ON, 3 OFF | MANUAL accepté à 19:11:27.958753 ; 101 ARM, 3 DISARM acceptés |

Pour Europe/Paris, ajouter deux heures à ces heures UTC. Les fenêtres incluent
quelques secondes avant/après transition ; les comptes ne sont pas un décompte
isolé de toute la durée de chaque état.

Il existe donc une différence d'armement associée à ces états, compatible avec
l'observation opérateur. Elle ne démontre pas encore que `/cmd_vel_teleop`
était présent pendant l'enregistrement, ni que le mux le transmettait, ni
l'état ARM effectif à chaque instant. Les logs de succès ACK ne remplacent
pas un heartbeat simultané et une observation de mouvement.

## RB-03 — Capture passive simultanée actuelle

Fenêtre `2026-10-07T19:32:47Z`–`19:33:05Z` (18 s) :

- HighLevelStatus : IDLE=1, sans transition (46 messages).
- FCU : connected=true, armed=false, MANUAL, system_status=4 (18 messages).
- `/cmd_vel` : 181 commandes zéro ; `/cmd_vel_emergency` : 181 commandes zéro.
- `/mavros/manual_control/send` : 180 messages, y=z=0.
- `/cmd_vel_teleop` : aucune donnée reçue pendant cette fenêtre.
- Aucun setpoint de vitesse ou raw/local reçu.
- `/mavros/rc/out` : 36 messages, sorties `[1500,1500,0,0,...]`.
- ESC : données fraîches ; tous les RPM à zéro. Le petit bruit de vitesse EKF
  à l'arrêt ne constitue pas une mesure de vitesse au sol.

Seconde capture passive MAVLink sortant, environ `19:35:48Z`–`19:35:58Z` :
99 MANUAL_CONTROL neutres, 98 COMMAND_LONG. Le dernier COMMAND_LONG décodé
est `MAV_CMD_COMPONENT_ARM_DISARM=400`, param1=0, cible 1/1. Source MAVROS
255/composant 191 ; MANUAL_CONTROL cible 1, y=z=0. Aucune commande envoyée
par l'agent. Pas de SET_MODE, SET_POSITION_TARGET_LOCAL_NED ou SET_ATTITUDE_TARGET
pendant cette fenêtre IDLE.

La première tentative de capture avait interrogé le graphe avant sa découverte
DDS et ne voyait aucun topic. Elle a été invalidée et remplacée par la capture
ci-dessus après délai de découverte ; elle ne prouve aucune panne runtime.

## RB-04 — Contrat ArduRover et proposition de modes

[GCS_MAVLink_Rover.cpp au commit FCU](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/Rover/GCS_MAVLink_Rover.cpp)
confirme que MANUAL_CONTROL est un override des entrées pilote steering(y)
et throttle(z), et que les cibles natives locales ne sont utilisées qu'en GUIDED.
`ModeManual::update()` transmet les entrées pilote au mixeur traction.

Pour `SET_POSITION_TARGET_LOCAL_NED`, le handler Rover exige une origine EKF
et traite vitesse + yaw_rate comme une consigne de vitesse et de taux de rotation.
La direction avant/arrière est dérivée du signe de vx quand yaw_rate est fourni.
[Documentation Guided](https://ardupilot.org/dev/docs/mavlink-rover-commands.html).
`ModeGuided` arrête la cible de vitesse/rotation après 3 s sans mise à jour.
Ce délai natif ne doit pas être confondu avec le bail de commande ROS/GUI.

| État | Actuel | Proposition à valider |
| --- | --- | --- |
| Enregistrement | MANUAL + MANUAL_CONTROL ; pas d'armement traction indépendant | MANUAL + MANUAL_CONTROL via joystick, FCU armé explicitement selon la politique traction, lame OFF indépendante |
| Tonte manuelle | MANUAL + MANUAL_CONTROL ; lame ON déclenche ARM global | MANUAL + MANUAL_CONTROL, même chaîne fonctionnelle conservée ; lame actionneur séparé |
| Autonome/reprise/transit | MANUAL + MANUAL_CONTROL, sans cible vitesse native | GUIDED + SET_POSITION_TARGET_LOCAL_NED, vitesse longitudinale et yaw_rate dans le repère corps |
| Arrêt normal | zéro via mux + lame OFF/DISARM ; peut rester MANUAL | HOLD + neutralisation du transport actif + lame OFF ; politique de désarmement traction explicitée |
| Sécurité/E-stop | HOLD + neutre + DISARM | HOLD + neutralisation + lame OFF et désarmement de sécurité selon contrat physique validé |

La proposition GUIDED est établie architecturalement, pas validée physiquement.
Le transport doit changer avec le mode : remplacer seulement MANUAL par GUIDED
laisserait un override pilote à la place de la cible native attendue.
Runtime `STICK_MIXING=0`, `GUID_OPTIONS=0`, vitesse nominale `WP_SPEED=0.5`.

Interfaces disponibles : `/mavros/setpoint_velocity/cmd_vel` et
`/mavros/setpoint_raw/local`, chacune avec son plugin subscriber mais **zéro
publisher**. Le paramètre `mavros/setpoint_velocity.mav_frame` est LOCAL_NED.
Un simple remap du Twist du robot dans ce repère serait incorrect.
Solution candidate : plugin velocity configuré BODY_NED, ou PositionTarget
raw explicite BODY_NED, masque 1479 (position/accélération/yaw ignorés),
vitesse longitudinale et yaw_rate. Avec l'API ROS MAVROS, conserver les
conventions FLU et vérifier sa conversion vers FRD/NED (y et yaw_rate changent
de signe). Ne pas appliquer la conversion une seconde fois dans le bridge.
Voir [plugin velocity MAVROS](https://github.com/mavlink/mavros/blob/ros2/mavros/src/plugins/setpoint_velocity.cpp)
comme référence de transformation ; sa branche ros2 n'est pas une preuve
de révision exacte de la bibliothèque déployée.

## RB-05 — Sorties FCU et chemin lame

Lecture du cache MAVROS puis lecture FCU `ParamPull(force_pull=true)` :
`success=true`, 894 paramètres reçus. Readback ultérieur confirme notamment
SERVO3_FUNCTION=35, CAN_D1_UC_ESC_BM=7, ARMING_REQUIRE=1, RC_OPTIONS=192.
Le pull ne modifie aucun paramètre FCU et n'envoie aucune commande d'actionneur.

| Sortie | Fonction runtime | MIN / TRIM / MAX | REVERSED | DroneCAN configuré |
| --- | --- | --- | --- | --- |
| SERVO1 | 74 = Throttle Right | 1000 / 1500 / 2000 | 0 | RawCommand index 0 |
| SERVO2 | 73 = Throttle Left | 1000 / 1500 / 2000 | 0 | RawCommand index 1 |
| SERVO3 | 35 = Motor3 | 1000 / 1500 / 2000 | 0 | RawCommand index 2 si une sortie non nulle est produite |
| SERVO4–16 | 0 = Disabled | 1100 / 1500 / 1900 | 0 | hors bitmap ESC actuel |

Les noms de fonctions sont vérifiés dans
[SRV_Channel.h au commit FCU](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/SRV_Channel/SRV_Channel.h).
**35 n'est pas une fonction « moteur de coupe ».** Une interprétation provisoire
erronée dans une mise à jour conversationnelle a été corrigée après lecture
de cette enum. ARM ne fournit pas une consigne Motor3.

CAN1 : driver 1, DroneCAN, 500 kbit/s, ESC_BM=7, ESC_OF=0, ESC_RV=7,
CAN node FCU 10. CAN2 : driver 2, 1 Mbit/s, aucun ESC sélectionné.
CAN_D1_UC_SRV_BM=0 : pas de commandes DroneCAN servo ArrayCommand configurées.
RELAY1–6_FUNCTION=0 : aucun relais configuré pour la lame.

Le masque ESC_RV=7 autorise les valeurs RawCommand positives et négatives pour
les trois sorties. `AP_DroneCAN::SRV_send_esc()` applique en outre soft_armed
et le masque safety ; un FCU désarmé impose zéro aux ESC. Il faut donc séparer
**condition globale d'armement** et **consigne indépendante de lame**, sans
prétendre que l'actionneur CAN fonctionne indépendamment de tout armement.
Le mapping sortie → index RawCommand provient du code/config ; il n'a pas
été mesuré sur le bus CAN dans ce passage.

Chemin API actuel conservé :

`MapPage/MowerActions` avant `(1,0)`, arrière `(1,1)`, OFF `(0,0)` →
`/api/mowglinext/call/blade_control` → `handleBladeControl()` →
`/behavior_tree_node/blade_control` → `BladeControlService` / `BladeDirection`
→ `/hardware_bridge/mower_control` → `on_mower_control()` → ARM/DISARM global.

La sélection GUI avant/arrière choisit la direction et enlève l'inhibition
opérateur ; elle ne contourne pas un OFF de l'arbre (idle, transit, safety).
OFF est inhibé au niveau BT et possède aussi un fallback direct hardware.
Ce comportement API n'a pas besoin d'être modifié pour ajouter une véritable
commande d'actionneur. Le backend n'utilise actuellement pas la direction
mémorisée pour commander un moteur.

### Primitive native candidate, non encore utilisable avec FUNCTION=35

`MAV_CMD_DO_SET_SERVO=183` via `/mavros/cmd/command` est une primitive standard
pour imposer une sortie PWM, ensuite transformée par ArduPilot en RawCommand
réversible DroneCAN. Param1 est le numéro de sortie 1-based, param2 la valeur
PWM. Mais
[AP_ServoRelayEvents::do_set_servo](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/AP_ServoRelayEvents/AP_ServoRelayEvents.cpp)
refuse les fonctions déjà réservées comme Motor3=35. Une sortie dédiée de
fonction compatible (p.ex. 0) serait nécessaire **après validation de son
identité physique et autorisation de configuration**. Rien n'a été changé.

OFF au neutre, FWD au-dessus et REV en dessous du neutre sont un candidat
pour un ESC réversible ; le neutre réel, le sens, les limites de faible puissance,
la conversion exacte et la réponse VESC restent à mesurer. Ne pas utiliser
1000/2000 comme première commande de test. DO_SET_SERVO conserve une consigne :
le retour OFF à perte de client/bridge et le délai de coast-down avant inversion
doivent avoir une autorité démontrée, sans réémettre indéfiniment une consigne
ancienne. Un simple watchdog CAN ne suffit pas si le FCU continue lui-même
à émettre la dernière consigne.

Les relais ne permettent pas seuls OFF/FWD/REV sur ce montage et ne sont pas
configurés. Aucune interface SET_ACTUATOR_CONTROL_TARGET/DO_SET_ACTUATOR
spécifique lame n'a été identifiée dans les handlers Rover/GCS de cette
révision. Ne pas déduire le support d'une primitive de sa seule présence
dans MAVLink ou d'un topic MAVROS générique.

Le YAML custom fourni sélectionne DroneCAN, MAV_SERVO_RELAY,
SERVORELAY_EVENTS, RELAY et MAV. Il ne sélectionne pas Lua/Scripting et aucun
SCR_* n'est présent dans le cache lu. Ce manifeste constitue une intention de
build ; il n'est pas une preuve du bitmap complet du binaire installé. Aucun
test DO_SET_SERVO (y compris OFF) n'a été envoyé pour vérifier le support compilé.

## RB-06 — Identité ESC et asymétrie

Rôles bridge runtime : right_esc_slot=0, left_esc_slot=1, blade_esc_slot=2.
Source télémétrie actuelle `ardupilot_legacy` / source=2. Slots 0/1/2 valides,
slot 3 invalide ; COMMON ESC_INFO/ESC_STATUS silencieux dans l'échantillon.
Ces rôles déclarés ne prouvent pas la relation aux fils et aux IDs VESC.

Échantillon passif à `19:34:26Z` :

| Slot | RPM | Courant | Tension | Température |
| --- | --- | --- | --- | --- |
| 0 | 0 | 0.01 A | 28.39 V | 39 °C |
| 1 | 0 | 0.02 A | 28.18 V | 38 °C |
| 2 | 0 | 0 A | 28.40 V | 33 °C |

Legacy `rpm_direction_valid=false` : la magnitude ne prouve pas le sens.
Des mesures antérieures de rotation manuelle, conservées dans
`MM-ESC-ODOMETRY-20261005`, associaient droite=ESC0/RPM1,
gauche=ESC1/RPM2 et coupe=ESC2. Elles restent datées et ne valident pas
automatiquement les indices de commande VESC ni le runtime actuel.

L'opérateur indique « il est sur le trois » et confirme avoir tourné le moteur
à la main. Capture dédiée de 50 s, `19:37:44Z`–`19:38:34Z` : 600/599/599
observations valides pour les slots 0/1/2, compteurs avançants, mais aucun RPM
non nul. FCU désarmé, MANUAL, lame OFF/200/0 RPM/0 A pendant cette fenêtre.
Le geste n'a pas été synchronisé ou détecté dans l'échantillon : **pas de
nouvelle preuve de mapping physique**. Ne pas conclure que le moteur n'a pas
tourné à partir de cette absence de détection.
Pas d'interface SocketCAN sur l'hôte Rock dans `ip -details link` : pas de lecture
directe du CAN1 Pixhawk ou de la configuration VESC par ce chemin. Aucun
forwarding CAN n'a été activé, aucun port série FCU existant n'a été ouvert.
Les IDs/noms VESC et leurs index de commande ne sont pas contenus dans la
télémétrie legacy normalisée : ils restent à lire avec un accès dédié sûr.

Asymétrie : plafonds GUI, échelles bridge et MIN/TRIM/MAX des roues sont
symétriques ; MOT_THST_ASYM=1.0 et SERVO1/2_REVERSED=0. Cela établit seulement
la symétrie de la conversion/configuration logicielle examinée. Pas de captures
AV/AR non nulles simultanées ni de mesure de vitesse réelle dans ce passage,
pas de lecture complète des paramètres VESC. Aucune compensation ajoutée.

## RB-07 — Essai GUI lame observé pendant cette investigation

L'opérateur signale un essai roues GUI en tonte manuelle puis le clic
« démarrer la lame sens avant ». L'agent observe uniquement.

Capture synchronisée `2026-10-07T19:41:22.903Z`–environ `19:43:22Z` :

- Début : IDLE=1, connected=true, armed=false, MANUAL, sorties `[1500,1500,0]`.
- `HighLevelControl command=7` reçu à `19:41:36.607Z`.
- À `19:41:42.933Z` : MANUAL_MOWING=4, armed=true, MANUAL,
  `hardware_bridge/status.mow_enabled=true`.
- MAVLink sortant : SET_MODE custom_mode=0 (MANUAL), puis COMMAND_LONG
  command=400, param1=1 (ARM), param2=0, répété. Aucun command=183
  (DO_SET_SERVO) ou autre commande d'actionneur dans les fenêtres observées.
- GUI : POST `/api/mowglinext/call/blade_control` répond 200 à `19:41:41Z`
  puis `19:42:00Z`. Le bouton avant est identifié par l'opérateur ; les logs
  HTTP ne contiennent pas le corps de requête/direction.
- Sorties restent `[1500,1500,0]`, ESC2 RPM=0, courant=0,
  tension environ 28.10–28.15 V ; roues ESC0/1 à 0 RPM.
- À `19:42:32.940Z` : retour IDLE=1, armed=false, MANUAL,
  mow_enabled=false. Commandes DISARM=400/param1=0 et MANUAL_CONTROL neutres.

**Défaut lame reproduit** : mode/ARM présents et intention ON publiée, mais
aucune consigne de sortie coupe ne traverse le chemin actuel. `mow_enabled`
représente une intention/autorisation, pas une preuve de rotation.

L'essai roues antérieur n'est pas couvert : le GUI journalise la fin d'une
connexion `/publish/joy` à `19:39:32Z`, durée 31.88 s, avant cette capture.
La référence « roues fonctionnelles » reste l'observation opérateur ; aucune
valeur non nulle cmd_vel/RPM ne doit être inventée pour l'essai manqué.

Une confirmation lame retirée/déconnectée ou moteur neutralisé, robot immobilisé
et E-stop accessible a été demandée pour tout éventuel test d'actionneur de
l'agent. La réponse « sur démarrer la lame sens avant » identifie le bouton,
sans confirmer ces prérequis. Aucun test de sortie de l'agent n'a été effectué ;
seule l'observation des actions opérateur a continué. OFF/FWD/REV natif non testé.

## RB-08 — Répétition synchronisée de la procédure opérateur

L'opérateur demande de regarder la procédure reprise, puis confirme l'arrêt.
Capture passive démarrée à `19:44:16.489Z` le 7 octobre 2026 :

- À `19:44:16.490Z` : HLS=4/MANUAL_MOWING ; à `19:44:16.620Z`, connected=true,
  armed=false, MANUAL. Statut mow_enabled=false et COMMAND_LONG 400/param1=0
  répétés ; sorties `[1500,1500,0]`.
- À `19:44:26.619Z` : heartbeat armed=true, MANUAL.
  Fenêtre `19:44:25Z`–`19:44:28Z` : statut mow_enabled false→true et commandes
  MAVLink 400/param1 0→1. L'intention lame arme donc bien le FCU dans cet essai.
- Jusqu'à `19:44:37.501Z` : HLS MANUAL_MOWING, armed=true, lame ON demandée,
  mais toutes les sorties observées restent `[1500,1500,0]`, ESC0/1/2 RPM=0,
  ESC2 courant=0, tension environ 28.00–28.06 V. Aucune commande 183 observée.
- À `19:44:38.160Z` : HLS=IDLE ; à `19:44:38.622Z` : armed=false, MANUAL.
  Commande 400/param1=0 et MANUAL_CONTROL neutre ; sortie3 toujours zéro.
  L'opérateur confirme avoir arrêté. Retour zéro RPM confirmé sur les trois ESC.

Cette séquence valide expérimentalement ON→ARM et arrêt→DISARM sans sortie
de coupe, sur l'image et les paramètres de cette baseline. Elle ne valide
pas l'actionneur OFF/FWD/REV natif, le sens physique ou les indices VESC.
Pas de commande teleop non nulle reçue dans cette séquence ; elle ne doit pas
être présentée comme un essai traction. Aucun geste physique de l'agent.

## Prochaine étape et critères de clôture

1. Achever l'observation de rotation manuelle proposée et confirmer slot coupe
   + arrêt des autres ESC ; identifier séparément l'index de commande VESC.
2. Réaliser, sous conduite opérateur et autorisation de test appropriée, une
   capture synchronisée RECORDING puis MANUAL_MOWING : HLS, armed/mode,
   teleop, emergency, cmd_vel, MANUAL_CONTROL wire, sorties et ESC.
   Si teleop est absent : investiguer GUI/WS ; s'il est présent mais cmd_vel
   zéro : mux/BT ; si envoyé non nul et FCU désarmé : autorité traction ;
   si envoyé/armé mais sorties zéro : interprétation FCU/safety ; si sorties
   présentes mais RPM nul : configuration VESC/acheminement physique.
3. Valider le contrat GUIDED/vitesse native sans mouvement autonome : conversion
   de frames, masque, unités, reverse, zero/pivot, cadence et perte de commande.
4. Identifier l'actionneur coupe et approuver sa fonction de sortie, la primitive
   native, la protection de perte de commande et la garde d'inversion.
5. Seulement après ces preuves, proposer un patch ciblé pour revue humaine.

HARDWARE_REQUIRED : RECORDING/tonte manuelle simultanés et identité de commande
VESC, fonctionnement physique OFF/FWD/REV et arrêt de sécurité non validés.
Un essai de sortie exige une nouvelle confirmation opérateur : lame retirée/
déconnectée ou moteur neutralisé, robot immobilisé, E-stop accessible. Critère :
seul ESC cible, aucun RPM traction parasite, faible consigne, sens observé,
RPM/courant et retour zéro à chaque OFF, arrêt garanti en perte de commande.
Ne pas lancer la tonte autonome pour acquérir ces preuves.

Aucun changement de code, paramètre, mode, firmware ou conteneur ; aucun commit
ou push. Seul ce checkpoint dans le dossier demandé est écrit.
