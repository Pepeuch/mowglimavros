# SERVO3 / DO_SET_SERVO — checkpoint AVANT essai

7 octobre 2026. Baseline distincte : modification opérateur SERVO3_FUNCTION=0.
Les preuves précédentes portant sur FUNCTION=35 restent historiques.

## Autorisation

L'utilisateur demande explicitement l'essai natif sortie3 : neutre1500,
puis seulement après confirmation physique faible positif1550, neutre et arrêt,
faible négatif1450, neutre et arrêt. ARM si nécessaire, aucune commande roues,
fin au neutre puis DISARM. STOP si rejet ou réaction roue. Aucun MIN/MAX,
changement code/GUI/paramètres ArduPilot, commit ou push. Écritures limitées
à ce dossier d'investigation.

## Baseline et relecture FCU

SSH pepeuch UID1000 @192.168.10.32, hostname rock-5b,
PWD initial /home/pepeuch. Checkout remote MowgliNext 9fb33e59.
Image MAVROS cfb4c921cc2ea23cf05b45843b8941af7a838ddf18a02091eeacc0dd460acf07,
démarrée 2026-10-07T18:04:18.7915975Z. Firmware de la baseline :
Pixhawk5X / ArduRover4.7.1 dbe79216 ; ne pas généraliser à un autre build.
Submodule local main@9f62ba4b1afbccbdaa8b22899ecd4874bc28b204.

ParamPull(force_pull=true) : success=true, 894 paramètres. GetParameters
après pull confirme sur le FCU (aucun paramètre écrit par l'agent) :

| Paramètre | Valeur |
| --- | --- |
| SERVO1_FUNCTION | 74 / Throttle Right |
| SERVO2_FUNCTION | 73 / Throttle Left |
| SERVO3_FUNCTION | 0 |
| SERVO3_MIN / TRIM / MAX | 1000 / 1500 / 2000 |
| SERVO3_REVERSED | 0 |
| CAN_D1_UC_ESC_BM / ESC_RV / ESC_OF | 7 / 7 / 0 |
| ARMING_REQUIRE | 1 |

## Prérequis et arrêt

Confirmation opérateur robot immobilisé, E-stop accessible, lame retirée/
déconnectée ou moteur mécaniquement sécurisé obtenue : réponse « oui » au
questionnaire regroupant explicitement ces trois conditions, avant essai.
Pas de consigne autre que1500 tant que cette confirmation n'est pas acquise.
Échantillonnage simultané FCU/HLS/cmd_vel/MANUAL_CONTROL/rc_out/ESC0..2/ACK
avant et pendant essai. Exiger connexion et observations fraîches, zéro roues
et commandes traction nulles. L'absence d'un message n'est pas une valeur zéro.

Le runtime précédemment audité émet des DISARM répétés en IDLE depuis
hardware_bridge. Vérifier cette concurrence : un ACK ARM seul ne suffit pas,
il faut armed=true stable au heartbeat. Ne pas réarmer en boucle, passer en
force-arm, modifier une sécurité ou changer le comportement du bridge pour
masquer un conflit d'autorité. Toute extension nécessaire doit être approuvée.

DO_SET_SERVO183 : MAVROS CommandLong, broadcast=false, param1=3,
param2=PWM, autres paramètres0 ; enregistrer réponse success/result et ACK77
command183/result. Rejet : fin sans contournement. Si roue RPM non nul ou
commande traction non nulle : neutraliser sortie3 et DISARM, arrêter le test.

Neutralisation/disarm de fin autorisés ; aucun arrêt/redémarrage de processus
robot, CAN forwarding ou essai de perte bridge autorisé dans cette séquence.
DO_SET_SERVO conserve la consigne jusqu'à changement : utiliser une fenêtre
courte bornée et finally neutre+DISARM ; ce test ne valide pas un watchdog.

## État

Préflight paramètres conforme ; préflight runtime et confirmation physique
à compléter avant essai. Aucun ordre d'actionneur exécuté à la création de ce
checkpoint. Les résultats seront consignés dans un checkpoint séparé.
