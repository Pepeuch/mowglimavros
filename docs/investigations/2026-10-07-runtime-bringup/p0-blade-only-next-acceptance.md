# P0 lame uniquement — reprise du 8 octobre 2026

Disposition ACTIVE ; acceptation du signe ESC2 HARDWARE_REQUIRED.
Submodule main@9f62ba4b1afbccbdaa8b22899ecd4874bc28b204. Modifications
existantes CMake/tests RPM communs et checkpoints conservées ; AGENTS parent
modifié par utilisateur, ne pas toucher. Aucun code produit modifié ici.

## Instructions relues et faits conservés

AGENTS submodule, INDEX, MM-ESC-ODOMETRY-20261005,
MM-SAFETY-INPUTS-20261006, MM-MAVROS-REFRESH-20261006, checkpoints
servo3-native-actuation-* / servo3-isolated-actuation-* / can-passive-* lus.
ESC0 droite, ESC1 gauche, ESC2 coupe ; node1/index0,node2/index1,node3/index2.
SERVO3_FUNCTION0, neutre1500, mapping end-to-end et réponse isolée1550/1450
acquis. Signe roue226 validé historiquement, pas d'essai roues à refaire.
Status1034 transporte int18 signé, legacy perd le signe dans ArduPilot.
Les noms avant/arrière ne sont pas encore associés aux PWM de coupe.

## Périmètre strict

Lame seulement. Pas de refonte modes GUIDED/MANUAL/HOLD ou autorité globale
ARM/DISARM, GUI général, protections roues levées ou autre chantier parent.
Les ARM/DISARM de ce protocole sont des actions de banc bornées, pas une
nouvelle politique produit. Aucun paramètre FCU ni code pendant l'essai.
Pas de commit/push. Tous les checkpoints restent dans ce dossier.

## Baseline à revalider avant essai

Robot192.168.10.32/pepeuch/rock-5b, identité/OS/PWD, commit checkout robot,
digest/start image, PID exact bridge/MAVROS et identité starttime, firmware
Pixhawk5X/ArduRover4.7.1 dbe79216. Baseline précédente imagecfb4c921... et
bridge46/MAVROS45 HISTORIQUE, pas des PID ou valeurs actuels supposés.
Relire sur FCU FUNCTION0, MIN1000/TRIM1500/MAX2000 et sorties roues74/73.
Capturer paramètres sans écriture, connexion/désarmement, chargefalse,
cmd_vel et ManualControl réellement neutres, sorties1500/1500/1500,
RPM0 pour les troisESC avec identité source fraîche, sécurité physique FCU.

## Autorisation et prérequis

Nouvelle autorisation opérateur explicite AVANT toute commande actionneur/ARM.
Robot immobilisé ; lame retirée ou moteur mécaniquement sécurisé ; E-stop
physique indépendant immédiatement accessible ; charge interrompue.
L'autorisation historique du7octobre ne vaut pas nouvelle confirmation.
La suspension temporaire du seul hardware_bridge, si encore nécessaire,
doit être explicitement couverte. MAVROS et les autres processus restent actifs.

Le dernier banc a provoqué HLS EMERGENCY après SIGSTOP, et l'ancien lecteur
a néanmoins poursuivi. Cette anomalie est un point de revue obligatoire,
pas une permission implicite de recommencer. Garder les interverrouillages
FCU actifs ; si une urgence devient active, neutraliser/DISARM/restaurer,
sans commande non neutre supplémentaire. Si l'isolation requise reproduit
cet état, arrêter ce protocole et demander une procédure de banc sûre,
sans élargir automatiquement au chantier modes/autorité globale.

## Restauration indépendante AVANT action

Superviseur séparé avec clients MAVROS, pipe de vie, échéance bornée, identité
du bridge revalidée avant SIGCONT. Prêt/confirmé avant toute suspension ouARM.
Ordre de fin et secours :183/3/1500, traction neutre uniquement, observation
troisESCzéro, DISARM, arrêt CAN_FORWARD32000/param1=0 et retrait du filtre,
SIGCONT si bridge suspendu, vérifier réception status/reprise et FCUdésarmé.
Finally du lecteur + superviseur ne dépendant pas du processus test.
Rejet ou panne d'un service : consigner, tenter les autres actions de secours.
Ne jamais forcer ARM, désactiver une protection ou réessayer en boucle.

## Capture et séquence bornée

Filtre CAN_FILTER_MODIFY388 sur1034, CAN_FORWARD32000/bus1 temporaire.
Ne pas injecter CAN_FRAME sur le bus ; recevoir386 seulement.
Réassembler par bus/node/transferID/toggle, vérifier longueur/CRC signature
A9AF28AEA2FBB254. DSDL champs non alignés : int18 rpm à bit80,
extension du signebit17 ; uint5 esc_index à bit105 (p13>>2)&31.
Rejeter transferts incomplets/conflictuels ; ne pas déclarer capture exhaustive.

Capturer ensemble brut1034 nodes1/2/3, payloads/transfert/timestamps,
legacy11030, EscObservation0/1/2, rc/out, mavros/state, cmd_vel, emergencies,
charge et diagnostics. Aucun changement de pipeline de télémétrie pour le test.

1.183/3/1500, relire neutre et troisESCarrêtés ; aucune roue commandée.
2.ARM une seule fois si toutes gardes sont acquises ; attendre heartbeat
  armé stable au neutre, pas seulement ACK. Tout conflit/rejet : STOP.
3.1550 environ3 s maximum, puis1500.
4.Attendre arrêt ESC2 confirmé par au moins5 observations distinctes et
  au moins1 s à zéro, données fraîches brut etnormalisées ; timeout borné.
5.Seulement si propre,1450 environ3 s, puis1500 et même confirmation arrêt.
6.DISARM, couper forwarding et restaurer bridge/superviseur, vérifier état final.

STOP sur RPMroue/traction non nulle, urgence, charge, perteFCU/télémétrie,
configuration inattendue, ACKrejeté, arrêt manquant ou sortie roue non neutre.
Échéance globale après isolation45 s, watchdog75 s maximum comme précédent
banc ; ces limites ne remplacent jamais provenance/identité d'observation.
Jamais1000/2000,100%, inversion directe ni modification de paramètres.

## Critère PASS et suite ordonnée

PASS signeESC2 : node3/index2 RPMbrut non nul dans les deux impulsions,
signes opposés, ESC0/1àzéro, retours1500/RPMzéro et restauration vérifiés.
Formuler d'abord1550→signeA,1450→signeB opposé. Demander ensuite confirmation
visuelle opérateur du sens mécanique ; bruit/RPM/signe ne valent pas preuve
du sens physique avant/arrière. Aucun mapping avant arbitraire basé surPWM.

Après preuve mécanique seulement, figer neutral1500 et PWM avant/arrière
configurables, sans considérer1550/1450 comme vitesse nominale. Implémenter
on_mower_control→183 uniquement, sans ARM/DISARM ; OFF, sécurité/lift/tilt,
startup/reconnexion→neutre ; inversion→neutre+arrêt/délai ; déduplication.
Publier intention off/forward/reverse distincte des RPM/courant/température
ESC2 et timestamp frais, avec tests exigés par demande opérateur.
L'ajout blade_requested_direction doit être compatible avec le contrat public
et la définition Status réellement installée ; pas de simulation STM32.

Failsafe FCU sur disparition totale companion : NON RÉSOLU, exigence séparée.
Timer Pi et superviseur de banc ne prouvent pas le watchdog produit.

## État à cette création

Protocole préparé ; aucun SSH, ARM, servo, forwarding ou signal processus
exécuté dans cette reprise. Autorisation/sécurité actuelles et baseline runtime
restent à acquérir. Implémentation volontairement retenue jusqu'au sens validé.

## Autorisation et préflight du8octobre

Opérateur confirme tout en place et dock débranché, autorise le protocole
annoncé incluant suspension temporaire bridge avec STOP siEMERGENCY.
SSH pepeuch UID1000 rock-5b, Armbian26.8.3/Debian13.6,
kernel7.1.8-edge-rockchip64, PWD/home/pepeuch, checkoutrobot9fb33e59be9931.
Image cfb4c921cc2ea23cf05b45843b8941af7a838ddf18a02091eeacc0dd460acf07
inchangée, conteneur redémarré2026-10-08T15:24:52.1600688Z.
PID45MAVROS/46bridge identifiés, starttime à lire avant signal (pas historique).
Lecture8 s : connectedtrue/armedfalse/MANUAL, HLS IDLE/emergencyfalse,
Emergencyfalse/latchedfalse/NONE, chargefalse/mowfalse, cmd_vel etManualControl
zéro ; ESC0/1/2rpm0, courant0/0,02/0A, tension26,62/26,46/26,70V,
compteurs17907/18106/17908 frais. rc/out1500/1500/0 : sortie3 sera
neutralisée explicitement et relue1500 avant suspension/ARM.
GetParameters confirme74/73/0 et SERVO3MIN1000/TRIM1500/MAX2000.
Aucun ordre physique dans ce préflight. Prochaine étape autorisée : restauration
indépendante prête, neutre, CAN1034, puis isolation surveillée ; EMERGENCY
interditARM/non-neutre et déclenche immédiatement la restauration.
