# RPM signé — contrat commun ESC0/1/2

2026-10-07 ; demande ajoutée au P0 : vérifier la source avant adaptation,
ne pas créer de chemin particulier ESC2 ni inférer la direction depuis PWM.
MowgliNext4c461ba3 ; submodule main9f62ba4b ; sources FCU ArduPilot
dbe792162d06cab66c3475fd5556bf7a120f119e, baseline robot/image dans
servo3-isolated-actuation-results.md. Aucun nouvel actionnement.

## Source et étage exact de perte

- DroneCAN uavcan.equipment.esc.Status1034 définit rpm int18 signé :
  https://raw.githubusercontent.com/dronecan/DSDL/master/uavcan/equipment/esc/1034.Status.uavcan
  C'est une définition de protocole, PAS une capture prouvant le signe VESC.
- AP_DroneCAN::handle_ESC_status transmet msg.rpm directement à
  update_rpm(esc_index,...), même chemin pour tous les ESC, avec ESC_OF.
  Source épinglée :
  https://raw.githubusercontent.com/ArduPilot/ardupilot/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/AP_DroneCAN/AP_DroneCAN.cpp
- AP_ESC_Telem conserve new_rpm ; son sender MAVLink send_esc_telemetry
  applique fabsf(rpmf), puis borne0..65535, pour tous les slots.
  Perte exacte AVANT réception par MAVROS :
  https://raw.githubusercontent.com/ArduPilot/ardupilot/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/AP_ESC_Telem/AP_ESC_Telem.cpp
  ESC_TELEMETRY_1_TO_4/5_TO_8/9_TO_12 définissent rpm uint16[4],
  donc aucune direction réelle récupérable dans ce payload.
- Plugin esc_wheel_odometry legacy_packet copie m.rpm sans abs supplémentaire.
  ObservationEngine::legacy_esc conserve la magnitude et annonce honnêtement
  rpm_direction_valid=false pour chaque ESC, pas seulement la coupe.
- Le chemin COMMON ESC_STATUS porte rpm int32[4] ; common_status copie le signe
  et rpm_direction_valid=true pour tous les slots. publish_esc copie ces champs
  dans EscObservation sans déduire la direction d'une commande.
- ESC_INFO fournit metadata/index/count/températures/erreurs, PAS le RPM :
  https://mavlink.io/en/messages/common.html#ESC_INFO
  À associer à ESC_STATUS, pas à considérer comme une mesure de vitesse :
  https://mavlink.io/en/messages/common.html#ESC_STATUS

## Réutiliser le transport odométrique existant

Le plugin traite déjà RPM226 (rpm1/rpm2 float signés) pour l'odométrie legacy.
AP_RPM_ESC_Telem obtient la moyenne des ESC du masque, sans fabs ; GCS send_rpm
publie les deux instances seulement. Ce chemin peut expliquer les roues signées
alors que leurs EscObservation issues d'ESC_TELEMETRY restent non signées.
Ce n'est pas une différence de type/configuration/câblage VESC.
RPM226 n'offre pas trois valeurs indexées : ne pas détourner une instance roue
pour la lame, ni fusionner sans identité/provenance des mesures.
Conserver les messages odométriques existants et le couple COMMON
ESC_INFO/ESC_STATUS ; une correction à l'émetteur FCU doit fournir les trois
RPM signés réellement observés si le protocole source les émet.

## Lecture passive robot

SSH rock-5b pepeuch, pas d'interface SocketCAN sur l'hôte ; les VESC sont déjà
câblés au Pixhawk. Deux interfaces USB Pixhawk if00/if02 visibles ; aucune
ouverture de if02 ni modification SLCAN. MAVROS45/bridge46 actifs.
Capture10 s achevée21:27:12,697 UTC (23:27:12 heureParis) :
ESC0/1/2 source2 ARDUPILOT_LEGACY, rpm0, rpm_valid=true,
rpm_direction_valid=false pour les trois, compteurs58876/59516/58878.
FCU connecté, désarmé, MANUAL. Flux11030 et226 reçus25 fois chacun ;
aucun ESC_STATUS291, ESC_INFO290 ou CAN_FRAME386 dans cette capture.
La lecture de paramètres via cache GetParameters renvoie type0 (non disponible) :
ne pas interpréter comme paramètresRPM absents du FCU ni comme valeur0.
Le décodage brut initial n'a pas restauré les zéros tronqués MAVLinkv2 ;
aucune valeur de son dictionnaire wire vide n'est utilisée comme preuve.

## Tests logiciels, pas preuve VESC

Ajout test_signed_esc_contract.cpp + cible CMake : COMMON conserve ±120/240/360
et direction_valid pour les trois ESC ; legacy conserve120/240/360 et marque
direction inconnue pour les trois. Aucun traitement spécial, fallbackPWM,
modification de normalisation ou message ROS.
Suite observation_engine existante + deux tests compilés C++17 avec warnings
en erreurs, gtest ; résultat51 tests PASS. Nouveau fichier clang-format18 clean.
Aucun build ROS complet ni preuve de publication runtime COMMON revendiqués.

## HARDWARE_REQUIRED — source réelle pas encore capturée

La présence de signe dans le protocole ne prouve pas l'émission signée par ces
VESC. Aucune comparaison powered roueF/R versus coupeF/R DroneCAN brute acquise.
Une demande opérateur est ouverte pour CAN_FORWARD temporaire filtré1034,
sans ARM/actionnement ni changement persistant. Source FCU épinglée fournit
CAN_FILTER_MODIFY et CAN_FORWARD (bus1, arrêtparam1=0, expiration sans
renouvellement environ5 s vérifiée par callback périodique) :
https://raw.githubusercontent.com/ArduPilot/ardupilot/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/AP_CANManager/AP_MAVLinkCAN.cpp
Disponibilité de ce mécanisme sur l'image custom non encore vérifiée.
Ne jamais injecter CAN_FRAME vers le bus pour une capture passive.

Après autorisation et revue de la procédure : réassembler Status1034 avec
nodeID/esc_index/transferID/timestamps et comparer simultanément RPM226,
ESC_TELEMETRY11030, éventuels291/290 et EscObservation. Pour chaque moteur
testé, neutre→faibleF→neutre+arrêt→faibleR→neutre+arrêt. Identifier les trois
sources au repos, puis comparer une roue et coupe, sans mouvement autonome.
Robot immobilisé, roues levées/sécurisées, coupe retirée ou moteur sécurisé,
E-stop disponible ; autorisation physique distincte après revue, restauration
neutre/DISARM prête. PASS signe réel si le même ESC émet des rpm non nuls
opposés avec observations identifiées fraîches, sans inférence PWM.
Si signe absent à la source : diagnostiquer firmware/configVESC commune,
ne pas annoncer direction_valid=true.

## État du P0 initial

Checkpoint avant code créé : p0-implementation-before.md. Implémentation
ARM/lame manuelle pas encore réalisée : la nouvelle priorité est l'audit
RPM commun demandé avant adaptation. Failsafe FCU lame NON RÉSOLU,
GUIDED/autonomie non implémentés, ni déploiement ni essai matériel.
Seuls tests/CMake et checkpoints modifiés ; aucun commit/push.

## Correction de provenance après rappel opérateur

Lecture complète du checkpoint agent active/MM-ESC-ODOMETRY-20261005.md :
les phases historiques du5octobre établissent déjà RIGHT=ESC0=RPM1,
LEFT=ESC1=RPM2, FORWARDpositif/REVERSEnégatif, TYPE5/masks1,2/scaling1,
avec rotations à la main et rampes powered simultanées +/- sous garde500RPM.
Image historique2032c3d0... différente de l'image du7octobre ; pas de nouvelle
preuve physique sur cette image à déduire. Ne pas refaire ces essais simplement
par oubli du checkpoint. Le signe DroneCAN roue est cohérent avec ce chemin
source conservant le signe, sans capture brute bus historique revendiquée.
Ce checkpoint identifie aussi les limites du plugin stock MAVROS esc_status
(INFO nécessaire au tableauSTATUS, mauvaise échelletempérature), déjà évitées
par le décodage direct du plugin propriétaire. ESC2direction reste non acquise.
