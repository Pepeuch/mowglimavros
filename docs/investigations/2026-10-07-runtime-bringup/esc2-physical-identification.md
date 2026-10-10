# Identification physique ESC2 — capture passive

7 octobre 2026. Télémétrie physique slot2 identifiée ; index de commande
VESC/sortie3 encore HARDWARE_REQUIRED.

Ce relevé complète les checkpoints précédents sans changer leur baseline.
Autorisation : observation uniquement ; aucune commande traction/lame, ARM,
paramètre FCU, activation CAN forwarding ou changement GUI.

Baseline revalidée : robot `192.168.10.32`, user pepeuch UID1000, `rock-5b`,
PWD initial `/home/pepeuch`, MowgliNext remote `feat/mavros-refresh@9fb33e59`,
image MAVROS `sha256:cfb4c921cc2ea23cf05b45843b8941af7a838ddf18a02091eeacc0dd460acf07`.
Submodule local `main@9f62ba4b1afbccbdaa8b22899ecd4874bc28b204`.

## Procédure et critères

1. Démarrer les subscriptions passives avant le geste ; attendre la découverte
   DDS et observer FCU connected=true/armed=false, commande traction neutre,
   sorties roues au neutre, RPM roues et coupe zéro avec observations fraîches.
2. L'opérateur fait tourner à la main le seul arbre du moteur de coupe, puis
   confirme le geste et l'arrêt. Aucun clic ON lame/tonte manuelle nécessaire.
3. Capturer simultanément HLS, FCU State, cmd_vel/teleop/emergency,
   MANUAL_CONTROL ROS et MAVLink sortant, SERVO_OUTPUT_RAW/rc_out,
   EscObservation (slot, validités, RPM, courant, tension, compteur), Status.
4. Critère identification télémétrie : slot2 seul voit un RPM non nul cohérent
   avec le geste ; slots0/1 restent arrêtés, FCU désarmé, aucune commande
   traction non nulle, compteurs source frais et retour RPM zéro confirmé.
5. La rotation à la main peut produire du RPM avec courant nul ; elle n'est
   pas un essai sous charge et ne valide pas le sens du RPM legacy non signé.

## Deux preuves différentes

L'identification physique du slot télémétrie 2 ne prouve pas à elle seule que
le VESC coupe écoute le RawCommand index2. Lire ensuite son index de commande
configuré, identité/nœud et câblage avec un accès VESC sûr. Sur cette baseline,
ESC_BM=7/ESC_RV=7/ESC_OF=0 définit l'intention FCU sortie3→RawCommand2 ; le
paramétrage du récepteur VESC est une preuve distincte.

Ne modifier SERVO3_FUNCTION qu'après ces preuves et validation opérateur.
Le candidat FUNCTION=0, MIN1000/TRIM1500/MAX2000 et OFF/valeurs positives/
négatives par rapport au neutre reste à valider ; aucune valeur nominale ou
primitive d'actionneur n'est testée dans cette capture.

## Résultat

Capture démarrée à `2026-10-07T19:59:43.331Z`, active et neutre confirmés avant
le geste. L'opérateur répond « oui je le fais tout de suite » à la demande de
rotation manuelle du seul arbre coupe, sans commande GUI.

| Fenêtre (fin UTC, périodes de 4 s) | Slot2 RPM max / observations non nulles | Slots0/1 RPM |
| --- | --- | --- |
| 20:00:31.351 | 106 / 21 sur 48 | 0 / 0 |
| 20:00:35.336 | 272 / 47 sur 48 | 0 / 0 |
| 20:00:39.336 | 298 / 48 sur 48 | 0 / 0 |
| 20:00:43.338 | 301 / 48 sur 48 | 0 / 0 |
| 20:00:47.336 | 250 / 8 sur 48, dernier RPM=0 | 0 / 0 |
| 20:00:51.352–20:00:59.352 | 0, aucune observation non nulle | 0 / 0 |

172 observations slot2 non nulles dans ces cinq périodes. Courant slot2=0 A,
tension environ 28.64–28.68 V ; courant nul cohérent avec une rotation à la
main et non avec une mesure sous charge. Compteur slot2 58028→59002 pendant
la phase de rotation et 59198→59589 dans les fenêtres arrêtées suivantes :
pas de gel de télémétrie. Les trois slots sont valides et leurs compteurs
avancent. Le Status public lame suit les valeurs RPM du seul slot2, y compris
le retour zéro, tandis que mow_enabled reste false.

Sur les mêmes fenêtres : FCU connected=true, armed=false, MANUAL ; HLS IDLE.
cmd_vel et MANUAL_CONTROL ROS y/z=0, MANUAL_CONTROL MAVLink y/z=0 cible1.
SERVO_OUTPUT_RAW/rc_out premières sorties `[1500,1500,0]`. Aucun ordre traction
non nul observé ; aucune commande moteur/ARM/lame produite par l'agent.
Les commandes DISARM déjà émises par le bridge sont présentes, distinctes
des gestes manuels et de l'observation de l'agent.

**PASS limité : le moteur de coupe manipulé par l'opérateur est observé sur
le slot télémétrie MAVROS ESC2 de cette baseline.** Roues immobiles selon
leurs retours RPM et traction neutre selon les messages/sorties observés.
Retour zéro physique observé par télémétrie fraîche après la rotation.
L'opérateur confirme ensuite « je viens de stopper » ; les fenêtres suivantes
jusqu'à `20:02:35Z` restent à zéro RPM pour les trois ESC, FCU désarmé et
traction neutre. L'arrêt est donc recoupé avec la confirmation opérateur.

**Non prouvé :** ID/nœud VESC, son index RawCommand d'écoute, correspondance
physique sortie3→ESC coupe, sens du RPM legacy non signé, commande active,
charge/courant nominal, arrêt après perte de bridge ou inversion. Cette capture
ne justifie pas encore SERVO3_FUNCTION=0 sans lecture du VESC et validation
opérateur. Invalidation si câblage, index VESC, rôles, source télémétrie,
firmware ou images changent.

Prochaine étape : lire/faire confirmer la configuration du VESC coupe et son
index de commande2, puis présenter les paramètres envisagés et l'essai faible
consigne pour validation opérateur. Aucune modification de paramètre effectuée.
Voir `p0-fcu-traction-blade-contract.md` pour l'autorité ARM séparée et le
préalable d'un lease d'actionneur supervisé côté FCU.
