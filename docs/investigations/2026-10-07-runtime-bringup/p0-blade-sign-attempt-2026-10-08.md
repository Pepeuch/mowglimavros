# P0 lame — reprise autorisée, arrêt avant ARM le8octobre

Disposition ACTIVE ; signeESC2 HARDWARE_REQUIRED, aucun sens/PWM figé.
Protocole et autorisation : p0-blade-only-next-acceptance.md.
Le8octobre opérateur confirme robotimmobilisé/coupesécurisée/E-stopaccessible,
dockdébranché et autorise la séquence annoncée, avec STOP sur urgence.

## Baseline revalidée

SSHpepeuch UID1000, rock-5b, PWD/home/pepeuch, Armbian26.8.3/Debian13.6,
kernel7.1.8-edge-rockchip64 ; checkoutrobot9fb33e59be9931bbb99a99d9a6389fe06894828d.
Imagecfb4c921cc2ea23cf05b45843b8941af7a838ddf18a02091eeacc0dd460acf07,
nouveau démarrage conteneur2026-10-08T15:24:52.1600688Z ; MAVROS45actif.
Bridge46exécutableexactmavros_hardware_bridge_node/__node:=hardware_bridge,
identitéstarttime2842 revalidée avant signaux (différente du7octobre).
Submodulelocalmain9f62ba4b ; aucune application/code/GUI/paramètreFCU changé.
GetParameters74/73/0, SERVO3MIN1000/TRIM1500/MAX2000.
Préflight8 s : FCUconnecté/désarmé/MANUAL, HLS IDLE/emergencyfalse,
Emergencyfalse/latchedfalse/NONE, chargefalse/mowfalse ; cmd_vel etMANUAL_CONTROL
zéro ; troisESCvalides0RPM, courants0/0,02/0A, tensions26,62/26,46/26,70V.
rc/outinitial1500/1500/0, ensuite neutralisation183 explicite réussie.

## Vérifications préparatoires et gardes

Lecture réutilisée des checkpoints agentESC/safety/refresh et SERVO3/CANpassive,
sans nouvel essai roues ni réinvestigation mappings.
Premier passage : préparation watchdog bloquait brièvement les callbacksROS,
gardeESC1 s expirée avant toutecommande/suspension ; aucune actionphysique.
Correction superviseur uniquement : traiter les réceptions après le handshake,
ne pas modifier seuil, identité d'observation ou code produit.

Deuxième passage : neutre183 accepté, sortie3relue1500, forwarding1034accepté,
suspensionbridge ; gardeMANUAL_CONTROL expirée parce que producteur suspendu.
Retourneutre/DISARM/arrêtforwarding/SIGCONT tous réussis. AucuneARMtrue ni
consigne1550/1450. HLS resteIDLE dans cette fenêtre, pas d'urgenceactiveobservée.

Dernier passage : maintenir explicitement MANUAL_CONTROLstrictementzéro par
lecteur10Hz, comme prévu au banc précédent. Aucun cmd_vel publié.
Watchdog243indépendant prêt avant suspension, clientsMAVROS propres,
pipevie/échéance30 s pour cette vérification pré-ARM, finally+restauration
183/1500, DISARM, arrêtforwarding et reprisePIDvalidé.
SIGSTOP46 confirmé étatT ; MAVROS et autresprocessus restentactifs.
GuardemiseàjoursourceEmergency impose réception2,5 s ; aucune modification
d'interverrouillageFCU ni réarmement/répétitionARM.

## Résultat du dernier passage

HorairesUTC issus des sorties du lecteur ; heureParis = UTC+2.
183/3/1500 success=true/result0 ; CAN_FORWARD32000/bus1 acceptéresult0.
1202 CAN_FRAME reçues, filtre1034uniquement. Exemples node3 0x98040a03 :
5c7c00000000a59e ;4e0000b85c00003e ;00085e.
Ces exemples n'ont pas été utilisés comme preuve de signe en mouvement.

STOP_BEFORE_ARM : stale/missing emergency, environ2,42 s après SIGSTOP.
La dernière observation Emergency date d'avant la suspension. Son absence
est attendue puisque le producteur est suspendu ; sa dernière valeurfalse
ne constitue pas une observation de sécurité actuelle. Aucun délai prolongé
pour masquer cette absence d'autorité/provenance.
HLSreste [1,IDLE,false] ; ne PAS décrire cet arrêt comme EMERGENCYactif ni
prétendre avoir reproduit le changementHLS du7octobre.
FCUconnecté/désarmé/MANUAL, rc/out1500/1500/1500, troisESC0RPM au STOP.

Nettoyage :183/3/1500, DISARMfalse et CAN_FORWARDparam1=0 tous ACKsuccess
true/result0. Filtre retiré après arrêt. SIGCONTduPID46validé ; lecteur observe
nouveau status après reprise, processusS, FCUdésarmé ettroisESC0RPM.
Watchdog libéré parDONE seulement après vérification, processus terminé.
Dernierreadback : HLSIDLE, Emergencyfalse/latchedfalse/NONE, chargefalse/mowfalse,
cmd_vel etMANUAL_CONTROLzéro, out1500/1500/1500 ; ESC0/1/2courant0/0,01/0A,
tensions26,50/26,31/26,56V, compteurs29353/29678/29354 frais.

## Conclusion et prochaine décision nécessaire

SigneESC2 NON VALIDÉ : aucuneARMtrue ni1550/1450 dans cette reprise.
Pas de1000/2000, essai roues, paramètreFCU, mode, arrêtcontainer ou commit/push.
Mappings historiques restent acquis ; cette vérification ne les invalide pas.

La suspension requise pour isoler les DISARMparasites retire aussi le producteur
de sécurité. Continuer demande une procédure où la sécurité reste observable
et autoritaire pendant le banc, pas une lecturefalsemiseencache ou une hausse
de timeout. Ne pas élargir à la politique globale ARM/DISARM/modes.

Option à proposer, NON autorisée/exécutée dans cette étape : correctif minimal
OFFlame→183neutre, ONinhibé, tests logiciels et revue avant déploiement ; cela
supprimerait le DISARMparasite tout en conservant bridge/sécurité actifs pour
l'acceptation du signe. Cette option change l'ordre demandé (code avant sens),
nécessite directionopérateur, revue et autorisation distincte de déploiement.
Sinon faire valider une autorité de supervision de banc indépendante couvrant
les observations safety/lift/tilt/urgence sans désactiver protections.

Prochaineacceptation sur baseline exacte : capture1034CRC/node3/index2,
1500→ARMstable→1550max3 s→1500+zéroobservé→1450max3 s→1500+zéro→DISARM,
ESC0/1toujourszéro. PASS siRPMbrutsESC2nonzéro de signesopposés, gardesactives,
arrêts etrestauration vérifiés. Confirmation visuelle mécaniqueensuite,
jamais sensphysique déduit duPWM. FailsafeFCUtotalcompanion NON RÉSOLU.
