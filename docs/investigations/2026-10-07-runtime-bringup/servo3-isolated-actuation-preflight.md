# SERVO3 — checkpoint avant essai avec bridge temporairement suspendu

7 octobre 2026. Autorisation opérateur explicite SIGSTOP/SIGCONT du seul
hardware_bridge, essai183/3 à1500→1550→1500→1450→1500 si conditions stables,
DISARM final et reprise obligatoire. Robot immobilisé, coupe physiquement
sécurisée, E-stop physique accessible confirmés par l'opérateur.

Baseline SSH revalidée : pepeuch UID1000, rock-5b, PWD /home/pepeuch,
MowgliNext remote9fb33e59, image MAVROS
sha256:cfb4c921cc2ea23cf05b45843b8941af7a838ddf18a02091eeacc0dd460acf07,
démarrée2026-10-07T18:04:18.7915975Z. Pixhawk5X/ArduRover4.7.1 dbe79216.
Submodule local main@9f62ba4b1afbccbdaa8b22899ecd4874bc28b204.

PID container46 : exactement
/ros2_ws/install/mowgli_mavros_bridge/lib/mowgli_mavros_bridge/mavros_hardware_bridge_node
avec remap __node:=hardware_bridge. PID45 est MAVROS : il n'est jamais suspendu.
Le chemin exécutable et l'identité PID seront revérifiés juste avant chaque signal.
Relecture FCU FUNCTION0, MIN1000/TRIM1500/MAX2000, roues74/73 avant essai.

## Restauration préparée AVANT SIGSTOP

Le superviseur est un processus Python indépendant, créé avant suspension et
confirmé prêt avec ses propres clients MAVROS. Il surveille un pipe de vie du
test : EOF/crash ou échéance75 s déclenche la restauration, sans dépendre d'un
timer dans hardware_bridge. Aucun fichier n'est écrit sur le robot.

Restauration normale et de secours : demande183/3/1500, neutreMANUAL_CONTROL,
observation zéro trois ESC, DISARM, signalSIGCONT du PID validé puis vérification
réception fraîche /hardware_bridge/status et état process non T. En secours,
l'échec d'un service est consigné, DISARM et SIGCONT sont néanmoins tentés.
L'ordre400/ARM n'est jamais répété pour contourner un rejet.
Une échéance du superviseur interrompt uniquement son propre processus de test,
pas MAVROS ni un autre processus robot. Le trap du test exécute aussi finally.
Le superviseur n'est désarmé qu'après restauration vérifiée.

## Gardes et déroulement

- AvantSTOP : connected=true, armed=false, modeMANUAL, ESC0/1/2=0 RPM et frais,
  traction zéro observée, HLS IDLE, sortie3=1500 relue.
- Maintenir uniquement MANUAL_CONTROL neutre. Aucun cmd_vel non nul publié.
- AprèsSTOP et état T confirmé : drainer les requêtes déjà en transit,
  puis vérifier2 s sans commandeARM/DISARM autre que celles du test.
  Si un autre producteur continue : STOP avantARM.
- Neutre1500 confirmé sur rc/out `[1500,1500,1500]`, puis ARM explicitement.
  Exiger heartbeat armé stable2 s, toutes sorties neutres, trois ESC arrêtés.
- Positif1550 pendant3 s maximum ; surveillance immédiate roues0 et fraîcheur.
  Neutraliser1500, observer sortie3neutre et RPMcoupe0 pendant au moins1 s.
  Seulement si l'essai est propre et l'arrêt acquis, négatif1450 pendant3 s,
  puis même retour1500 et attenteRPM0. Jamais MIN/MAX ou inversion directe.
- Rejet183, RPMroue, sortie roue non neutre, cmd_vel/ManualControl non nul,
  perteFCU, safety/mode/ARM incompatible ou télémétrie périmée : STOP/finally.
- Aucun changement code, GUI, paramètres, modeFCU, CAN forwarding ou container.
  Suspension limitée au bridge déjà identifié, pas de restart.

ACK183 via réponse CommandLong success/result et capture brute si disponible.
Mesurer simultanément State/rc_out/ESC0..2 RPM-courant-tension et diagnostics.
La télémétrie legacy RPM ne donne pas le sens physique : observation opérateur
nécessaire. Cet essai ne valide ni vitesse nominale100%, ni deadband précis,
ni watchdog du futur produit, ni arrêt à simple cessation DO_SET_SERVO.

Résultat à écrire dans un fichier séparé du même dossier. Aucun ordre de
suspension/non-neutre n'a été envoyé à la création de ce checkpoint.

## Condition ajoutée par l'opérateur avant essai

L'opérateur indique que le robot est au dock et propose d'enlever le chargeur.
Essai retenu avantSIGSTOP/ARM/non-neutre en attendant confirmation opérateur
de l'interruption de charge selon sa procédure habituelle, sans mouvement
autonome et en conservant robot immobilisé/coupe sécurisée/E-stop accessible.
Le dock seul ne prouve pas un refusARM natif ArduPilot ; le conflit40DISARM
de l'essai précédent est indépendant et reste à isoler comme prévu.
Aucune commande de chargeur, suspension de processus ou ARM envoyée dans
ce nouveau passage.

L'opérateur confirme ensuite que la charge est interrompue. L'hypothèse
« sécurité dock bloque » reste à comparer au conflit DISARM observé ; elle
n'est pas présentée comme établie avant l'essai. Les confirmations précédentes
robot immobilisé, moteur sécurisé, E-stop accessible restent le cadre autorisé.

## Reprise : cadence observée avant toute suspension

Le passage précédent a été arrêté après le neutre accepté mais AVANT SIGSTOP
et ARM : garde sys_status à 2 s incompatible avec sa cadence réelle. Retour
1500 et DISARM acceptés, trois ESC zéro ; bridge jamais suspendu.
Nouvelle capture passive de 12 s sur rock-5b : sys_status et rc/out 6 messages
chacun, intervalle maximal 2,0014 s ; State 12 messages, intervalle 1,017 s ;
ESC0/1/2 53/51/49 observations, intervalle maximal 0,409 s, tous zéro RPM.
HLS IDLE, cmd_vel zéro, charge false, rc/out [1500,1500,1500], FCU désarmé.
PID46 et identité starttime2392666 inchangés. Le superviseur d'essai en mémoire
utilise désormais 2,5 s pour sys_status et rc/out seulement ; State reste 2 s,
ESC restent 1 s. Aucun changement de fréquence FCU, paramètre ou code produit.
Cette marge documente la cadence mesurée, sans remplacer l'identité des
observations ESC (compteurs avançants). Tout dépassement reste un STOP.
