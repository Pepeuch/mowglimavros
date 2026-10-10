# Acceptation ESC2 A/B charge — préparation, 2026-10-08

État : HARDWARE_REQUIRED, confirmation physique opérateur attendue.
Aucun actionnement, ARM, CAN_FORWARD ou changement runtime effectué dans
cette préparation. Conclusion A/B : INCONCLUSIVE, essais non exécutés.

Baseline de départ : `blade-off-deployment-runtime.md`, OFF-only déployé sur
rock-5b / Pixhawk actuel, binaire SHA256
`ebf5dfd600c147899e802a71ca29706fdfc82b1f7e065ea4779c7c04cc760b26`.
Baseline à revalider en lecture seule avant les essais.

Autorisation : A sans charge puis B avec charge, seulement SERVO3 neutre et
1550/1450 pendant environ trois secondes chacun, ARM/DISARM explicites pour
le banc. Aucun code, paramètre FCU, mouvement roues, commit/push modifié.
Bridge et sécurité restent actifs ; aucune suspension ni contournement.

Avant TOUT actionnement, confirmation opérateur explicite requise : robot
immobilisé, lame retirée ou coupe mécaniquement sécurisée, roues incapables de
déplacer le robot, E-stop accessible, zone dégagée. Pour A, chargeur débranché
ET is_charging=false / charger_enabled=false réellement confirmés. La dernière
capture OFF-only publiait is_charging=true : ne pas supposer cet état changé.
Identifier les producteurs et la fraîcheur des deux indicateurs ; ne pas
assimiler branchement physique et état backend. Si cette preuve manque, STOP.

Préflight : connecté/désarmé, mode connu, rc/out 1500/1500/1500, trois ESC zéro,
traction neutre et sécurité observables. Préparer capture DroneCAN 1034 filtrée
et restauration indépendante avant ARM. Réutiliser `can-passive-results.md`
pour réassemblage/CRC et décodage signé int18, node3/index2, sans inférence PWM.
Réutiliser `p0-blade-sign-attempt-2026-10-08.md` : une sécurité publiée avant une
suspension ne constitue pas une autorité actuelle. Ne pas suspendre le bridge.

Procédure A puis B : 1500, ARM stable, 1550 ~3 s, 1500 jusqu'à RPM zéro,
1450 ~3 s, 1500 jusqu'à RPM zéro, DISARM. B seulement après A terminé et après
branchement opérateur puis état de charge backend frais/cohérent confirmé.
Ne modifier aucune autre variable entre A/B ; arrêter sur refus ou transition
de sécurité inattendue, sans tentative de contournement ou réarmements répétés.

Capturer : ARM/ACK, mode, rc/out, RPM DroneCAN signé ESC2, courant, temps de
montée/arrêt, ESC0/1 zéro, charge pendant toute la séquence, transitions safety.
Noter 1550 -> signe X et 1450 -> signe Y avant toute étiquette mécanique ;
demander ensuite à l'opérateur le sens observé. Final : neutre confirmé, trois
ESC zéro, désarmé, CAN_FORWARD arrêté, bridge actif.

Conclusions possibles uniquement : CHARGE_NO_EFFECT, CHARGE_BLOCKS_ARM,
CHARGE_BLOCKS_SERVO_OUTPUT, CHARGE_BLOCKS_ESC_ACTUATION,
CHARGE_CAUSES_MODE_OR_SAFETY_TRANSITION ou INCONCLUSIVE.
Failsafe perte totale companion toujours NON RÉSOLU. Aucun résultat physique
nouveau acquis à ce stade. Prochaine étape : confirmation physique opérateur,
puis préflight lecture seule et validation des gardes avant toute commande.

## Préflight A acquis après confirmation opérateur

Opérateur : « oui c'est ok je viens de debrancher le chargeur » en réponse
aux cinq conditions de sécurité et au débranchement. Identité SSH revalidée :
pepeuch UID1000 / rock-5b / /home/pepeuch ; image OFF-only inchangée, active.
Lecture passive huit secondes, epoch UTC final 1791484792.2160547 :
is_charging=false (80 status), charger_enabled=false (8 Power), stamp Power
1791484791.709910247 ; producteur /mavros/battery_observer,
GID 0110a62f7965e3d45af5877d0000be03. v_charge=5.679 V, charger_status=unknown.
FCU connecté/désarmé/MANUAL, sorties 1500/1500/1500, trois RPM/courants zéro.
Compteurs ESC0 48432→48823, ESC1 54765→55160, ESC2 48453→48844.
Lectures suivantes : Emergency inactive/non latched, HLS IDLE/non emergency,
cmd_vel et MANUAL_CONTROL réellement nuls, paramètres lus 74/73/0 et
SERVO3 1000/1500/2000. Aucun paramètre FCU écrit ni ARM à ce stade.

Script de banc temporaire uniquement, pas de changement du code produit :
/tmp/mowgli-off-only-20261008.BcGAvI/blade-charge-ab.py sur robot.
Décodage signed18 testé hors réseau sur -131072/-5000/-1/0/1/5000/131071,
index2 et CRC16 vecteur connu ; PASS. Garde charge/provenance Power, sécurité,
traction, roues, FCU et données DroneCAN pendant l'essai. Superviseur séparé
doit être prêt avant ARM ; absence de battement >2 s ou plafond75 s déclenche
neutre/DISARM/arrêt forwarding. Bridge/sécurité jamais suspendus.

## Test A interrompu — comparaison INCONCLUSIVE

Preuves compactes : `blade-charge-A-interrupted-2026-10-08.jsonl`.
Le script tourne dans un conteneur observateur distinct, UID1000, sans devices,
image originale cfb4c921… avec les interfaces déjà installées. Le produit
reste l'image OFF-only 281e47a4… ; aucune nouvelle image produit déployée.

Première préparation arrêtée AVANT ARM et avant tout 1550/1450 : erreur
du logger (champ wall dupliqué). Neutre/DISARM/arrêt forwarding ont ensuite
été envoyés par un lecteur de restauration séparé, tous success=true/result0 ;
trois RPM zéro, sorties 1500/1500/1500 et aucune CAN_FRAME sur les deux dernières
secondes. Outil temporaire corrigé uniquement, pas le code produit.

Deuxième passage A :

| Événement | Epoch UTC | Observation |
| --- | --- | --- |
| Superviseur prêt | 1791485152.576266 | clients MAVROS disponibles |
| Neutre 183/3/1500 | 1791485152.6436176 | success=true/result0 |
| CAN_FORWARD bus1 | 1791485152.9787152 | success=true/result0, filtre1034 |
| ARM explicite | 1791485156.1163094 | success=true/result0 |
| ARM stable au neutre | 1791485158.3407977 | armed=true, MANUAL, trois sorties1500, roues RPM0, urgence inactive |
| 183/3/1550 | 1791485158.4997308 | success=true/result0 ; ACK brut183/result0 à1791485158.4813244 |
| STOP | 1791485158.9617553 | garde stale/missing raw1 : capture source roue gauche >1 s |
| Retour 183/3/1500 | 1791485159.0011747 | success=true/result0 |
| DISARM | 1791485159.0452554 | success=true/result0, ACK brut400/result0 |
| CAN_FORWARD arrêt | 1791485159.0836937 | success=true/result0 ; filtre retiré |

1550 n'a PAS duré trois secondes : impulsion interrompue après environ0,50 s
entre ACK de consigne et ACK de neutre. ESC2 legacy : un échantillon292RPM,
0,90 A pendant la fenêtre1550, maximum766RPM sur la capture comprenant la
phase de nettoyage. ESC0/1 legacy restent0RPM ; courantmax0,03A chacun.
Ne pas attribuer 766RPM exclusivement à la fenêtre1550.

Capture CRC-validée avant impulsion : node1/index0 201 transferts, node2/index1
228, node3/index2 226, tous RPM0. Derniers1034 : respectivement epochs
1791485157.9800212 / 1791485157.961223 / 1791485157.9596288.
Donc AUCUN RPM brut en rotation acquis pendant1550 ; signe indéterminé.
1450 NON EXÉCUTÉ. Pas de sens mécanique déduit du PWM et aucun mapping FWD/REV
accepté. rc/out3 pendant l'impulsion n'a pas été capturé dans la preuve
compacte ; ACK et réaction ESC2 ne remplacent pas cette valeur.

Les trois flux1034 cessent ensemble environ4,98 s après ACK CAN_FORWARD ;
l'expiration/renouvellement du forwarding est une hypothèse à vérifier
passivement, pas une panne roue ni un effet de charge démontré. Ne pas allonger
la garde de fraîcheur pour masquer cette rupture d'observation.

La vérification de nettoyage à+2 s trouvait encore83RPM ESC2 en coast-down :
cleanup_ok=false correctement enregistré. Aucune inversion ensuite.
Contre-vérification passive cinq secondes terminée epoch1791485228.318003 :
FCU connecté/désarmé/MANUAL, trois sorties1500, ESC0/1/2 RPM0 et courant0A,
tensions26,34/26,21/26,42V, compteurs4614/11186/4635 ; bridge actif,
Emergency inactive/non latched, is_charging=false et charger_enabled=false,
Power stamp1791485227.719555522, CAN_FRAME_count=0 sur cette fenêtre.

Limite du superviseur de banc découverte : le logger sur pipe a pu échouer
quand son lecteur disparaissait (corrigé pour ne pas empêcher les commandes
de secours), et le superviseur est un enfant du même conteneur de banc :
il n'est PAS indépendant de la mort du PID1 de ce conteneur. Aucun ARM lors
de la première erreur ; le nettoyage explicite a réussi lors du second passage,
puis l'arrêt physique publié a été confirmé séparément. La présence du
superviseur ne vaut donc pas validation du secours en perte totale du test.
Avant toute reprise non neutre, vérifier le secours dans un domaine de vie
distinct et sans dépendance au logger/lecteur, ainsi que la capture renouvelée.
Ce défaut d'outil de banc n'est pas un changement de politique produit.

Test B NON EXÉCUTÉ ; opérateur non sollicité pour rebrancher pendant cet arrêt.
Conclusion autorisée : INCONCLUSIVE. A sans charge prouve uniquement ARM accepté
et une réaction ESC2 à1550, pas le signe ni le résultat A/B complet.
Aucun effet ou blocage de charge ne peut être conclu sans B télémétriquement
confirmé. Aucun code produit, paramètreFCU, mode, sortie roue, commit/push
modifié. STOP sur perte de capture ; robot laissé désarmé et neutre.

Reprise exacte : valider passivement la continuité1034 au-delà de la durée
complète avec arrêt du forwarding vérifié ; valider un secours indépendant
du conteneur test. Puis nouvelle confirmation opérateur et A complet ; après
neutre/arrêt/DISARM, demander branchement et confirmer charge fraîche pour B.
Baseline, gardes et procédure A/B identiques, pas de relaxation de sécurité.

## Prérequis passifs validés ensuite, sans reprise moteur

Voir `can-renewal-independent-rescue-validation.md` : capture renouvelée17.001 s
et secours dans un conteneur distinct survivant au SIGKILL du principal PASS.
Ces essais ne changent pas la conclusion A/B INCONCLUSIVE ni le signeESC2 non
validé. Intégration au futur outil A/B et confirmation opérateur avant toute
nouvelle rotation restent obligatoires. STOP après les deux PASS passifs.

## Nouvelle reprise A/B autorisée après les prérequis PASS

Confirmation opérateur renouvelée : « oui tout est dans l'etast de test, tu peu
y aller », en réponse aux cinq conditions physiques et au chargeur débranché.
Baseline robot OFF-only inchangée revalidée par SSH. Préflight passif A exit0,
epoch1791486569.7970254 : paramètres lus74/73/0/1000/1500/2000, FCU
connecté/désarmé/MANUAL, trois sorties1500 et RPM0, Emergency inactive/non latched,
HLS IDLE, traction cmd_vel et ManualControlnulle, chargefalse et Powerfreshfalse.
Powerstamp1791486568.747227445, v_charge5.679V, v_battery25.853V.

Intégration uniquement dans un nouvel outil temporaire :
/tmp/mowgli-off-only-20261008.BcGAvI/blade-ab-integrated.py ; aucun code produit.
Décodeur1034 inchangé importé depuis l'outil testé. Principal et secours ont
chacun leur conteneur ; tokenblade-ab-20261008-A2. ReadinessROS du secours
requise avant ARM et fraîche<=1 s pendant le fonctionnement. Heartbeatprincipal
à compteur croissant, lease2 s et plafond75 s. RenouvellementCAN toutes2 s
dans les spins, y compris attentesRPC, sans publishertraction ni écritureFCU.
Gardesduprotocole précédentes conservées ; retourszéro bruts+legacy, au moins
1 s et5 observations brutes distinctes puis délai supplémentaire avant inversion.
Fin normale vérifiée avant DONE du secours ; le secours confirme sans intervenir.
Résultat de cette reprise A non encore acquis à cette écriture.

## Résultat nouvelle reprise A — PASS, confirmation mécanique acquise

Preuves : `blade-charge-A-pass-2026-10-08.jsonl`, logs conservés dans
mowgli-blade-ab-integrated-A-main-20261008 et son conteneur rescue distinct.
Principal exit0, secours exit0 avec rescue_normal_done/intervened=false : le
secours était disponible pendant le test et n'a envoyé aucune restauration.
15 renouvellementsCAN_FORWARD ; aucune rupture ni garde relâchée.

| Mesure | Impulsion1550 | Impulsion1450 |
| --- | --- | --- |
| Début COMMAND_LONG183 epochUTC | 1791486628.4134626 | 1791486640.9005716 |
| Retour1500 epochUTC | 1791486631.4151855 | 1791486643.9108121 |
| Durée entre commandes observées | 3.001723 s | 3.010241 s |
| rc/out pendant impulsion | 1500/1500/1550 | 1500/1500/1450 |
| RPM1034 node3/index2 non nuls | +10 à +4859 | -4895 à -68 |
| Échantillons1034 totaux / non nuls | 138 /131 | 134 /130 |
| Courant1034 min/max | 0 à0.924805 A | 0 à0.931641 A |
| Tension1034 min/max | 25.359375 à25.53125 V | 25.359375 à25.515625 V |
| PremierRPMnon nul reçu après commande | 0.142998 s | 0.115577 s |
| Premierzéro reçu après retour1500 | 5.737541 s | 5.205528 s |
| RPMEscObservation pendant impulsion | 0 à4186 | 0 à4769 |

Les délais sont ceux de réception télémétrique par rapport aux COMMAND_LONG,
pas des chronométrages mécaniques exacts. Arrêt confirmé par les gardes brutes
et normalisées, avec au moins1 s/5 observations brutes distinctes àzéro,
puis délai supplémentaire et nouvelle confirmation avant la seconde impulsion.
Les ACK183 et ARM/DISARM de la séquence sont success=true/result0.
ESC0/1 restent0RPM dans les observations brutes1034 et normalisées.
MANUAL reste le seul mode observé, Emergencyinactive, HLSIDLE ; aucune
transition safety inattendue. Chargefalse pour status ET Power tout le long.

Signe réel source démontré pour ESC2 : positif à1550, négatif à1450.
EscObservation reste une magnitude positive dans la seconde impulsion sur
cette baseline legacy ; ce résultat n'autorise aucune correction de code
dans cette étape et ne démontre pas encore le transport signé jusqu'au contrat.

Confirmation opérateur après A : « l'ordre physique du sens des lames:
1 arriere /2 avant ». Rapportée à l'ordre effectif1550 puis1450 :
**1550 -> RPM brut positif -> sens mécanique arrière observé** ;
**1450 -> RPM brut négatif -> sens mécanique avant observé**.
Ne pas généraliser cette convention aux roues, à un autre câblage ou firmware.
Ce sont des consignes d'acceptation faibles, pas les valeurs nominales produit.
Aucun FWD/REV implémenté ou déployé.

Fin A : main_cleanup/restored=true/errors=[] à1791486656.838418 ;
FCU connecté/désarmé/MANUAL, sorties1500/1500/1500, troisESC0RPM,
CAN_FORWARD0 acquitté, filtre retiré, silenceCAN vérifié ; bridge actif.
Secours confirme sa fin normale sans intervention à1791486659.1175566,
CAN_frames=0 sur sa vérification finale. Aucun moteur maintenu actif dans
l'attente de l'opérateur.

Test B EN ATTENTE du branchement opérateur et de la charge backend fraîche,
cohérente et avec provenancePower connue. Conclusioncharge resteINCONCLUSIVE
tant que B n'est pas acquis. Même outil/durées/gardes, seul état de charge
attendu et token de génération changent. Aucun code produit/paramètreFCU
modifié, aucun commit/push. Ne pas actionner avant préflightB conforme.

## Préflight B acquis, configuration inchangée

Opérateur confirme chargeur branché puis conditions physiques « oui toujours
pareil mais chargeur brancher ». Demande explicite supplémentaire conservée :
aucune inversion/traduction logicielle du sens ; il configurera les VESC
lui-même APRÈS les essais. La correspondance mécanique A est une mesure sur
la configuration actuelle, pas un contrat logiciel à implémenter.

Image et StartedAt sidecar inchangés ; outil intégré SHA256
5cc45c38938afdc0cd1f7b9bebec593eddb4ddd33d9f8464b49ede68670a7446,
identique au A. PréflightB read-only exit0, epoch1791486991.8374763 : paramètres
lus74/73/0/1000/1500/2000, FCU connecté/désarmé/MANUAL, trois sorties1500,
troisRPM0, Emergency inactive/non latched, HLSIDLE, tractionnulle.
Powerstamp1791486991.752681983 : charger_enabled=true, charger_status=charging,
v_charge26.965V, v_battery26.439V, charge_current+0.870A ; statusis_charging=true.
Provenancecanonique : /mavros/battery_observer sur /hardware_bridge/power,
sidecar non redémarré. Même séquence/gardes/durées queA ; uniquement charge
attendueB et tokenblade-ab-20261008-B1 changent. RésultatB pas encore acquis.

## Résultat B et comparaison finale — CHARGE_NO_EFFECT

Preuves B : `blade-charge-B-pass-2026-10-08.jsonl`. Les mentions « en attente »
précédentes décrivent leurs états historiques. PrincipalB et secoursB exit0,
restored=true ; secours_normal_done/intervened=false. 14 renouvellementsCAN,
aucune rupture ni garde assouplie. Outil et configuration produit/FCU inchangés.

Charge confirmée pendant toute la fenêtre B : tous les statusis_charging=true,
tous les Powercharger_enabled=true, stamps/réceptionsPower gardés frais.
Provenance relue aprèsB : unique publisher /mavros/battery_observer sur
/hardware_bridge/power, GID0110a62f7965e3d45af5877d0000be03, identique àA.

| Comparaison | A sans charge | B avec charge |
| --- | --- | --- |
| Charge canonique /status | false /false | true /true |
| ARM explicite | accepté, stable | accepté, stable |
| Mode pendant séquence | MANUAL | MANUAL |
| ACK183 et ARM/DISARM | success=true/result0 | success=true/result0 |
| rc/out à1550 | 1500/1500/1550 | 1500/1500/1550 |
| rc/out à1450 | 1500/1500/1450 | 1500/1500/1450 |
| RPM1034 non nuls à1550 | +10 à+4859 | +9 à+4831 |
| RPM1034 non nuls à1450 | -4895 à-68 | -4751 à-61 |
| Durées1550 /1450 entre commandes observées | 3.001723 /3.010241 s | 3.019163 /3.020607 s |
| Courant1034 max1550 /1450 | 0.924805 /0.931641 A | 0.905762 /0.915039 A |
| Première réponseRPM reçue1550 /1450 | 0.142998 /0.115577 s | 0.140428 /0.102406 s |
| Premierzéro reçu après1500, positif/négatif | 5.737541 /5.205528 s | 4.989297 /4.601503 s |
| RPMroues ESC0/1, bruts et normalisés | 0/0 | 0/0 |
| Retour1500 /arrêt complet avant inversion | confirmé | confirmé |
| HOLD/DISARM/mode/safety spontané | aucun observé | aucun observé |
| Intervention du secours | non | non |

ÉchantillonsB1034 node3/index2 totaux/non nuls :1550 137/130,1450 139/134.
Tensions1034 B :26.046875–26.234375V à1550 ;26.078125–26.234375V à1450.
EscObservation B resteunsignedlegacy :0..4173RPM à1550,0..4643RPM à1450,
courantmax0.87A dans les deux fenêtres. Le signe réel est prouvé àla source1034,
PAS jusqu'au contrat EscObservation direction_valid=true. Aucune correction
de pipeline effectuée dans cette étape.

ChronologieB epochUTC : ARM ACKservice1791487042.5803218 ;1550 envoyé
1791487045.4214396, retour1500 1791487048.4406025 ; arrêt/coast-down confirmé
avant1450 envoyé1791487056.9029117 ; retour1500 1791487059.9235187.
Les délais de réponse/zéro sont des délais de réception télémétrique, pas des
chronométrages mécaniques exacts. Minimum1 s/5 transferts distincts àzéro et
attente supplémentaire conservés avant inversion.

Conclusion : **CHARGE_NO_EFFECT**, au sens d'absence de blocageARM/sortieSERVO3/
actionnementESC2 ou de transition safety causée par la charge observée dans
ce protocole natif au banc. Cela ne prouve PAS identité des performances,
absence de protectiondock dans toutes les voiesGUI/produit, ou permission de
tondre sur chargeur. Tensions et temps diffèrent entre captures ; aucune
causalitéfine revendiquée sur ces écarts. Baseline limitée àce robot,
Pixhawk5X/ArduRover4.7.1 dbe79216, imageOFF-only281e47a4… et profil inchangé.
Ne pas généraliser après modificationVESC/firmware/câblage/backend.

### État final après B et STOP

main_cleanup1791487071.8405604 :restored=true/errors=[].
FCU connected=true/armed=false/MANUAL, sorties1500/1500/1500.
ESC0/1/2 valid=true/rpm_valid=true, RPM0/0/0, courant0/0/0A,
tensions26.23/26.06/26.21V, compteurs29284/36840/29306.
Emergencyinactive/non latched, HLSIDLE, bridgeactif/mow_enabled=false,
tractionpubliée nulle. Charge active : Powerstamp1791487071.758640208,
v_charge27.049V, v_battery26.530V, charge_current+0.88A, charger_statuscharging.
Neutre/DISARM/CAN_FORWARD0 acquittés, filtreREMOVE1034 envoyé. SilenceCAN
vérifié pendant2.2 s aprèsdrainage par le principal et sur une fenêtre propre
de2.2 s par le secours (CAN_frames=0), fin normale1791487074.1183798.
MAVROS/ros2/gui/gps actifs avec StartedAt inchangés ; aucun redémarrage produit.
ConteneursBdiagnostic terminés, logs conservés.

Sens mécanique arrière1550/avant1450 confirmé par opérateur pour A seulement,
pas de nouvelle inférence mécanique pourB. L'opérateur configurera les VESC
APRÈS les tests ; aucune traduction/inversionlogicielle demandée ou effectuée.
Aucun1000/2000,100%, déplacement, changementARMpolicy, code produit,
paramètreFCU, commit/push. STOP aprèsA/B, aucun FWD/REV produit.
Failsafe perte totale companion NON RÉSOLU ; secours de banc ne couvre pas
perteDocker/Rock5B/MAVROS/liaisonFCU. Validationdocumentaire gitdiff--check PASS.
