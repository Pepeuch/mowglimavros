# Audit des fréquences MAVLink — historique et contrôle après optimisation

## Preuves historiques — 9 octobre 2026

Le rapport demandé et ses CSV/JSON ne sont plus présents dans ce checkout
MowgliNext feat/mavros-refresh@582e6fe96eabaa45d5381fc7f27dbdab7f62ed22,
MowgliMAVROS main@9f62ba4b1afbccbdaa8b22899ecd4874bc28b204 (initialement propre).
Cette page est donc **une reconstruction documentée des mesures historiques**,
suivie du nouveau contrôle ; elle n'est pas présentée comme le texte original
retrouvé. Aucun rapport existant ni log historique n'a été écrasé.

Les événements JSON exacts ont été récupérés, en lecture seule, depuis les
conteneurs conservés sur le robot :
- mowgli-mavlink-passive-rate-audit-20261009-r2 :60.020539788s ;
- mowgli-mavlink-passive-rate-audit-20261009-instances :30.019915084s.

Ils restent intacts dans
[mavlink-frequency-proof-20261009-recovered.json](mavlink-frequency-proof-20261009-recovered.json).
Le [CSV historique récupéré](mavlink-frequency-metrics-20261009-recovered.csv)
est dérivé de ces données, pas déclaré identique à l'ancien CSV absent.
Le script ancien reste disponible sans modification sous
/tmp/mavlink-passive-rates-20261009.py et dans le répertoire diagnostic robot.

Résumé historique confirmé par ces preuves : HEARTBEAT, SYS_STATUS, ATTITUDE,
SERVO_OUTPUT_RAW, RPM226 et ESC_TELEMETRY11030 étaient chacun à environ1Hz ;
BATTERY_STATUS1Hz global soit0.5Hz par batterie0/1. EscObservation délivrait3Hz
par ESC mais seulement1Hz d'acquisitions distinctes. Gap ESC2 distinct1.024259s,
sans marge vis-à-vis de la cible1000ms de BladeControl. Le premier essai utilisant
KEEP_LAST5 était sous-compté : seul l'essai4096 est utilisé comme référence.

Baseline historique : image788d5831… et bridge47527d46… du patch lame,
StartedAt2026-10-09T07:39:58.379363269Z. Firmware ArduRover4.7.1dbe79216,
Pixhawk5X/Rock5B. Les essais moteur/FWD, défauts de politique et calibration
ne sont pas réévalués par le présent audit.

## Après optimisation — 10 octobre 2026

### Scope et baseline réellement inspecté

Audit strictement passif : aucune écritureFCU, commande511/stream-rate,
ARM/DISARM/servo émise par l'auditeur, activation moteur/lame, modificationVESC,
build/déploiement ou restart production. Aucun commit/push. Les seules
modifications sont cette page, données de preuve et scripts d'audit.

Robot SSH pepeuchUID1000, hostname rock-5b, PWD/home/pepeuch,
Armbian/Debian sur noyau7.1.8-edge-rockchip64/aarch64.
Liaison USB ttyACM0 :
serial:///dev/serial/by-id/usb-Holybro_Pixhawk5X_2F002F001050425937353320-if00:921600.
Firmware4.7.1dbe79216 confirmé par log AUTOPILOT_VERSION, SDKMAVLink2026.9.9.

**Changement de baseline non limité aux fréquences :**
- image actuelle sha256:cfb4c921cc2ea23cf05b45843b8941af7a838ddf18a02091eeacc0dd460acf07 ;
- bridge actuel SHA25613f26d44d1d08465bb5d7ccefe271a9db6f82a602bf24cc3384c21c9ba643f2b ;
- démarrage sidecar2026-10-10T12:43:52.639646551Z, inchangé pendant notre audit ;
- ce n'est **pas** le bridge OFF/FWD/REV du9octobre. Les paramètres blade_forward/
  reverse/neutral et BladeControl ne sont pas dans ce binaire ; la source clean
  actuelle on_mower_control appelle encore send_arm_command(blade_authorized).
- Les seuils1s acquisitionlame et3s Safety du patch précédent sont donc
  évalués ci-dessous comme **cibles attendues**, pas revendiqués comme des
  protections actives du binaire actuel. Le trackerESC réellement actif a3s ;
  readiness5s et connexionMAVROS10s. Aucune correction de ce drift n'a été faite.

Paramètres lus dans le cache après la mesure, sans ParamPull :
SERIAL0_PROTOCOL2/BAUD921 ; MAV1_RAW_SENS10, EXT_STAT5, RC_CHAN5,
RAW_CTRL2, POSITION10, EXTRA1=20, EXTRA2=1, EXTRA3=10, PARAMS10, ADSB0,
OPTIONS0. MAV2 reste à1Hz pour les groupes usuels. Ce sont les paramètres
actuels, pas des demandes émises par l'auditeur. La cadence mesurée par message
fait foi : ne pas déduire mécaniquement RPM/ESC20Hz de la seule valeur EXTRA3.

ConfigurationROS effective : sourceauto, rôlesright0/left1/blade2,
timeoutobservations3s, ticks_per_meter343, track_width0.325,
readiness5s, battery5s, tilt_emergency_ms500 et imu_inclination_threshold56.
Le diagnostic réel confirme active_source=ardupilot_legacy,
wheel_tick_source=ardupilot_legacy, validité gauche/droite true.
Voir [source-diagnostics](mavlink-source-diagnostics-20261010.json).
rpm_metric_calibrated=true signifie uniquement que le paramètre343 est positif :
**aucune validation métrique physique n'est acquise par cet indicateur**.

### Protocole et qualité des mesures

Script [mavlink-passive-rates-20261010.py](mavlink-passive-rates-20261010.py),
adapté du script initial sans changer les producteurs. Abonnements best-effort
MAVLink depth4096, clock monotone callback, warmup4s, sources sysid/compid
séparées, framingOK, séquences contrôlées. Aucune publication, aucun client
de commande FCU ; lectures ROS List/GetParameters uniquement après capture.

- Capture principale60.000154741s : 2026-10-10T13:17:30.238Z
  →2026-10-10T13:18:30.271Z (UTC ;15:17:30→15:18:30 Paris).
- Vérification30.006545305s :
  2026-10-10T13:20:11.267Z→2026-10-10T13:20:41.291Z.
- Diagnostics indépendants mowgli-mavlink-rate-audit-20261010-60s et…-30s,
  exit0. Aucun processus produit redémarré.
- Framing invalide0, aucun saut de séquence observé dans les deux fenêtres.
  Contrôle modulo256 et fluxROS best-effort : pas une preuve absolue de zéro
  perte électrique ou de comportement sous toute charge.
- FCU connecté=true/armed=false/MANUAL, sorties1500/1500/1500 pendant les
  deux fenêtres ; aucune transition détectée, tous les RPM observés zéro.
- Hz=N/durée complète ; mean/min/max/std/p95/p99 sur les inter-arrivées,
  percentiles interpolés. Le gap de comparaison est le pire des deux fenêtres.
  Ce sont des latences de réception, pas des bornes temps-réel garanties.
- Trames MAVLink distinctes et acquisitions ESC source/count/stamp distinctes
  séparées des republications ROS. Le compteurESC1 a passé son rollover65535
  pendant la seconde fenêtre ; source/stamps/validité restent cohérents.
- RPM226 n'a pas de timestamp/counter de mesure physique embarqué : on mesure
  des trames distinctes par séquence, pas une prétendue nouvelle acquisition
  capteur prouvée par changement de la valeur RPM.

Données agrégées :
[preuve60s+30s et paramètres](mavlink-frequency-proof-20261010.json),
[CSV fréquences/jitter](mavlink-frequency-metrics-20261010.csv).

### Synthèse avant/après, exigences et marges

La marge est seuil−gap maximal observé, et non une garantie en conditions futures.
Pour tilt500ms, la colonne est une marge de **cadence**, pas la marge d'un
deadline arrêtphysique500ms (voir analyse ci-dessous).

| Flux | Avant Hz | Après Hz (60s) | Plus grand gap (60s+30s) | Exigence/cible | Marge observée | Verdict |
| --- | ---: | ---: | ---: | --- | ---: | --- |
| ESC0 distinct | 1 | 20.000 | 97.89ms | 1s | 902.11ms | PASS cadence (cible lame) |
| ESC1 distinct | 1 | 20.000 | 97.35ms | 1s | 902.65ms | PASS cadence (cible lame) |
| ESC2 distinct | 1 | 20.000 | 97.35ms | 1s | 902.65ms | PASS cadence (cible lame) |
| RPM #226 | 1 | 20.000 | 81.33ms | 3s | 2918.67ms | PASS fréquence ; signe physique hors essai |
| SYS_STATUS | 1 | 5.000 | 218.23ms | 3s | 2781.77ms | PASS cadence (cible Safety) |
| ATTITUDE → /imu/data | 1 | 20.000 | 81.70ms | 0.5s | 418.30ms | PASS cadence ; debounce ≠ délai absolu |
| SERVO_OUTPUT_RAW | 1 | 5.000 | 221.48ms | Pas de TTL dédié | Non définie | PASS cadence ; pas de TTL spécifique |
| Batterie ID0 | 0.5 | 5.000 | 223.90ms | 5s | 4776.10ms | PASS |
| Batterie ID1 | 0.5 | 5.000 | 223.79ms | 5s | 4776.21ms | PASS |
| /wheel_odom | non configuré le9 | 20.000 | 79.14ms | 3s | 2920.86ms | PASS flux ; WARN calibration |
| /wheel_ticks | non mesuré le9 | 20.000 | 79.85ms | 3s | 2920.15ms | PASS flux |

ESC_TELEMETRY11030 brut1200 messages/60s :19.99995Hz, gap95.485ms.
Pour chacun des ESC0/1/2, les observations ROS sont à21.99994Hz mais les
acquisitions distinctes à19.99995Hz. Les2Hz supplémentaires sont des
republications ; elles ne renouvellent pas la preuve de fraîcheur.
Gaps distincts98/97/97ms environ, largement sous la cible1s.
Leur source normalisée estlegacy2 ; count_valid/rpm_valid restent vrais et
les compteurs progressent. Source, validité, stamp et count viennent du même
message accepté, pas du PWM.

BATTERY_STATUS600 messages/60s :9.99997Hz global ; IDs0 et1 chacun300 messages,
4.99999Hz et gaps223.904/223.785ms. Vérification30s IDs0/1 :151/150 échantillons,
5.032/4.999Hz (effet bord de fenêtre), gaps222.911/220.454ms. Les deux batteries
ont donc environ4.776s de marge vis-à-vis5s ; la cadence globale n'est pas
utilisée comme substitut de leur identité.

### ATTITUDE20Hz et tilt500ms : suffisant sans imposer50Hz

Source on_mavros_imu actuellement déployée : mémorise le premier callback
incliné, puis active le tilt lorsque une observation toujours inclinée arrive
au moins500ms plus tard. **500ms est une persistance/debounce**, pas un deadline
« inclinaison physique→moteur arrêté en500ms ».

ATTITUDE20Hz et imu/data20Hz sont cohérents ; période moyenne50ms, pire gap
ATTITUDE81.700ms, pire gap du topicIMU80.372ms sur60s (68.409ms sur30s).
Une fenêtre500ms contient typiquement10 observations. **PASS cadence pour ce
consommateur ; aucune justification de passer à50Hz dans cet audit.**

La quantification de l'instant d'apparition et celle de la première observation
après500ms ajoutent au plus deux gaps dans un modèle idéal d'échantillonnage :
avec81.7ms observés, ordre de grandeur500+2×81.7≈663.4ms avant décision ROS,
sans inclure latencecapteur/FCU/ACK ou mécanique. Ce calcul n'est PAS une borne
certifiée, seulement l'explication du contrat. Si500ms est exigé comme délai
absolu d'arrêt physique, ce contrat reste WARN/HARDWARE_REQUIRED et augmenter
la cadence seul ne constitue pas une preuve suffisante. Aucun tilt ou arrêt
de moteur n'a été testé ici. Pas de watchdog IMU moteur nouveau introduit.

### SERVO_OUTPUT_RAW5Hz : pas de nécessité prouvée de10Hz

5Hz vérifiés sur#36 et/mavros/rc/out, gap pire221.477ms.
Consumer rc/out du binaire courant n'est pas une condition d'autorisation
BladeControl (celui-ci n'est pas chargé). Dans le patch précédent, le retour
de sortie servait à invalider un cache lors d'un changementPWM/stampdistinct,
pas à prouver RPMzéro ; aucun TTL100ms explicitement imposé par cette voie.

PASS cadence de télémétrie/cache sur la baseline. **Aucun réglage10Hz
supplémentaire nécessaire démontré** ;10Hz resterait un objectif optionnel de
réactivité/supervision, à décider seulement avec un besoin mesurable après
réintégration du vrai consumer. Ne pas accélérer MAV1_RC_CHAN et tous ses
messages pour satisfaire une cible non requise.

### RPM signés et odométrie

RPM2261200 trames/60s :20Hz, gap81.335ms. Le chemin inspecté
handle_rpm→legacy_rpm→MotorTickIntegrator garde les floats signés sansabs,
mappingrightRPM1/ESC0,leftRPM2/ESC1 établi dans le checkpoint hardware
MM-ESC-ODOMETRY-20261005. Aucun sens déduit du PWM.

PASS disponibilité/cadence/fraîcheur des tramesRPM et des wheel ticks/odom.
Au repos, rpm1/rpm2 sont zéro et finis sur les deux captures : ce contrôle
passif **ne revalide pas un signe physique positif/négatif**. Le legacy11030
reste une magnitudeunsigned et EscObservation.rpm_direction_valid=false :
aucune correction du transport signé ESC2 n'a été faite ni déduite de ces taux.

wheel_odom etwheel_ticks20Hz, gaps79.136/79.853ms sur60s, sourceactive
ardupilot_legacy réellement observée. Calibration343 ettrackwidth0.325 lues,
non modifiées. Les ticks cumulés des diagnostics peuvent être nonzéro
depuis des essais antérieurs : leur existence ne prouve ni mouvement présent
ni échelle métrique correcte. Calibration métrique : WARN/HARDWARE_REQUIRED,
RTK/distance connue nécessaire dans une mission ultérieure autorisée.

### Points résiduels à ne pas masquer par une hausse de fréquence

1. **Drift de déploiement, WARN** : le binaire actuel est revenu au baseline
  13f26d44…/imagecfb4c921…, pas au patchOFF/FWD/REV. Les gardes1000ms lame
   et3000ms Safety sont des cibles testées en marge, non des garde-fous actuels.
2. **DISARM répétitif, WARN** :600 COMMAND_LONG400,param1=0 reçus sur le sink
   pendant60s et300 pendant30s, soit10Hz. AucunARMtrue,183,511,66 ou32000
   observé par ces captures. Ces requêtes sont de la pile préexistante :
   le script auditeur ne crée aucun publisher/client de commande.
   Le code on_mower_control courant appelle send_arm_command ; cela est
   cohérent avec le flux, mais le caller exact n'a pas été identifié par une
   trace de services. Ne pas prétendre qu'une nouvelle cadence résout ce contrat.
3. **Signe ESC2 / métrique odométrique / sécurité physique**, WARN et hors clôture
   fréquence : pas de nouvel essai, aucun mouvement autorisé par cette mission.
4. **Silence futur ou sourcefigée** : aucun gap dépassant les cibles critiques
   ni perte séquence observée ici ; la mesurestationnaire90s ne valide pas
   les reconnections, gels, pertecompanion ou la charge d'un essai moteur.

### Fréquences et jitter de tous les messages reçus —60s

Les types absents ont N0 ; on ne conclut pas à une panne lorsqu'un message
événementiel ou sans capteur actif n'apparaît pas. ESC_STATUS291/ESC_INFO290/
WHEEL_DISTANCE9000/DISTANCE_SENSOR132 restent non observés ; ne pas les
activer sans support/provenance/consumer qualifiés. Le CSV garde la précision
complète et les gaps de stamps backend.

| Message | N | Hz | Moyenne / min / max ms | Écart-type ms | p95 / p99 ms |
| --- | ---: | ---: | --- | ---: | --- |
| HEARTBEAT #0 | 60 | 1.000 | 999.991 / 998.532 / 1001.859 | 0.723 | 1001.079 / 1001.663 |
| SYS_STATUS #1 | 300 | 5.000 | 199.970 / 182.665 / 217.348 | 7.303 | 213.491 / 215.422 |
| SYSTEM_TIME #2 | 600 | 10.000 | 100.005 / 70.565 / 121.662 | 7.995 | 115.347 / 117.676 |
| PARAM_VALUE #22 | 14 | 0.233 | 4230.781 / 0.539 / 10999.447 | 5085.251 | 10999.404 / 10999.438 |
| GPS_RAW_INT #24 | 300 | 5.000 | 199.957 / 182.841 / 215.991 | 4.990 | 206.889 / 214.376 |
| RAW_IMU #27 | 599 | 9.983 | 100.026 / 76.330 / 124.475 | 11.030 | 116.466 / 117.895 |
| SCALED_PRESSURE #29 | 600 | 10.000 | 100.006 / 75.786 / 125.277 | 10.235 | 116.160 / 118.191 |
| ATTITUDE #30 | 1200 | 20.000 | 50.004 / 22.488 / 81.700 | 11.223 | 64.803 / 67.517 |
| LOCAL_POSITION_NED #32 | 600 | 10.000 | 100.030 / 65.088 / 136.634 | 9.589 | 118.205 / 120.074 |
| GLOBAL_POSITION_INT #33 | 599 | 9.983 | 100.027 / 78.090 / 122.009 | 11.134 | 116.681 / 118.047 |
| SERVO_OUTPUT_RAW #36 | 300 | 5.000 | 199.962 / 182.816 / 221.477 | 6.838 | 213.143 / 215.148 |
| MISSION_CURRENT #42 | 300 | 5.000 | 199.966 / 183.073 / 220.244 | 7.258 | 213.357 / 215.330 |
| RC_CHANNELS #65 | 300 | 5.000 | 199.959 / 182.855 / 217.147 | 5.737 | 211.344 / 215.051 |
| VFR_HUD #74 | 60 | 1.000 | 999.907 / 979.935 / 1023.929 | 8.615 | 1018.173 / 1021.826 |
| COMMAND_ACK #77 | 600 | 10.000 | 99.634 / 13.945 / 342.717 | 76.182 | 301.031 / 323.542 |
| TIMESYNC #111 | 606 | 10.100 | 98.673 / 0.565 / 323.609 | 72.577 | 300.599 / 303.699 |
| SCALED_IMU2 #116 | 599 | 9.983 | 100.026 / 75.713 / 125.170 | 10.732 | 116.275 / 118.154 |
| GPS2_RAW #124 | 300 | 5.000 | 199.986 / 181.653 / 220.167 | 5.049 | 206.498 / 214.897 |
| POWER_STATUS #125 | 300 | 5.000 | 199.966 / 183.028 / 218.418 | 7.324 | 213.565 / 215.447 |
| SCALED_IMU3 #129 | 599 | 9.983 | 100.026 / 75.823 / 125.268 | 10.457 | 116.195 / 118.178 |
| SCALED_PRESSURE2 #137 | 600 | 10.000 | 100.005 / 75.775 / 121.638 | 9.231 | 115.585 / 117.548 |
| BATTERY_STATUS #147 | 600 | 10.000 | 100.031 / 75.833 / 123.561 | 12.111 | 119.589 / 121.574 |
| MEMINFO #152 | 300 | 5.000 | 199.966 / 182.960 / 219.490 | 7.379 | 213.404 / 215.302 |
| AHRS #163 | 600 | 10.000 | 100.025 / 78.207 / 121.146 | 11.159 | 116.742 / 117.888 |
| AHRS2 #178 | 1200 | 20.000 | 50.004 / 22.215 / 81.368 | 11.068 | 64.552 / 66.460 |
| EKF_STATUS_REPORT #193 | 600 | 10.000 | 100.005 / 60.949 / 136.347 | 8.340 | 117.106 / 120.249 |
| RPM #226 | 1200 | 20.000 | 50.004 / 22.970 / 81.335 | 11.402 | 65.138 / 69.205 |
| VIBRATION #241 | 600 | 10.000 | 100.030 / 76.048 / 123.315 | 11.160 | 119.197 / 121.123 |
| ESC_TELEMETRY_1_TO_4 #11030 | 1200 | 20.000 | 50.004 / 23.605 / 95.485 | 11.575 | 65.447 / 70.401 |

### Conclusion de clôture

**PASS — le constat de fréquences insuffisantes de l'audit du9octobre est
résolu pour les flux critiques mesurés**, sur CE firmware/robot/image/config.
Les observations et identités sont fraîches avec une marge suffisante :
aucun réglage de fréquence supplémentaire nécessaire démontré,
ni50Hz ATTITUDE ni10Hz SERVO_OUTPUT_RAW imposés.

**La clôture globale sans réserve de l'acceptation MAVROS/lame n'est pas
possible** : drift de binaire etDISARMspam sont des points d'architecture/
déploiement distincts, auxquels s'ajoutent les acceptations physique/métrique
horsscope. L'audit de cadence peut être retenu/clôturé sur sa baseline ;
ces WARN restent explicitement ouverts et ne sont pas transformés enFAIL
de fréquence. Aucune correction ni nouveau déploiement effectué.

Checkpoint RETAINED dans cette page pour respecter le scope limité aux
rapports/données/scripts. La référence historique de9octobre reste dans ses
logs originaux et les fichiersrecovered ; ceux-ci n'ont pas été remplacés
par les nouvelles mesures. Aucun fichier source/contratROS/commande moteur,
paramètreFCU ou configurationVESC modifié. Aucun commit/push.

Suite après décisionopérateur seulement : traiter le drift/ownership des
commandes, puis refaire l'acceptation du vrai binaire si déployé ; ne pas
reprendre un essai moteur àpartir du seulPASS de ces fréquences.

