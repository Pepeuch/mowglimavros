# Audit passif fréquences MAVLink — 2026-10-09

Scope : mesure60s réelle sur192.168.10.32/rock-5b, lecture configuration et
seuils logiciels ; aucune modification FCU/intervalle/VESC/code produit,
aucun ARM/actionnement/restart/commit/push. Outil diagnostic temporaire,
abonnements uniquement, lectures des paramètres en cache après la mesure.

Baseline : ArduRover4.7.1 dbe79216/Pixhawk5X, sidecar788d5831…
démarré2026-10-09T07:39:58.379363269Z, bridgeARM64 SHA47527d46…,
MowgliMAVROS main9f62ba4b + patch validé noncommité. Robot UID1000pepeuch,
Armbian26.8.3 Debian13.6/kernel7.1.8-edge-rockchip64/aarch64, PWD/home/pepeuch.
Device/dev/mavros résolu/dev/ttyACM0, topologie/baud à préciser.

Mesure via/uas1/mavlink_source best-effort, sources sysid/compid séparées,
framingOK seulement, horloge monotone de réception callback. Hz=N/fenêtre,
intervalles entre messages consécutifs, percentiles interpolés, gaps de bord
séparés ; ne pas assimiler cadence observée ROS à capture électrique exhaustive.
Mesure aussi republications et identités ESC distinctes source/count/stamp.
Flux sortant observé uniquement : commandes511/512 etREQUEST_DATA_STREAM,
aucune requête de fréquence émise par l'auditeur. Le démarrage passé ne peut
pas être recapturé sans restart ; n'en effectuer aucun pour cet audit.

État initial : mesure/configuration/consommateurs/recommandations à acquérir.

## Résultats vérifiés et qualité de mesure

Disposition RETAINED pour cet audit ; acceptation FWD/REV toujours incomplète.
Fenêtre retenue : epoch1791541713.8754497→1791541773.8991547,
durée monotone60.020539788s, diagnostic `mowgli-mavlink-passive-rate-audit-20261009-r2`,
exit0. Contre-mesure par instance/topic30.019915084s, epoch1791541895.1392019→
1791541925.1611705, diagnostic `…-instances`, exit0. Sources reçues1/1 seulement.
Framing invalide0, pas de saut de séquence observé (modulo256). Cela n’est
pas une garantie absolue de zéro perte/filtrage ni une capture électrique USB.

Premier essai60.002957s avec KEEP_LAST5 exclu des conclusions de fréquence :
il sous-comptait les rafales (exemple ATTITUDE26 contre60 au second essai).
La contre-mesure a UNIQUEMENT augmenté la profondeur du subscriber diagnostic
à4096, best-effort inchangé ; aucun QoS/paramètre du produit ou FCU modifié.
Les gaps artificiels de3–10s du premier observateur ne prouvent pas une panne
FCU. Les stamps backend et intervalles callback de la mesure retenue concordent.

Preuves agrégées (pas un dump de paquets) :
[mavlink-frequency-proof-20261009.json](mavlink-frequency-proof-20261009.json).
CSV exhaustif89types reçus ou enregistrés par les plugins actifs :
[mavlink-frequency-metrics-20261009.csv](mavlink-frequency-metrics-20261009.csv).
Count0 signifie NON OBSERVÉ dans cette fenêtre, pas message impossible/non supporté.
Pour N0/1, périodes/percentiles/gap entre messages sont non calculables (CSV vide).

## Configuration courante, lue sans FCU pull/écriture

- URL réelle : `serial:///dev/serial/by-id/usb-Holybro_Pixhawk5X_2F002F001050425937353320-if00:921600`,
  device USB CDC `ttyACM0`. 921600 est le baud configuré, pas une mesure de débit
  utile USB. GCS relay : `udp-b://@255.255.255.255:14550`.
- SERIAL0_PROTOCOL2, SERIAL0_BAUD921 ; autre port MAVLink SERIAL8_PROTOCOL2,
  SERIAL8_BAUD921, OPTIONS0. UARTs ne doivent pas être assimilés au numéro
  du groupe stream ; MAV1 correspond au canal0, MAV2 au canal1.
  GCS.cpp de la révision installée enregistre précisément les groupes1/2 sur
  _chan[0]/_chan[1]. USB est le premier port MAVLink de ce profil (SERIAL0).
- SERIAL1_PROTOCOL11/BAUD230 ; SERIAL2_PROTOCOL-1/BAUD57 ;
  SERIAL3_PROTOCOL5/BAUD460 ; SERIAL4_PROTOCOL5/BAUD921 ;
  SERIAL5_PROTOCOL22/BAUD115 ; SERIAL6_PROTOCOL9/BAUD115 ;
  SERIAL7_PROTOCOL9/BAUD115. Ces autres ports ne sont pas la liaison MAVROS.
- MAV1 et MAV2 : RAW_SENS/EXT_STAT/RC_CHAN/RAW_CTRL/POSITION/EXTRA1/EXTRA2/
  EXTRA3=1Hz ; PARAMS10 ; ADSB0 ; OPTIONS0. MAV_SYSID1, GCS_SYSID255,
  GCS_SYSID_HI0, MAV_OPTIONS0, MAV_TELEM_DELAY0.
- Aucun SRx_* exposé par le cache MAVROS de cette révision4.7.1 :
  les paramètres courants sont MAV1_*/MAV2_*. Ne pas écrire des noms SRx
  anciens ni supposer MAV1 désigne SERIAL1.
- Profil MAVROS `apm_config.yaml`, `apm_pluginlists.yaml`, sélection auto→ArduPilot.
  heartbeat_rate1 et timesync_rate10 sont des ÉMISSIONS du companion, pas
  des commandes pour configurer tous les messages FCU. system_time_rate1 pareil.
- Readiness5s, ESC observation3s, Battery observer5s, sourceESCauto, rôles0/1/2,
  wheel_odometry_requiredfalse. Valeurs effectives lues après les mesures.
- EntryPoint installé `/ros2_entrypoint.sh` source les overlays et lance
  `mavros_backend.launch.py` ; aucune commande intervalle dans cet entrypoint,
  le launch/bridge custom ou les plugins GNSS/ESC/batterie inspectés.
- Source MAVROS **commit exact5c68b905ab30de6ce630822dc46c33467e8f23ea** acquise
  en lecture depuis le dépôt officiel, plutôt qu’extrapoler la branche actuelle.
  sys_status.cpp:622–632 expose les services set_stream_rate/set_message_interval ;
  1452–1496 traduit l'appel intervalle en command511. Ce chemin n’est pas
  invoqué par connection_cb. Startup réclame version/capacités ponctuelles :
  REQUEST_MESSAGE512 n’est PAS SET_MESSAGE_INTERVAL511.
- Aucun511, REQUEST_DATA_STREAM66,183,400 ou32000 observé sortant durant les
  fenêtres retenues. Les logs startup courants ne prouvent pas un historique
  exhaustif des commandes ; **aucune capture passée de startup511 disponible**.
  Aucun restart effectué pour en créer une. Une demande par un GCS extérieur
  sur un autre canal n'est pas exclue par la seule écoute du sink MAVROS.

Source protocole : [ArduPilot requesting data](https://ardupilot.org/dev/docs/mavlink-requesting-data.html).
Source implémentation pinned :
[MAVROS sys_status.cpp](https://github.com/mavlink/mavros/blob/5c68b905ab30de6ce630822dc46c33467e8f23ea/mavros/src/plugins/sys_status.cpp),
[ArduPilot GCS.cpp installé](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/GCS_MAVLink/GCS.cpp).

## Statistiques messages réellement reçus — fenêtre60.02054s

Toutes périodes et gaps ci-dessous en **millisecondes**, Hz=N/durée.
p95/p99 interpolés sur N−1 inter-arrivées ; max est le plus grand gap interne.
Le JSON conserve en plus gaps de bord et gaps des stamps backend. À cadence1Hz,
un peu plus de60s explique Hz0.99966, pas un réglage0.99966Hz.

| Message (#ID) | N | Hz | Période moy. ms | Min / max ms | p95 / p99 ms | Gap max ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| HEARTBEAT (0) | 60 | 1.000 | 1000.06 | 996.72 / 1004.00 | 1002.85 / 1003.62 | 1004.00 |
| SYS_STATUS (1) | 60 | 1.000 | 1000.13 | 978.56 / 1025.82 | 1010.63 / 1019.99 | 1025.82 |
| SYSTEM_TIME (2) | 60 | 1.000 | 1000.13 | 962.04 / 1031.86 | 1017.28 / 1030.64 | 1031.86 |
| PARAM_VALUE (22) | 14 | 0.233 | 4076.89 | 0.44 / 11000.43 | 10995.74 / 10999.49 | 11000.43 |
| GPS_RAW_INT (24) | 60 | 1.000 | 1000.10 | 968.99 / 1030.33 | 1024.06 / 1030.32 | 1030.33 |
| RAW_IMU (27) | 60 | 1.000 | 999.93 | 971.61 / 1041.05 | 1017.60 / 1030.91 | 1041.05 |
| SCALED_PRESSURE (29) | 60 | 1.000 | 1000.06 | 976.51 / 1028.30 | 1019.11 / 1024.72 | 1028.30 |
| ATTITUDE (30) | 60 | 1.000 | 1000.07 | 979.83 / 1022.10 | 1004.91 / 1013.92 | 1022.10 |
| LOCAL_POSITION_NED (32) | 60 | 1.000 | 1000.16 | 969.81 / 1035.31 | 1022.20 / 1030.88 | 1035.31 |
| GLOBAL_POSITION_INT (33) | 60 | 1.000 | 1000.10 | 979.71 / 1023.07 | 1006.86 / 1016.15 | 1023.07 |
| SERVO_OUTPUT_RAW (36) | 60 | 1.000 | 1000.18 | 975.99 / 1038.66 | 1017.98 / 1028.77 | 1038.66 |
| MISSION_CURRENT (42) | 60 | 1.000 | 1000.17 | 969.07 / 1037.49 | 1019.05 / 1028.96 | 1037.49 |
| RC_CHANNELS (65) | 60 | 1.000 | 999.92 | 973.22 / 1040.01 | 1017.28 / 1029.41 | 1040.01 |
| VFR_HUD (74) | 60 | 1.000 | 1000.11 | 979.33 / 1024.25 | 1008.70 / 1018.04 | 1024.25 |
| TIMESYNC (111) | 606 | 10.097 | 99.00 | 0.42 / 306.19 | 262.00 / 282.42 | 306.19 |
| SCALED_IMU2 (116) | 60 | 1.000 | 1000.02 | 969.02 / 1026.57 | 1017.18 / 1021.41 | 1026.57 |
| GPS2_RAW (124) | 60 | 1.000 | 1000.12 | 965.60 / 1033.71 | 1026.68 / 1032.19 | 1033.71 |
| POWER_STATUS (125) | 60 | 1.000 | 1000.14 | 978.42 / 1026.66 | 1012.22 / 1021.61 | 1026.66 |
| SCALED_IMU3 (129) | 60 | 1.000 | 1000.04 | 976.53 / 1027.40 | 1016.72 / 1022.72 | 1027.40 |
| SCALED_PRESSURE2 (137) | 60 | 1.000 | 1000.08 | 973.30 / 1029.57 | 1021.41 / 1027.50 | 1029.57 |
| BATTERY_STATUS (147) | 60 | 1.000 | 1000.03 | 963.27 / 1037.83 | 1023.25 / 1034.75 | 1037.83 |
| MEMINFO (152) | 60 | 1.000 | 1000.15 | 978.40 / 1027.44 | 1016.79 / 1023.25 | 1027.44 |
| AHRS (163) | 60 | 1.000 | 1000.04 | 979.84 / 1020.32 | 1001.65 / 1010.19 | 1020.32 |
| AHRS2 (178) | 60 | 1.000 | 1000.06 | 979.87 / 1021.15 | 1002.97 / 1012.02 | 1021.15 |
| EKF_STATUS_REPORT (193) | 60 | 1.000 | 1000.15 | 958.67 / 1032.30 | 1019.01 / 1027.82 | 1032.30 |
| RPM (226) | 60 | 1.000 | 1000.01 | 965.38 / 1034.43 | 1022.46 / 1031.45 | 1034.43 |
| VIBRATION (241) | 60 | 1.000 | 1000.18 | 967.50 / 1030.92 | 1025.39 / 1028.11 | 1030.92 |
| ESC_TELEMETRY_1_TO_4 (11030) | 60 | 1.000 | 1000.04 | 960.01 / 1041.14 | 1026.64 / 1039.90 | 1041.14 |

ESC_INFO290, ESC_STATUS291, DISTANCE_SENSOR132, WHEEL_DISTANCE9000 et
BUTTON_CHANGE257 : N0/Hz0, périodes et percentiles non calculables. Pareil pour
tous les inputs enregistrés mais absents de la fenêtre, détaillés dans le CSV.
VIBRATION241 reçu mais pluginvibration désactivé. EKF_STATUS_REPORT193 reçu,
mais sys_status épinglé écoute ESTIMATOR_STATUS230, PAS193. SCALED_IMU2/3,
SCALED_PRESSURE2, POWER_STATUS, AHRS/AHRS2 reçus sans handler fonctionnel actif
identifié ; ce ne sont pas des sources de fraîcheur pour la lame.

## Cadence physique/provenance versus republication

| Signal | Messages/acquisitions | Hz | Gap maximal |
| --- | ---: | ---: | ---: |
| ESC0 delivery | 180 | 2.999 | 0.503939s |
| ESC0 source/count distinct | 60 | 1.000 | 1.022426s |
| ESC1 delivery | 180 | 2.999 | 0.504198s |
| ESC1 source/count distinct | 60 | 1.000 | 1.022768s |
| ESC2 delivery | 180 | 2.999 | 0.505028s |
| ESC2 source/count distinct | 60 | 1.000 | 1.024259s |
| BATTERY_STATUS ensemble (60s) | 60 | 1.000 | 1.037830s |
| Batterie0 (30s) | 15 | 0.500 | 2.023035s |
| Batterie1 (30s) | 15 | 0.500 | 2.013034s |

SERVO_OUTPUT_RAW port0 uniquement,30 messages/30s. Topics/imu/data,
sys_status,rc/out,state,gps/status etPower environ1Hz chacun, callbacks frais ;
Power à1Hz regroupe deux instances qui chacune n’apportent qu’une mesure/2s.
Le graphe réel confirme :
/mavros/imu → bridge ; /mavros/sys → bridge state/sys_status ;
/mavros/rc → bridge ; /mavros/battery_observer → bridge/BT/diagnostics ;
/mavros/mowgli_gnss → /gps/status → bridge/BT/navsat/MQTT.
Le JSON conserve producteurs/subscribers et statistiques30s complètes.

## Audit des seuils côté MowgliMAVROS

| Topic/contrat critique | Garde effective | Source/symbole |
| --- | --- | --- |
| /mavros/state | connexion MAVROS10s ; disponibilité lame≤2500ms monotones | on_mavros_state / drive_blade_locked ; sys conn_timeout effectif10 |
| /mavros/sys_status | hardware safety Released et reçu≤3000ms pour ON | drive_blade_locked / on_mavros_sys_status |
| /mavros/imu/data | readiness5s ; tilt persistant500ms, angle56° | on_mavros_imu / ReadinessState |
| /hardware_bridge/power | BatteryState5s par instance ; readiness/diagnostic bridge5s | battery_observer / on_power / publish_readiness |
| /esc_observation | observation engine3s, tracker3s sur stamp accepté ; BladeControl≤1000ms monotones par acquisition distincte | on_esc_telemetry / EscTelemetryTracker / BladeControl::sample,tick |
| /gps/status | readiness5s sur identité séquence/backend nouvelle ; adapter3s observationposition nouvelle | ReadinessState::gnss / AdapterState::fresh |
| /gps/fix et statut UG interne | appariement20ms, délai livraison100ms ; replay stamp/seq refusé | AdapterState::take_fixes |
| /wheel_odom | readiness5s ; condition requise désactivée sur cette baseline | ReadinessState::wheel ; wheel_odometry_requiredfalse |
| /uas1/mavlink_source BUTTON_CHANGE | pas de TTL invalidant automatiquement l'état bouton ; cache reset au disconnect, durée/âge observables | SafetyState / on_mavlink_source |
| /mavros/rc/out | stamp doit progresser pour valider changementPWM ; **aucun TTL explicite du cache sortie** | sub_blade_output callback |
| /uas1/mavlink_sink183 | fenêtre matching local2s, pas preuve ownership/lease et pas liveness topic | blade_wire_expectations / sub_blade_wire |
| /cmd_vel | pas de deadline locale explicite dans ce bridge | on_cmd_vel |
| /behavior_tree_node/high_level_status | pas de deadline locale ; entrée motion déclenche modeMANUAL | on_high_level_status |
| RPC commande lame | ACK3s, réponse service20s, attente arrêt15s, coast≥1s ET≥5 acquisitionszéro | BladeControl / drive_blade_locked |
| MAVROS command ACK | délai effectif5s ; différent du délai3s de FSM lame | /mavros/cmd command_ack_timeout lu5.0 |

Ces seuils ont des rôles différents : diagnostics readiness5s n’est pas une
autorisation de garder la lame active5s ; un callback/cache republié ne
renouvelle pas la preuve d'arrêt. La barrière tilt500ms s'évalue lors des
callbacksIMU : à1Hz elle ne garantit pas une détection en500ms. Il n’existe
pas dans ce passage de preuve produit sécurité lors d’une IMU figée.

FREQ-001 (OPEN, owning bridge/ESC cadence) : **1Hz sans marge est incompatible
avec une garde acquisition1000ms** ; gaps distincts ESC0/1/2>1s réellement
mesurés, et le compteur ne progresse qu’une fois/s malgré3 publications/s.
Cela expose l'ON/inversion à invalidation/remise àzéro de la preuve arrêt.
Ce résultat n’identifie PAS à lui seul la cause exacte du refus FWD précédent :
concurrence OFF/ownership et autres invalidations restent à corréler.
Ne pas contourner la provenance ni allonger le délai pour faire disparaître
le symptôme.

FREQ-002 (RETAINED, outil) : KEEP_LAST5 sous-compte un flux multitype en rafales ;
ne pas réutiliser sa cadence comme preuve d'un défautFCU. Subscriber diagnostic
4096 et contrôle séquence ont levé ce défaut de mesure, sans toucher au produit.

Le subscriber brut du bridge (BUTTON_CHANGE) utilise aussi SensorDataQoS
par défaut : risque analogue à qualifier sous rafales, mais aucune perte
BUTTON_CHANGE du produit n'est prouvée ici. Ne pas transformer le défaut du
premier observateur en conclusion de panne de ce subscriber sans capture dédiée.

## Tableau demandé — propositions après audit, NON appliquées

Les Hz proposés sont des cibles d’ingénierie sur cette installation, **pas**
des fréquences physiquement validées ni garanties de deadline. Pas d’inférence
signée àpartirPWM. Ne pas augmenter des types absents sans vérifier support/
producteur/consumer. Pour GNSS, répéter GPS_RAW_INT plus vite ne crée pas un
nouveau fix ; la cadence doit respecter les acquisitions récepteur.
Pour les trois ESC, même contrat/fraîcheur/cadence, aucun cas spécialESC2.

| MAVLink message | Current Hz | Consumer ROS2 | Freshness requirement | Recommended Hz (proposition NON appliquée) |
| --- | ---: | --- | --- | --- |
| HEARTBEAT #0 | 1.000 | sys_status → /mavros/state → bridge | 10s connexion MAVROS ; 2,5s autorisation lame | 1 (conserver) |
| SYS_STATUS #1 | 1.000 | sys_status → /mavros/sys_status → safety bridge | 3s autorisation lame | 5 |
| ATTITUDE #30 | 1.000 | imu → /mavros/imu/data → tilt/readiness | 5s readiness ; tilt500ms exige des callbacks plus rapides | 50 |
| LOCAL_POSITION_NED #32 | 1.000 | local_position → pose/odom FCU (pas fusion_graph) | pas de TTL local plugin ; ne remplace pas odométrie canonique | 1 ; 10 seulement si consumer navigation identifié |
| GLOBAL_POSITION_INT #33 | 1.000 | global_position → fix/odom/heading FCU | pas de TTL local plugin ; pas la source GNSS canonique | 1 ; 5 si usage identifié |
| GPS_RAW_INT #24 | 1.000 | global_position/gps_status/universal_gnss/mowgli_gnss GPS1 | adapter3s acquisition nouvelle ; readiness5s | 5 si récepteur≥5Hz, sinon taux acquisition natif |
| GPS2_RAW #124 | 1.000 | gps_status/universal_gnss GPS2 (GPS1 canonique actuel) | pas de besoin GPS1 ; adapter3s si GPS2 sélectionné | 1, pas augmenter source inutilisée |
| SERVO_OUTPUT_RAW #36 | 1.000 | rc_io → /mavros/rc/out → cache lame | stamp distinct ; aucun TTL lame explicite sur ce topic | 10 |
| RC_CHANNELS #65 | 1.000 | rc_io → /mavros/rc/in | pas de TTL bridge, pas une commande traction du banc | 1–5 selon télémétrie opérateur |
| BATTERY_STATUS #147 | 1.000 | sys_status + battery_observer → Power → bridge/BT | 5s par instance ; 0,5Hz par batterie actuellement | 2 total (~1 par instance) |
| RPM #226 | 1.000 | esc_wheel_odometry → wheel ticks/odom | 3s intégrateur/observations ; identité distincte requise | 20 si roue RPM est bien la source retenue |
| ESC_TELEMETRY_1_TO_4 #11030 | 1.000 | esc_telemetry + esc_wheel_odometry → ESC0/1/2 → blade | 3s tracker ; 1s acquisition lame + ≥5 zéros/≥1s | 10 pour le groupe, identique pour trois ESC |
| ESC_STATUS #291 | 0.000 | esc_status + esc_wheel_odometry | 3s observation ; 1s lame si source COMMON | 0 tant que support FCU non validé ; 10 si source validée |
| ESC_INFO #290 | 0.000 | esc_status + esc_wheel_odometry (métadonnées) | 3s métadonnées ; ne rafraîchit jamais mesure RPM | 0 tant que support non validé ; 1–2 ensuite |
| VIBRATION #241 | 1.000 | plugin vibration exclu ; seulement flux brut/GCS | aucune exigence bridge | 1 conserver diagnostics ; ne pas augmenter |
| EKF_STATUS_REPORT #193 | 1.000 | aucun handler actif identifié dans source MAVROS épinglée | pas confondre avec ESTIMATOR_STATUS#230 | 1 conserver diagnostic GCS |
| DISTANCE_SENSOR #132 | 0.000 | distance_sensor → rangefinder_pub | pas de TTL blade ; dépend d’un vrai capteur configuré | 0 absent ; 10–20 si capteur/consumer validés |
| WHEEL_DISTANCE #9000 | 0.000 | esc_wheel_odometry ; plugin wheel_odometry upstream exclu | 3s, time_us/component/source distincts | 0 absent ; 20 si source encodeur validée |
| RAW_IMU #27 | 1.000 | imu → data_raw/mag/accélération utilisée avec ATTITUDE | readiness5s via imu/data ; pas de TTL cache accélération plugin | 50 avec ATTITUDE pour données cohérentes |
| BUTTON_CHANGE #257 | 0.000 | bridge brut → wheel lift AP_Button | pas de TTL d’invalidation automatique ; durée/âge exposés | événement : cadence/absence à qualifier, aucun intervalle aveugle |
| SYSTEM_TIME #2 | 1.000 | sys_time + universal_gnss | pas de garde lame ; référence horloge | 1 conserver |
| TIMESYNC #111 | 10.097 | sys_time échange bidirectionnel | échange, pas mesure physique ESC ; seuils statistiques time plugin | 10 existant ; pas SET_MESSAGE_INTERVAL pour augmenter replies |
| MISSION_CURRENT #42 | 1.000 | waypoint → /mavros/mission | pas de TTL blade ; protocoles mission séparés | 1 conserver |
| VFR_HUD #74 | 1.000 | vfr_hud | pas de TTL blade | 1 conserver |
| MEMINFO #152 | 1.000 | sys_status diagnostics | pas de TTL blade | 1 conserver |

Recommandation de méthode : après validation opérateur, un propriétaire unique
applique **MAV_CMD_SET_MESSAGE_INTERVAL511 par message** et vérifie ACK,
fréquence réellement obtenue, gaps, compteur/stamps de source nouveaux et
charge du lien. Ne pas changer globalement MAV1_EXTRA3 pour accélérer ESC :
cela augmenterait simultanément plusieurs messages inutilisés. Ne pas utiliser
SRx/MAVx comme alias d'un intervalle individuel. Les groupes peuvent rester
baselineboot/fallback seulement si cette politique est explicitement décidée.

Prioritéproposée : #11030 à10Hz (intervalle100000µs) pour ALL ESC0/1/2,
#36 à10Hz, #1 à5Hz ; #30 et #27 à50Hz pour IMU/tilt ; #226 à20Hz si cette
source roue demeure autoritaire. #147 à2Hz total à vérifier **par instance**
(ArduPilot alterne batteries ; multiplier le groupe ne vaut pas preuve de1Hz
par batterie). Aucun de ces intervalles n’a été envoyé dans cet audit.

Le service MAVROS existant de configuration intervalle utilise une attente1s,
tandis que commandLong plugin attend5s : une réponse service false ne doit
pas être interprétée automatiquement comme refusFCU sans ACK associé.
Ne pas lancer de commandes pour tester cet aspect dans cet audit.

## Annexe — entrées de tous les plugins réellement chargés

Registre entrant dérivé des handlers du commit MAVROS installé et des plugins
custom, complété des handlers MissionBase hérités. Pour geofence/rallypoint,
MISSION_CURRENT/ITEM_REACHED ne sont pas enregistrés : réservéswaypoint.
mowgli_gnss écoute le GPS sélectionné (GPS1 ici), pas les deux simultanément.
Les branches de parsing non pertinentes ne sont pas activées par cet audit.
Les plugins sans handler entrant ne demandent pas par leur seule présence une
fréquence FCU. Le CSV donne N/Hz/period/gaps pour89types comprenant ces inputs.

| Plugin actif | Messages entrants enregistrés (zéro reçu si absent du tableau ci-dessus) |
| --- | --- |
| adsb | ADSB_VEHICLE |
| battery_observer | BATTERY_STATUS |
| cam_imu_sync | CAMERA_TRIGGER |
| camera | CAMERA_IMAGE_CAPTURED |
| cellular_status | Pas de handler MAVLink entrant ; rôle émission/ROS |
| command | COMMAND_ACK |
| companion_process_status | Pas de handler MAVLink entrant ; rôle émission/ROS |
| distance_sensor | DISTANCE_SENSOR |
| esc_status | ESC_INFO, ESC_STATUS |
| esc_telemetry | ESC_TELEMETRY_1_TO_4, ESC_TELEMETRY_5_TO_8, ESC_TELEMETRY_9_TO_12 |
| esc_wheel_odometry | ESC_INFO, ESC_STATUS, ESC_TELEMETRY_1_TO_4, ESC_TELEMETRY_5_TO_8, ESC_TELEMETRY_9_TO_12, RPM, WHEEL_DISTANCE |
| fake_gps | Pas de handler MAVLink entrant ; rôle émission/ROS |
| geofence | MISSION_ACK, MISSION_COUNT, MISSION_REQUEST, MISSION_REQUEST_INT, MISSION_ITEM, MISSION_ITEM_INT |
| gimbal_control | GIMBAL_DEVICE_ATTITUDE_STATUS, GIMBAL_DEVICE_INFORMATION, GIMBAL_MANAGER_INFORMATION, GIMBAL_MANAGER_STATUS |
| global_position | GLOBAL_POSITION_INT, GPS_GLOBAL_ORIGIN, GPS_RAW_INT, LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET |
| gps_input | Pas de handler MAVLink entrant ; rôle émission/ROS |
| gps_rtk | GPS_RTK |
| gps_status | GPS2_RAW, GPS2_RTK, GPS_RAW_INT, GPS_RTK |
| guided_target | POSITION_TARGET_GLOBAL_INT |
| home_position | HOME_POSITION |
| imu | ATTITUDE, ATTITUDE_QUATERNION, HIGHRES_IMU, RAW_IMU, SCALED_IMU, SCALED_PRESSURE |
| landing_target | LANDING_TARGET |
| local_position | LOCAL_POSITION_NED, LOCAL_POSITION_NED_COV |
| log_transfer | LOG_DATA, LOG_ENTRY |
| mag_calibration_status | MAG_CAL_PROGRESS, MAG_CAL_REPORT |
| manual_control | MANUAL_CONTROL |
| mocap_pose_estimate | Pas de handler MAVLink entrant ; rôle émission/ROS |
| mowgli_gnss | GPS2_RAW, GPS_RAW_INT |
| nav_controller_output | NAV_CONTROLLER_OUTPUT |
| obstacle_distance | Pas de handler MAVLink entrant ; rôle émission/ROS |
| obstacle_distance_3d | Pas de handler MAVLink entrant ; rôle émission/ROS |
| odometry | ODOMETRY |
| onboard_computer_status | Pas de handler MAVLink entrant ; rôle émission/ROS |
| open_drone_id | Pas de handler MAVLink entrant ; rôle émission/ROS |
| optical_flow | OPTICAL_FLOW |
| param | PARAM_VALUE |
| play_tune | Pas de handler MAVLink entrant ; rôle émission/ROS |
| rallypoint | MISSION_ACK, MISSION_COUNT, MISSION_REQUEST, MISSION_REQUEST_INT, MISSION_ITEM, MISSION_ITEM_INT |
| rangefinder | RANGEFINDER |
| rc_io | RC_CHANNELS, RC_CHANNELS_RAW, SERVO_OUTPUT_RAW |
| setpoint_accel | Pas de handler MAVLink entrant ; rôle émission/ROS |
| setpoint_attitude | Pas de handler MAVLink entrant ; rôle émission/ROS |
| setpoint_position | Pas de handler MAVLink entrant ; rôle émission/ROS |
| setpoint_raw | ATTITUDE_TARGET, POSITION_TARGET_GLOBAL_INT, POSITION_TARGET_LOCAL_NED |
| setpoint_trajectory | Pas de handler MAVLink entrant ; rôle émission/ROS |
| setpoint_velocity | Pas de handler MAVLink entrant ; rôle émission/ROS |
| sim_state | SIM_STATE |
| sys_status | AUTOPILOT_VERSION, BATTERY_STATUS, ESTIMATOR_STATUS, EVENT, EXTENDED_SYS_STATE, HEARTBEAT, HWSTATUS, MEMINFO, STATUSTEXT, SYS_STATUS |
| sys_time | SYSTEM_TIME, TIMESYNC |
| tdr_radio | RADIO, RADIO_STATUS |
| terrain | TERRAIN_CHECK, TERRAIN_REPORT, TERRAIN_REQUEST |
| trajectory | TRAJECTORY_REPRESENTATION_WAYPOINTS |
| tunnel | TUNNEL |
| universal_gnss | GPS2_RAW, GPS2_RTK, GPS_RAW_INT, GPS_RTK, SYSTEM_TIME |
| vfr_hud | VFR_HUD |
| vision_pose | Pas de handler MAVLink entrant ; rôle émission/ROS |
| vision_speed | Pas de handler MAVLink entrant ; rôle émission/ROS |
| waypoint | MISSION_ACK, MISSION_COUNT, MISSION_CURRENT, MISSION_ITEM_REACHED, MISSION_REQUEST, MISSION_REQUEST_INT, MISSION_ITEM, MISSION_ITEM_INT |
| wind_estimation | WIND, WIND_COV |

## Clôture et suite exacte

Aucune écriture FCU, aucun511/66/ARM/servo/CAN_FORWARD émis par l’auditeur,
aucun restart/build/déploiement/VESC/GUI/commit/push. Diagnostics distincts
terminés exit0 ; produit788d5831… resté actif au StartedAt07:39:58 inchangé.
Pendant les fenêtres, FCU connecté/désarmé/MANUAL, sorties1500/1500/1500.
Les essais de rotation ne sont PAS repris. Repo main9f62ba4b inchangé côtécode,
doc/CSV/JSON/checkpoint seulement ; worktree produit préexistant conservé.

Audit PASS pour réception/provenance/config/seuils sur la baseline décrite.
Startup511 historique non capturé : limite explicite, pas une absence prouvée.
Prochaine étape uniquement après choix opérateur : plan d'intervalles individuels
avec rollback des intervalles transitoires et validation PASSIVE avant moteurs,
puis corrélation du refus FWD si nécessaire. Aucune prescription de tondre ni
qualification failsafe totalcompanion n'en découle.

