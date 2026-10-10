# Capture DroneCAN1034 sur FCU actuel — résultat

Autorisation opérateur CAN_FORWARD temporaire sans ARM/moteur, câble inchangé.
Même robot192.168.10.32/rock-5b, MAVROS45/bridge46 actifs, firmware/image de
servo3-isolated-actuation-results.md ; FCU connecté/désarméMANUAL avant/après.

CAN_FORWARD32000/bus1 accepté success=true/result0, filtre388/1034 seulement.
Première capture15 s disponible mais décodage local initial des champs non
alignés int18/uint5 incorrect : produisait artificiellement indices0/2/4 et
RPM multiples65536 au repos. Ce résultat est EXCLU des preuves physiques.
Réassemblage/CRC valides n'impliquent pas un découpage correct des scalaires.
Correction suivant canardDecodeScalar : les bits du dernier octet partiel
occupent les bits de poids fort, pas faible. Code source de référence :
https://raw.githubusercontent.com/dronecan/libcanard/master/canard.c
rpm18 = p10 | p11<<8 | (p12>>6)<<16, extension signebit17 ;
esc_index = (p13>>2)&31. Pas de PWM ni traitement spécialESC2.

Contre-capture4 s UTC21:34:54,215→21:34:58,235 (Paris23:34:54→23:34:58).
Transferts complets identifiés bus0 dans CAN_FRAME (commande forwardingbus1),
DTID1034, CRC16 vérifié avec signatureA9AF28AEA2FBB254, payload14octets.

| Source DroneCAN | esc_index | Transferts complets | RPMmin/max | Dernière tension/courant |
|---|---|---|---|---|
| node1 | 0 | 152 | 0/0 | 26,891V / -0,0228A |
| node2 | 1 | 174 | 0/0 | 26,703V / -0,0301A |
| node3 | 2 | 178 | 0/0 | 26,922V / 0A |

Derniers payloads :
- node1/index0 :00000000b94ed5a5e55c00000580
- node2/index1 :00000000ad4eb6a7e35c00000104
- node3/index2 :00000000bb4e0000ca5c00000008

1658 CAN_FRAME reçues ;111 transferts incomplets/gaps rejetés, aucun CRC
non vérifié utilisé. La capture viaROS/MAVLink n'est pas sans perte ; ne pas
présenter ces compteurs comme cadence totale VESC ni contrôle exhaustif du bus.
RPM2260/0, ESC_TELEMETRY11030unsigned0/0/0/0 ; EscObservation0/1/2 rpm0,
direction_valid=false, source2legacy en parallèle. Pas de290/291 reçu.

Arrêt forwarding32000/param1=0 accepté21:34:58,254UTC ; filtreretiré après
arrêt ; aucune CAN_FRAME pendant2 s de contrôle. FCU reste désarméMANUAL,
troisESCzéro à21:35:00,592UTC. Aucune commande ARM/DISARM/mode/actionneur,
aucun paramètreFCU, aucune ouvertureUSBconcurrente, aucun changement bridge.
Les ACK77parasites de services existants ne sont pas des ordres de ce lecteur.

## Conclusion

Accès aux StatusDroneCAN bruts des trois VESC CONFIRMÉ via le Pixhawk actuel.
node1/index0, node2/index1, node3/index2 observés ; protocole signé identique
pour les trois. Tous arrêtés : cette capture NE VALIDE PAS le signe en rotation.
Les phases roue historiqueRPM226signé restent réutilisables avec leur baseline.
Il reste HARDWARE_REQUIRED d'observer ESC2dans les deux sens et, si nécessaire,
une roue de référence simultanément au brut1034/226/11030/EscObservation.

Même procédure de sécurité du checkpoint avant capture ; prochaine phase
physique doit être revue et séparément autorisée. Pas de nouvel actionnement
ni de déploiement du P0 avant revue. Tests communs51PASS ; applicationP0
ARM/lame non encore modifiée, failsafeFCUlame NON RÉSOLU.
