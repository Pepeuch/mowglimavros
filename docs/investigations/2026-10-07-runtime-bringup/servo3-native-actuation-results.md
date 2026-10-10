# Résultat SERVO3 / primitive native — 7 octobre 2026

Préflight et autorisation : [checkpoint AVANT essai](servo3-native-actuation-preflight.md).
La modification FUNCTION=0 est celle de l'opérateur ; l'agent n'écrit aucun
paramètre FCU. Confirmation physique « oui, j'y suis dans 5 s » acquise, délai
écoulé avant essai. Baseline image/firmware/runtime du checkpoint préflight.

## TEST1 — neutre 1500, sans ARM nécessaire pour ce premier readback

À `2026-10-07T20:19:11.917Z` : MAVROS CommandLong183, param1=3,
param2=1500, broadcast=false. Réponse `success=true`, `result=0` (ACCEPTED).
`rc/out` passe de `[1500,1500,0]` à `[1500,1500,1500]` dans les 4 s
d'observation. FCU connecté, désarmé, MANUAL pendant ce premier essai.

96 observations valides par ESC couvrant préflight + observation :

| ESC | RPM max | Courant max | Tension dernière | Compteur |
| --- | --- | --- | --- | --- |
| 0 | 0 | 0.05 A | 28.03 V | 34321→34709 |
| 1 | 0 | 0.05 A | 27.82 V | 34706→35098 |
| 2 | 0 | 0 A | 28.04 V | 34323→34710 |

Le résultat ACK provient de la réponse du plugin MAVROS. Le prélèvement du
topic MAVLink brut n'a pas capturé un ACK77/command183 correspondant : ne pas
prétendre un second enregistrement wire indépendant. Des commandes400/DISARM
du bridge sont observées pendant toute la fenêtre.

Fin de TEST1 : nouvelle neutralisation183/3/1500 ACCEPTED, puis
DISARM `success=true/result=0`. Readback final connecté/désarmé/MANUAL,
sorties `[1500,1500,1500]`, ESC0/1/2=0 RPM, compteurs frais.

PASS limité : commande183 acceptée et sortie3 ramenée au neutre, sans réaction
roues. FCU désarmé : ce test n'établit pas encore la chaîne de commande CAN
sous ARM ni l'identité de l'actionneur répondant à une consigne non nulle.

## Vérification d'autorité ARM avant TEST2

Préflight passif : HLS MANUAL_MOWING=4, FCU désarmé/MANUAL, bridge émet
400/param1=0 (DISARM) de façon répétée, sorties roues1500 et RPM0. Les topics
cmd_vel et MANUAL_CONTROL ROS sont silencieux dans cette fenêtre ; ce silence
n'est pas une observation de commande0. La vérification ARM maintiendra
explicitement le MANUAL_CONTROL au neutre, jamais de consigne traction non nulle.

Une seule tentative ARM au neutre est autorisée par le protocole utilisateur.
Exiger état armé stable avant toute1550 ; conflit ou désarmement → STOP,
neutre1500 + DISARM. Aucun force-arm, répétition ARM, arrêt de bridge ou
changement de sécurité autorisé pour contourner ce conflit.

## Reprise après sortie opérateur du mode tonte manuelle

L'opérateur indique avoir quitté MANUAL_MOWING et demande « réessaye ».
Nouveau préflight à `2026-10-07T20:21:38.016Z` : HLS IDLE=1,
FCU connecté/désarmé/MANUAL, sorties `[1500,1500,1500]`, roues et coupe0 RPM,
cmd_vel et MANUAL_CONTROL ROS observés au zéro. Compteurs ESC frais.
Traction maintenue explicitement au neutre MANUAL_CONTROL durant la tentative.

- DO_SET_SERVO183/3/1500 à `20:21:38.038Z` : success=true, result=0.
- **Une seule** requête ARM à `20:21:38.561Z` : success=true, result=0.
- Observation4 s après ARM : 1 commande400/param1=1 et **40 commandes400/
  param1=0** concurrentes sur le stream sortant MAVROS.
- Les4 heartbeats observés après demande restent armed=false/MANUAL ;
  aucun intervalle ARM stable acquis. Un ACK ARM seul ne vaut pas confirmation.
- cmd_vel0, MANUAL_CONTROL0, sorties `[1500,1500,1500]` pendant la fenêtre.
- ESC0 :49 observations,0 RPM,max0.05 A,dernier28.48 V.
- ESC1 :49 observations,0 RPM,max0.05 A,dernier28.26 V.
- ESC2 :49 observations,0 RPM,0 A,dernier28.50 V.

**STOP à20:21:42.600Z** : conflit d'autorité DISARM, avant1550/1450.
Sortie du mode tonte manuelle ne supprime pas le problème : la branche IDLE
continue d'envoyer lameOFF au bridge qui la traduit en DISARM global.

Fin explicite à20:21:42.663Z : DO_SET_SERVO183/3/1500 ACCEPTED ;
DISARM à20:21:42.803Z ACCEPTED. Readback final à20:21:44.817Z :
FCU connecté/désarmé/MANUAL ; sorties `[1500,1500,1500]` ;
ESC0/1/2=0 RPM et compteurs frais, tensions28.45/28.26/28.50 V.

## Conclusion

**SERVO3_MAPPING_NOT_CONFIRMED**.

Commande183 acceptée et neutre1500 relu sur sortie3. Pas de consigne1550/1450
envoyée, donc aucun effet actif sur ESC2 mesuré. Le blocage est l'autorité
ARM/DISARM concurrente, pas un refus183. Test2 et Test3 non exécutés.
Sens, deadband, arrêt sous puissance et comportement à cessation de commande
ne sont pas mesurés ; la neutralisation explicite n'est pas un test de lease.

## Proposition de suite — non exécutée, autorisation supplémentaire requise

Préparer une suspension SIGSTOP du **seul** processus mavros_hardware_bridge_node
dans mowgli-mavros, après nouveau readback neutre/désarmé, en conservant MAVROS
et ses services/transport. Ce n'est ni un patch ni un restart ; PID et état
doivent être revalidés juste avant action. La suspension désactive temporairement
les gardes/services ROS du bridge : montage mécaniquement sécurisé, E-stop
matériel indépendant et surveillance dédiée indispensables. Pas de suspension
du processus mavros_node ou du conteneur.

Essai borné déjà décrit, abort sur roues ou rejet ; finally sortie3=1500,
traction neutre, DISARM confirmé, puis SIGCONT du bridge et vérification de
son retour. Prévoir superviseur de restauration si l'outil de test tombe.
Cette isolation n'est pas effectuée dans ce passage et ne doit pas être déduite
de la seule autorisation de consignes183/ARM. Aucun code/GUI/paramètre FCU
modifié ; aucun commit/push.
