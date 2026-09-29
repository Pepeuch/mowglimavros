# Audit matériel / MAVROS / ArduPilot — MowgliNext
Date: 2026-09-22

**Mise à jour du 2026-09-28 :** l’association historique « roue droite → ESC2 / `esc_index=1` » de ce relevé est contredite par une nouvelle rotation manuelle contrôlée de la roue droite seule : seul ESC1 / `esc_index=0` a alors produit des RPM. Utiliser la [capture actuelle](.agent/shared/checkpoints/blocked/MM-MANUAL-CONTROL-ROUTING-20260928.md) pour l’état présent ; la cause du changement reste inconnue. Une rotation comparable de la roue gauche n’a produit aucun RPM détectable. Les lignes ci-dessous conservent la provenance du 22 septembre.

## Objet
Consolidation des observations faites sur le robot équipé d’un Pixhawk 5X, d’un Rock 5B, de MowgliMAVROS, de trois VESC DroneCAN, de deux entrées POWER et d’un u-blox F9P sur GPS2.

Statuts utilisés :
- **PROUVÉ** : observé directement en runtime, sur le bus, dans MAVLink ou dans le code.
- **FORTEMENT INFÉRÉ** : cohérent par recoupement de configuration mais pas encore confirmé physiquement.
- **À PROUVER / À CONFIGURER** : information manquante ou sous-système non finalisé.

## 1. Architecture observée

- SBC : Rock 5B, ARM64.
- FCU : Holybro Pixhawk 5X.
- ArduPilot : ArduRover 4.6.3.
- Liaison Rock 5B ↔ Pixhawk : USB / MAVLink à 921600 bauds.
- MAVROS : sidecar ROS 2 Kilted.
- RMW : CycloneDDS.

Topologie physique déclarée :
- CAN1 : VESC roue gauche, VESC roue droite, VESC moteur de tonte.
- GPS2 : u-blox F9P série.
- POWER1 : chargeur / station.
- POWER2 : batterie traction 7S.

## 2. État global FCU / MAVROS — PROUVÉ

`/mavros/state` :
- `connected=true`
- `armed=false`
- mode `MANUAL`
- `system_status=4`

`mavros_node` et `mavros_hardware_bridge_node` démarrent et restent actifs.
Mission Planner reçoit le flux MAVLink via UDP depuis le Rock 5B.

## 3. Correctif CycloneDDS — PROUVÉ

Symptôme initial :

```text
PruneDelay: unknown attribute
RCLError: failed to initialize rcl node
```

Cause : CycloneDDS 0.10.5 de l’image ARM64 ne supportait pas :

```xml
<Peer Address="localhost" PruneDelay="inf"/>
```

Correctif validé : suppression de `PruneDelay="inf"` dans :
- `docker/config/cyclonedds.xml`
- `install/config/cyclonedds.xml`

Résultat : sidecar stable, port Pixhawk ouvert, heartbeat ArduPilot reçu.

## 4. Double `hardware_bridge` — PROUVÉ

Deux nœuds `/hardware_bridge` ont été observés :
1. bridge legacy STM32 dans `mowgli-ros2`
2. bridge MAVROS dans `mowgli-mavros`

Conséquences :
- deux abonnements hardware à `/cmd_vel`
- deux publishers sur `/imu/data`, `/hardware_bridge/status`, `/hardware_bridge/power`, `/battery_state`

Cause : image `mowgli-ros2` déployée plus ancienne que `feat/mavros-refresh`, sans la condition `hardware_backend`.

À corriger plus tard par rebuild/re-déploiement de `mowgli-ros2` ou backport ciblé.

## 5. Batteries / POWER — PROUVÉ

Configuration :

```text
BATT_MONITOR=21
BATT_I2C_BUS=1
BATT_I2C_ADDR=69
BATT2_MONITOR=21
BATT2_I2C_BUS=2
BATT2_I2C_ADDR=69
```

Mapping établi :

```text
BATTERY_STATUS id0 = BATT_  = POWER1 = chargeur / station
BATTERY_STATUS id1 = BATT2_ = POWER2 = batterie traction 7S
```

Reste à valider : convention de signe du courant et logique charge/décharge.

## 6. CAN1 / DroneCAN / VESC — PROUVÉ

Configuration ArduPilot CAN1 :

```text
CAN_P1_DRIVER = 1
CAN_P1_BITRATE = 500000
CAN_D1_PROTOCOL = DroneCAN
CAN_D1_UC_ESC_BM = 7
CAN_D1_UC_ESC_OF = 0
CAN_D1_UC_ESC_RV = 7
CAN_D1_UC_NODE = 10
CAN_D1_UC_OPTION = 1924
```

`CAN_D1_UC_ESC_BM=7` active les trois premiers canaux ESC.

Mission Planner DroneCAN Inspector :
- node 1 = `org.vesc.60`
- node 2 = `org.vesc.60`
- node 3 = `org.vesc.60`
- tous `OPERATIONAL / OK`
- chacun publie `uavcan_equipment_esc_Status` autour de 49 Hz

Mapping DroneCAN observé :

```text
node 1 -> esc_index 0
node 2 -> esc_index 1
node 3 -> esc_index 2
```

À l’arrêt : courant ~0 A, RPM 0, température ~302–303 K, tension ~27 V.

Conclusion : CAN1, DroneCAN et les trois VESC fonctionnent.

## 7. MAVLink ESC telemetry — PROUVÉ

ArduPilot émet `ESC_TELEMETRY_1_TO_4` (ID 11030) à environ 2 Hz.

Capture 15 s :
- 29 paquets `ESC_TELEMETRY_1_TO_4`
- 0 `ESC_TELEMETRY_5_TO_8`
- 0 `ESC_STATUS`
- 0 `ESC_INFO`
- ~30 messages `RPM` #226

MAVROS publie correctement :

```text
/mavros/esc_telemetry/telemetry
```

Exemple :
- ESC1 ~30 °C / ~26.18 V
- ESC2 ~29 °C / ~26.03 V
- ESC3 ~31 °C / ~26.28 V
- ESC4 nul

Les topics `/mavros/esc_status/status` et `/mavros/esc_status/info` restent vides parce qu’ArduPilot n’émet pas `ESC_STATUS` ni `ESC_INFO` dans cette configuration.

## 8. Correspondance des slots ESC — PROUVÉ

Le tableau ROS est 0-based :

| Tableau ROS | ESC MAVLink / Mission Planner | `esc_index` |
|---|---:|---:|
| `esc_telemetry[0]` | ESC1 | 0 |
| `esc_telemetry[1]` | ESC2 | 1 |
| `esc_telemetry[2]` | ESC3 | 2 |

Preuve issue de l’inspection du plugin : l’index interne commence à 0, et `ESC_TELEMETRY_1_TO_4` utilise `group_offset=0`.

## 9. Test physique interactif — PROUVÉ partiellement

Observation :
- roue droite → `esc_telemetry[1]`
- RPM positifs observés environ 471–664 RPM pendant la rotation manuelle

Donc :

```text
roue droite -> ESC2 -> esc_index 1 -> esc_telemetry[1]
```

Encore à identifier :
- roue gauche -> ESC1 ou ESC3
- tonte -> ESC1 ou ESC3

Les premières fenêtres roue gauche / tonte n’ont pas produit de RPM non nul sur la surface observée.

## 10. Fonctions SERVO / sorties ArduPilot

Configuration relevée :

```text
SERVO1_FUNCTION = 73  # Throttle Left
SERVO3_FUNCTION = 74  # Throttle Right
SERVO2_FUNCTION = 35  # Motor3
```

Important : les numéros SERVO ne doivent pas être assimilés directement aux numéros de télémétrie ESC.

La preuve expérimentale donne déjà :
- commande logique droite via `SERVO3`
- télémétrie roue droite via ESC2 / `esc_index=1`

Les mappings commande et télémétrie sont donc des relations distinctes.

## 11. RPM1 / RPM2

Configuration actuelle :

```text
RPM1_TYPE = DroneCAN
RPM2_TYPE = DroneCAN
RPM1_ESC_MASK = 0
RPM2_ESC_MASK = 0
```

Le message MAVLink `RPM` #226 est présent autour de 2 Hz mais contient :

```text
rpm1 = -1
rpm2 = -1
```

Explication la plus probable : aucun canal ESC n’est sélectionné dans les masks.

Roue droite prouvée :
- ESC2 = Channel2 = bit 1
- valeur mask correspondante = `2`

Donc, si la convention choisie est `RPM2 = roue droite` :

```text
RPM2_ESC_MASK = 2
```

Pour la roue gauche :
- si gauche = ESC1 → mask `1`
- si gauche = ESC3 → mask `4`

Le VESC de tonte doit rester exclu des masks utilisés par l’odométrie roue.

## 12. Wheel odometry

- plugin MAVROS standard `wheel_odometry` denylisté
- plugin custom `esc_wheel_odometry` chargé mais volontairement désactivé

À valider avant activation :
- ESC roue gauche
- ESC roue droite
- masks RPM1/RPM2
- signe des RPM
- rayon/diamètre roue
- track width
- éventuel rapport moteur/roue
- cohérence distance réelle / odométrie

## 13. GPS / F9P — À CONFIGURER

Topologie :
- F9P physique sur GPS2 du Pixhawk 5X

Configuration QGroundControl actuelle :

```text
SERIAL3_PROTOCOL = GPS
SERIAL3_BAUD = 460800
SERIAL4_PROTOCOL = GPS
SERIAL4_BAUD = 921600
GPS1_TYPE = DroneCAN
GPS1_CAN_OVRIDE = 124
GPS2_TYPE = uBlox
GPS_PRIMARY = SecondGPS
```

Un audit précédent avait relevé `SERIAL4_BAUD=460800`; QGC montre maintenant `921600`. Cette ancienne valeur est donc divergente / obsolète.

Runtime :
- GPS1/GPS2 : `fix_type=0`
- 0 satellite
- position invalide
- Mission Planner : `GPS: No GPS`
- `/gps/fix` non produit

À reprendre : alimentation, TX/RX, baud réel du F9P, configuration UBX, SERIAL4/GPS2, autoconfig ArduPilot.

## 14. IMU — PROUVÉ

MAVROS reçoit :
- `/mavros/imu/data_raw`
- `/mavros/imu/data`

À l’arrêt :
- gyro proche de zéro
- accélération proche de 1 g
- attitude fusionnée présente

Chaîne Pixhawk → MAVLink → MAVROS fonctionnelle.

## 15. Tableau de synthèse

| Élément | État / mapping | Confiance |
|---|---|---|
| Rock 5B ↔ Pixhawk | USB MAVLink OK | PROUVÉ |
| MAVROS ↔ ArduPilot | connecté | PROUVÉ |
| CAN1 | 500 kbit/s DroneCAN | PROUVÉ |
| 3 VESC | présents, OPERATIONAL/OK | PROUVÉ |
| node1 | esc_index 0 / ESC1 | PROUVÉ |
| node2 | esc_index 1 / ESC2 | PROUVÉ |
| node3 | esc_index 2 / ESC3 | PROUVÉ |
| roue droite | ESC2 / esc_index1 | PROUVÉ |
| roue gauche | ESC1 ou ESC3 | À PROUVER |
| tonte | ESC1 ou ESC3 | À PROUVER |
| ESC telemetry MAVLink | `ESC_TELEMETRY_1_TO_4` ~2 Hz | PROUVÉ |
| MAVROS ESC telemetry | fonctionne | PROUVÉ |
| RPM #226 | présent mais `-1/-1` | PROUVÉ |
| RPM masks | `0` / `0` | PROUVÉ |
| POWER1 | BATTERY_STATUS id0 | PROUVÉ |
| POWER2 | BATTERY_STATUS id1 | PROUVÉ |
| GPS2/F9P | configuré mais non détecté | À CORRIGER |
| IMU | fonctionnelle | PROUVÉ |
| double hardware_bridge | ancien déploiement `mowgli-ros2` | PROUVÉ |

## 16. Prochain test minimal recommandé

Un seul test physique supplémentaire suffit pour fermer le mapping ESC :

1. robot DISARMED ;
2. observer `esc_telemetry[0]` et `esc_telemetry[2]` ;
3. tourner uniquement la roue gauche ;
4. identifier lequel des deux slots produit un RPM non nul.

Alors :

```text
si gauche -> esc_telemetry[0]:
    gauche = ESC1
    droite = ESC2
    tonte  = ESC3

si gauche -> esc_telemetry[2]:
    gauche = ESC3
    droite = ESC2
    tonte  = ESC1
```

La tonte sera alors identifiée par élimination, puis pourra être confirmée séparément.

## 17. Configuration RPM attendue après preuve finale

Convention proposée :

```text
RPM1 = roue gauche
RPM2 = roue droite
```

Valeur déjà déterminable :

```text
RPM2_ESC_MASK = 2
```

Pour la gauche :

```text
si ESC1 -> RPM1_ESC_MASK = 1
si ESC3 -> RPM1_ESC_MASK = 4
```

Ne pas inclure le VESC de tonte dans les masks des roues.

## 18. Sécurité appliquée pendant les audits

Pendant les audits et captures :
- aucun armement
- aucun `cmd_vel`
- aucun changement de mode
- aucune commande moteur
- aucune commande lame
- aucune commande d’actionneur pendant les phases passives

Les rotations utilisées pour l’identification ont été réalisées manuellement sur les éléments mécaniques.
