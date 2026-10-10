# P0 — autorité FCU, traction et actionneur lame

Disposition : ACTIVE, conception avant implémentation. 7 octobre 2026.
Baseline locale MowgliMAVROS `main@9f62ba4b1afbccbdaa8b22899ecd4874bc28b204`.
Preuves runtime et observations opérateur :
[checkpoint séparé](operator-observations-and-contract-checkpoint.md).
Le rapport historique MowgliNext n'est pas modifié.

## Autorisation et limites

Le nouvel ordre P0 autorise la préparation du contrat séparé. Aucun commit/push,
aucune modification GUI, aucun réglage compensatoire traction AV/AR.
L'ordre précédent impose d'établir les modes, le transport et l'identité de
l'actionneur avant modification du code. La confirmation définitive sortie
physique/index de commande et le lease FCU restent à acquérir : ce document
ne présente pas une implémentation ou un test matériel déjà terminé.
Pas de modification des paramètres Pixhawk ni de test de sortie dans ce passage.

## Contrat à conserver

Convention existante vérifiée dans MowgliNext `MapPage`, `MowerActions`,
`BladeControlService` et `BladeDirection` :

| Requête | Consigne actionneur |
| --- | --- |
| mow_enabled=0, direction ignorée/canonisée à 0 | OFF / neutre validé |
| mow_enabled=1, mow_direction=0 | FORWARD |
| mow_enabled=1, mow_direction=1 | REVERSE |

Le GUI reste inchangé. Une requête ON ne contourne ni l'inhibition du BT ni
une sécurité matérielle. Un succès de transport ou un ACK n'est pas une preuve
de rotation ; RPM/courant proviennent de l'observation ESC distincte.

## Autorité FCU/traction proposée

Propriétaire unique dans le bridge ; `MowerControl` n'appelle jamais ARM/DISARM.
États internes séparés : `DISARMED`, `VALIDATING`, `ARM_PENDING`, `READY`,
`STOPPING`, `FAULT`. Un booléen `mow_enabled` ne pilote aucune transition ARM.

- Entrée autorisée RECORDING=3, MANUAL_MOWING=4 ou AUTONOMOUS=2 : sélectionner
  le mode correspondant, conserver traction et lame au neutre, vérifier FCU
  connecté, observations requises fraîches, sécurité physique relâchée et absence
  d'urgence active/latched, puis une seule requête ARM si nécessaire.
- Le heartbeat FCU connecté, armé et au mode attendu confirme READY. Un ACK
  ARM seul ne confirme pas la disponibilité. Le bridge ne publie aucune
  consigne non nulle avant READY.
- Une requête déjà en cours n'est pas répétée à chaque tick/HLS. Échec, rejet,
  expiration, désarmement externe ou perte FCU : neutre/OFF et état FAULT.
  Pas de boucle infinie de réarmement ; nouvel essai uniquement selon une
  politique bornée et explicite, après nouvelle intention opérateur valide.
- Une reconnexion/reprise de processus ne réarme pas sur un ancien cache HLS.
  Réacquérir les observations et une transition/intention de session valide.
  Définir la provenance et le renouvellement de cette autorité avant de choisir
  son délai de silence ; une republication de HLS ne prouve pas un nouvel ordre.
- Passer entre états actifs conserve ARM sans le lier à la lame ; un changement
  de transport/mode passe toutefois par neutralisation et confirmation du mode.
- Sortie de l'autorité active : lame OFF, traction neutre, HOLD confirmé.
  Politique de désarmement hors états actifs documentée séparément. Safety/E-stop
  peut désarmer globalement au titre de la sécurité, jamais au titre d'un simple
  `mow_enabled=false`.
- RECORDING et les transits peuvent garder le FCU armé tout en ayant lame OFF.

Modes/transport : RECORDING et MANUAL_MOWING = MANUAL + MANUAL_CONTROL,
autonome/reprise = GUIDED + consigne native de vitesse/yaw-rate en repère corps,
arrêt = HOLD + neutralisation du transport utilisé. Aucun mouvement autonome
n'est nécessaire à la première vérification du contrat logiciel.

## Actionneur lame proposé

Machine d'état séparée : `OFF`, `WAIT_AUTHORITY`, `START_PENDING`,
`FORWARD`, `REVERSE`, `STOP_PENDING`, `COAST_DOWN`, `FAULT`.

- ON attend READY de l'autorité FCU existante, sans demander ARM lui-même.
- OFF stoppe le renouvellement et demande le neutre du seul actionneur lame.
  La traction active reste disponible et ARM reste inchangé.
- ON répétée même direction met à jour uniquement l'intention valide/bail :
  aucune nouvelle séquence de démarrage, aucun ARM, aucun log d'action répété.
- Inversion : OFF confirmé, délai minimal de coast-down ET plusieurs observations
  physiques distinctes et fraîches indiquant RPM sous le seuil d'arrêt, puis
  démarrage opposé. Pas de démarrage si télémétrie absente/périmée. Les seuils,
  durée et limites ne sont pas inventés ; ils seront mesurés sur ce moteur.
- Si OFF arrive pendant ARM_PENDING, START_PENDING ou COAST_DOWN, annuler le
  démarrage différé. Utiliser une génération de consigne pour rejeter un callback
  tardif. Sérialiser les transactions afin qu'une ancienne ON ne soit pas
  réémise après OFF par un retry du transport.
- RPM non nul non demandé, courant incohérent, retour zéro manquant, safety ou
  autorité perdue : OFF, arrêt du renouvellement et FAULT. Ne pas amplifier la
  consigne automatiquement pour obtenir un RPM attendu.
- Distinguer intention demandée, commande transportée/ACK et effet ESC observé.
  Ne pas annoncer « lame tourne » sur `mow_enabled=true` seul.

## Sortie native et point bloquant : arrêt après disparition du bridge

Lectures SSH P0 : hostname rock-5b, checkout remote `9fb33e59`, image MAVROS
`sha256:cfb4c921cc2ea23cf05b45843b8941af7a838ddf18a02091eeacc0dd460acf07`,
SERVO3_FUNCTION=35, CAN_D1_UC_ESC_RV=7, FS_GCS_ENABLE=0, FS_ACTION=0,
MAV_GCS_SYSID=255, MAVROS system_id=255. FCU connecté, désarmé, MANUAL au relevé.

DO_SET_SERVO (183) sur FUNCTION=0 est accepté par le handler source ArduPilot,
alors que Motor3=35 est refusé. **DO_SET_SERVO seul n'a pas de durée de vie** :
le FCU peut conserver et transmettre l'ancienne consigne CAN après la mort du
bridge. Un timer dans le bridge ou une action dans son destructeur ne résout
pas ce cas. Un watchdog VESC ne le résout pas si le FCU continue les RawCommand.
Un failsafe GCS surveille des heartbeats qui peuvent continuer depuis MAVROS,
indépendamment du processus bridge ; ce n'est pas un lease d'actionneur.

### Candidat natif temporisé : DO_REPEAT_SERVO (184), un seul cycle

Source vérifiée au commit FCU
`dbe792162d06cab66c3475fd5556bf7a120f119e` :

- [GCS_ServoRelay.cpp](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/GCS_MAVLink/GCS_ServoRelay.cpp)
  transmet param1=sortie, param2=PWM, param3=nombre de cycles,
  param4=durée du cycle en secondes (conversion *1000 vers ms).
- [AP_ServoRelayEvents.cpp](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/AP_ServoRelayEvents/AP_ServoRelayEvents.cpp)
  prépare `repeat=cycles*2`, `delay_ms=cycle_ms/2`. Avec cycles=1, application
  de la consigne puis retour à `get_trim()` après la demi-période, et fin.
  DO_SET_SERVO OFF sur cette même sortie annule le repeat restant.
- [Rover.cpp](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/Rover/Rover.cpp)
  ordonnance `AP_ServoRelayEvents::update_events` à 50 Hz si compilé.
- FUNCTION=0 est accepté par les deux primitives ; FUNCTION=35 refusé.

Proposition à valider : ON comme une consigne temporisée renouvelée à cadence
bornée par une intention encore valide ; OFF comme DO_SET_SERVO(TRIM) et arrêt
des renouvellements. Si le bridge disparaît ou cesse de renouveler, le dernier
événement reçu expire dans le FCU et remet TRIM, sans dépendre des heartbeats
MAVROS. Ce retour est une propriété source candidate, **pas encore une preuve
d'actionnement/arrêt sur le firmware custom du robot**.

Idempotence ne signifie pas absence de renouvellement d'un lease : seuls les
renouvellements nécessaires et bornés seraient transmis, hors callbacks BT,
sans changement d'état ni ARM périodique. La durée du lease/cadence et la marge
de scheduling/transport doivent être justifiées avant adoption. Param3 doit
être 1, jamais 0 ou négatif (pas de répétition infinie). Respecter les bornes
uint16 de délai du handler. L'expiration est relative à la réception FCU, pas
à l'heure d'émission ROS ; borner aussi les retries et les commandes en transit.

Limite importante : `AP_ServoRelayEvents` partage un unique événement repeat
entre sorties/relais. Un autre DO_REPEAT_SERVO/RELAY peut remplacer cet événement
et supprimer le retour OFF prévu pour la lame. Le candidat exige donc la
propriété exclusive de ce mécanisme dans le déploiement (missions, GCS, RC,
autres producteurs compris) ; sinon il faut une autorité FCU dédiée plutôt
que prétendre que ce lease est garanti. À tester explicitement hors matériel.

## Mapping de sortie : aucune modification tant qu'il reste provisoire

Sortie3/index RawCommand2 est le candidat coupe ; rôle télémétrie bridge slot2.
L'opérateur indique ESC3 et les preuves historiques l'associent à la coupe,
Le dernier geste du premier audit n'avait pas été couvert par une capture RPM
exploitable. La nouvelle capture `20:00:27Z`–`20:00:47Z` du 7 octobre confirme
maintenant le moteur coupe manipulé à la main sur le **slot télémétrie2** seul,
jusqu'à 301 RPM, FCU désarmé, traction neutre et roues à 0 RPM. Retour zéro
observé avec compteurs frais. Voir [preuve dédiée](esc2-physical-identification.md).
L'index de commande VESC, son ID/nœud, le câblage et l'index de télémétrie doivent
être recoupés sur la même baseline, sans déduire l'un de l'autre.

Après confirmation et autorisation de configuration seulement : FUNCTION=0,
borne arrière / neutre / borne avant validés. L'API reste OFF→TRIM,
direction0→avant et direction1→arrière. La première commande d'essai est
TRIM ± petit écart mesuré, jamais MIN/MAX. 100 % nominal n'est validé qu'après
sens, courant/RPM, arrêt, inversion et disparition de bridge vérifiés.

## Vérifications logicielles à réaliser avant déploiement

Dans un domaine ROS isolé avec faux services FCU et aucune connexion robot :

1. ARM appartient aux transitions d'autorité, une requête en cours maximum,
   READY seulement après heartbeat armé/mode ; aucun ARM depuis MowerControl.
2. RECORDING et MANUAL_MOWING traction disponible avec lame OFF ; OFF ne
   produit jamais DISARM en autorité active. AUTONOMOUS utilise le transport
   natif GUIDED et non MANUAL_CONTROL ; neutralisation aux transitions.
3. ON sans READY n'énergise aucune sortie ; OFF annule une ON en attente ;
   réponses ACK tardives, rejet, désarmement externe, reconnect et HLS périmé
   n'entraînent pas de réarmement/démarrage automatique.
4. Égalité ON/direction dédupliquée, renouvellement lease borné, aucune
   commande inversée avant OFF + coast-down + observations d'arrêt valides.
5. Fonction35 rejetée/config non validée inhibe ON ; mauvaise direction,
   paramètres non finis, output=slot confusion et telemetry stale refusés.
6. Avec le vrai mécanisme source FCU ou SITL : perte de bridge, perte ROS/HLS,
   MAVROS restant vivant, retry tardif et interférence autre repeat ; retour
   TRIM observé dans la borne définie ou candidat rejeté.

## HARDWARE_REQUIRED — protocole minimal et prochaine action

Baseline exacte à capturer : IDs d'images/source, firmware custom et manifeste,
paramètres sortie/CAN/VESC, rôle ESC et câblage, robot/FCU concernés.

Identification manuelle du slot télémétrie : PASS sur la baseline de la
[preuve dédiée](esc2-physical-identification.md). Prochaine action sûre : lire
la configuration du VESC cible pour associer son index de commande à
RawCommand2. Une rotation manuelle seule ne valide pas cet index de commande.

Avant changement FCU/test de sortie : checkpoint rempli, prérequis opérateur
confirmés (lame retirée/déconnectée ou moteur neutralisé, robot immobilisé,
E-stop accessible), export des paramètres concernés et restauration préparée.

Essai ultérieur, sous confirmation opérateur distincte : neutre → faible avant
→ neutre → faible arrière → neutre ; seul moteur coupe, aucune traction ni
autre sortie, courant/RPM/sens cohérents, retour zéro. Tester perte du seul
bridge avec MAVROS restant vivant sur montage neutralisé : TRIM demandé au FCU
dans la borne lease définie, puis RPM zéro dans le coast-down mesuré.
La modification/restart nécessaire à ce test n'est pas autorisée par le présent
passage d'inspection. Aucun essai pleine puissance ni mouvement autonome.

Code applicatif, GUI et paramètres robot inchangés. Aucun build/test matériel,
commit ou push. Seul ce checkpoint de conception est ajouté dans le dossier
explicitement demandé. Les tests d'implémentation ne sont pas encore exécutés.
