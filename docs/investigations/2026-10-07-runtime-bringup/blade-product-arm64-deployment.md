# OFF/FWD/REV — préparation ARM64 et déploiement contrôlé, 2026-10-09

Autorisation opérateur : build ARM64 et remplacement du seul bridge validé,
rollback avant remplacement du sidecar. Aucun code produit nouveau, VESC,
paramètre FCU, GUI, commit/push. STOP au premier comportement inattendu.

Source : main9f62ba4b + patch non commité, validation complète référencée dans
blade-product-fwd-rev-implementation.md. Node SHA256
a8cbaee9bae1fe050a8479ec72e5100745a283003c30414e13901e06649584cf.
Baseline robot revalidée : pepeuch UID1000, rock-5b, aarch64,
Armbian26.8.3 Debian13.6, kernel7.1.8-edge-rockchip64 ; PWD/home/pepeuch.
Sidecar OFF-only281e47a4… démarré2026-10-08T18:04:49.171997794Z inchangé.

Build isolé UID1000, sans réseau/devices, dérivé du sidecar OFF-only actuel.
SDK MAVLink header-only précédent inchangé, seul target bridge compilé.
Aucune acceptation physique du nouveau binaire revendiquée à ce stade.
FWD1450/REV1550 sont la baseline faible, pas une vitesse nominale.
Les essais moteurs restent bloqués jusqu'à nouvelle confirmation explicite :
robot immobilisé, coupe sécurisée, roues incapables de déplacer le robot,
E-stop accessible, zone dégagée et configuration VESC inchangée depuis A/B.

Déploiement subordonné au build vérifié, rollback exact OFF-only préparé,
préflight frais désarmé/neutre/RPM zéro/sécurité inactive/traction nulle.
Validation progressive startup/OFF puis essais FWD/REV/inversion/safety avec
secours indépendant ; pas d'ancien outil natif ON sous le nouveau propriétaire.
Failsafe perte totale companion toujours NON RÉSOLU.

État initial : BUILD_PENDING / DEPLOYMENT_PENDING / HARDWARE_REQUIRED.

## Avant remplacement — prérequis logiciels et préflight PASS

Build natif UID1000 GCC15.2 depuis baseOFF-only281e47a4…, target bridge PASS.
ELF AArch64, aucune bibliothèque absente au ldd. BinaireSHA256
47527d460563bbb4656a328cb78e772a7d3ce1fd7f9b1e6787e303bd4b03f22c.
Graphs réels isolés sans réseau/devices : blade startup/OFF/FWD/REV/inversion/
safety/stale/reject/reconnect/status PASS et provider4 variantes PASS.
Ce sont des mocks FCU, pas des essais physiques.

Image locale blade-product-20261009 :
sha256:788d583129ffeec5cebd660cff1ddfe15357bae272b7741bf7be76bdba0759e1.
Seulement quatre COPY : binaire bridge, header node, header BladeControl,
configuration package. Aucun plugin/MAVROS/interface/détecteur/launch remplacé.
SHA du binaire embarqué identique au binaire ARM64 testé.

Rollback privé : /tmp/mowgli-blade-product-rollback-20261009.Xng0jP ; sauvegarde
.env/inspect/identités autres conteneurs, script syntaxe vérifiée restaurant
le digest exactOFF-only281e47a4… et recréant uniquement mavros --no-deps.
Ne pas publier les sauvegardes contenant potentiellement des secrets.

Préflight epoch1791530657.788 environ, conteneur diagnostic r3 exit0 :
connecté/désarmé/MANUAL, rc/out1500/1500/1500, RPM0/0/0 et compteurs progressants,
Emergency inactive/non latched, HLSIDLE, traction nulle et mow_enabledfalse.
Charge active fraîche. Paramètres lus via /mavros/param/get_parameters :
SERVO1/2/3_FUNCTION74/73/0, SERVO3_MIN/TRIM/MAX1000/1500/2000.
Paramètres ROS actuels mowing_enabledtrue, blade_esc_slot2.

Incidents outils corrigés sans toucher le produit : sourcing ROS incompatible
avec nounset avant setup ; Dockerfile FROM imageID interprété comme dépôt,
remplacé par tag local revalidé par digest ; ancien endpoint ParamGet absent,
lecture GetParameters établie dans le checkpoint précédent utilisée ensuite.
Tous avant remplacement ; aucun ARM/actionnement.

Prochain : observateur démarrage/OFF prêt avant remplacement, puis uniquement
neutralisation automatique désarmée et OFF répétés. Les essais non neutres
restent HARDWARE_REQUIRED en attente de nouvelle confirmation opérateur.

## Déploiement effectué — validation runtime interrompue

Seul mavros recréé le2026-10-09T07:25:52.294847266Z, image788d5831… active.
Seule la sélection MAVROS_IMAGE a été actualisée par le helper officiel
upsert_env_key ; Compose YAML inchangé SHA256cf5b58328421f59301634b2499d468501be8092204d54c7d0be46f5af4d517fd.
Le SHA du binaire installé correspond au binaire ARM64 testé ci-dessus.
Environnement comparé comme ensemble clé/valeur identique (ordre seul différent),
Entrypoint/Cmd/mounts/NetworkMode/IpcMode/Privileged/Devices/Binds inchangés.
IDs ET StartedAt de ros2/gui/gps inchangés ; aucun autre conteneur produit arrêté.
Rollback préparé, NON exécuté ; l'image produit reste actuellement déployée.

Observateur startup indépendant : mowgli-blade-product-startup-off-20261009,
ready1791530734.4608188, finexit1. Deux défauts de l'outil diagnostic ont empêché
la preuve startup complète : souscription MAVLink sink reliable incompatible
avec le publisher best-effort, et synchronisation sur intention off publiée
avant que le FCU connecté soit stabilisé. ACK183/result0 reçu à1791530758.2417774,
mais payload sortant NON capturé. L'assertion état à1791530758.4408264 a échoué.
Ne pas attribuer cet échec à un refus FCU, un mouvement, ou au code produit :
la capture ne permet pas ce diagnostic. Le subscriber PRODUIT utilise déjà
SensorDataQoS et n'a pas ce défaut de l'observateur temporaire.

STOP appliqué : aucun appel OFF explicite du vérificateur n'a été atteint,
aucun ARM, aucun1550/1450, aucun essai inversion/emergency/lift/tilt/reconnect.
La déduplication OFF et l'absence ARM/DISARM au flux startup restent NON VALIDÉES
sur cible dans ce passage : pas de faux PASS issu des mocks logiciels.
L'outil local a été corrigé QoS/synchronisation et sa syntaxe vérifiée, mais
NON relancé. Aucun second remplacement sidecar ni restart bridge réalisé.

Contre-vérification uniquement passive après STOP : conteneur
mowgli-blade-product-final-passive-20261009, exit0, epoch1791530839.4619946 :
- connecté=true, armed=false, MANUAL, rc/out1500/1500/1500 ;
- intentionoff, mow_enabledfalse, bridge actif ;
- ESC0/1/2 valid=true/rpm_valid=true, RPM0/0/0 ;
- compteurs7495/38228/7576 ->7739/38475/7821 pendant la fenêtre ;
- courants0/0.04/0A, tensions28.57/28.35/28.60V ;
- Emergency inactive/non latched, HLSIDLE, tractionnulle ;
- charge active fraîche, Powerstamp1791530839.002097812 ;
- six paramètres relus74/73/0/1000/1500/2000, inchangés.
Paramètres startup nouveau bridge effectivement lus : forward1450/reverse1550/
neutral1500. Aucun paramètre ROS ou FCU écrit par le vérificateur.

État : ARM64_BUILD_AND_ISOLATED_GRAPHS_PASS ; DEPLOYED ;
STARTUP_CAPTURE_INCONCLUSIVE ; PRODUCT_RUNTIME_ACCEPTANCE_PENDING.
L'acceptation physique reste HARDWARE_REQUIRED ; confirmation renouvelée
demandée mais non reçue pour ce passage. Aucun code produit/GUI/VESC/FCU modifié,
aucun commit/push. Logs diagnostics conservés dans leurs conteneurs distincts.
Ne pas utiliser le bridge nouvellement déployé pour tondre : acceptation cible
incomplète et failsafe perte totale companion NON RÉSOLU.

Reprise exacte après direction opérateur : observateur corrigé best-effort
prêt avant nouvelle génération startup, attendre connecté/désarmé/neutres/
ESC frais zéro stabilisés, capturer183/3/1500 puis répéter OFF sans183/400.
Seulement après startup/OFF PASS et nouvelle confirmation physique (cinq
conditions + profil VESC inchangé), préparer le banc avec secours indépendant
et commandes service produit, puis validation progressive autorisée.

## Reprise après confirmation opérateur

Confirmation renouvelée reçue pour les cinq conditions physiques et le profil
VESC inchangé : « oui tout est ok tu peu envoyer ». Reprise d'abord limitée
au startup/OFF : nouvelle génération identifiée par GID du publisher Status,
observateur best-effort et attente stabilisation FCU. Image/binaire inchangés,
aucun code produit modifié. Nouveau préflight passif avant recréation du seul
sidecar ; aucune rotation avant startup/OFF PASS. Résultats à acquérir.

### Startup et OFF — PASS acquis lors de la reprise

Observateur r4 : changement réel de GID reçu1791531601.4819868,
COMMAND_LONG183/3/1500 à1791531604.2506912, ACK183/result0 à1791531604.5417032.
État connecté/désarmé/MANUAL, rc/out1500x3 et RPM0x3 stabilisés avant les appels.
Aucun183nonneutre/400 capturé sur cette fenêtre. Le test combiné a ensuite
échoué AVANT appelsOFF car son outil utilisait direction au lieu de mow_direction.
Les deux lancements antérieurs ont uniquement identifié le format MessageInfo
Lyrical structuré ; aucun restart/actionnement lors de ces erreurs diagnostic.

OFF répétés ensuite dans mowgli-blade-product-off-repeat-20261009 : exit0,
11réponses success=true entre1791531685.2195356 et1791531686.5414064.
Aucun nouveau183 ou400 pendant la fenêtre ; final1791531689.6642232 désarmé,
neutres1500x3/RPM0x3, intentionoff/mow_enabledfalse. Startup/OFF validés sur
image788d5831… ; aucune rotation encore. Binaire et paramètres inchangés.

Avant essai FWD faible : outil temporaire serviceproduit direction0, CAN1034
CRC/int18 décodeur inchangé/renouvelé2s, roues gardéesneutres/RPM0, safety
inactive et HLSIDLE, charge canonique fraîche active (baseline actuelle),
SYS_STATUS outputs released frais. Secours ancien intégré validé dans SON
conteneur distinct/token lease monotone2s/plafond75s, disponible AVANT ARM.
ARM explicite uniquement par clientbanc, jamais mower_control. Consigne1450
~3s puis OFF, preuvezéro brute+normalisée et coast-down, DISARM, CANstop/filtre
retiré/silenceCAN. Toute anomalie -> STOP et restauration, aucunREVensuite.
Confirmation physique opérateur et VESCinchangés reçue pour cette reprise.
Résultat FWD non encore acquis.

### Première tentative FWD — STOP, aucune consigne non neutre émise

Image788d5831… inchangée ; dernier démarrage sidecar
2026-10-09T07:39:58.379363269Z (recréation startup autorisée). Bridge actif,
aucun autre conteneur produit redémarré. Confirmation physique renouvelée et
profil VESC inchangé conservés. Aucun paramètre/code produit/GUI/VESC modifié.

Principal diagnostic mowgli-blade-product-fwd-main-20261009 et secours
mowgli-blade-product-fwd-rescue-20261009 : conteneurs distincts, PID hôte139759
et139819, namespacesPID non partagés, chacun sonPID1/logger/clientsROS.
Secours ready1791531930.7447908 avant ARM, tokenblade-product-fwd-20261009-01.

| Événement | Epoch UTC réception | Résultat |
| --- | --- | --- |
| OFF service produit | 1791531930.843076 | success=true, dédupliqué |
| CAN_FORWARD1/filtre1034 | 1791531931.191158 | success=true/result0 |
| ARM explicite du banc | 1791531933.5689921 | success=true/result0 |
| ARM stable | 1791531936.291617 | MANUAL, roues/coupe1500, RPM0x3, safety inactive |
| Appel FWD produit direction0 | après1791531936.291617 | réponse false avant échéance4s |
| Commande observée pendant FWD | 1791531937.00259 | 183/3/1500, PAS1450 |
| STOP | 1791531937.0111468 | refus produit, aucun nouvel essai |
| OFF nettoyage | 1791531937.0319586 | success=true |
| DISARM explicite banc | 1791531938.1510298 | success=true/result0 |
| CAN_FORWARD0 | 1791531938.1928773 | success=true/result0 |
| Dernier neutre bridge observé | 1791531938.2307658 | 183/3/1500, ACK183/result0 à1791531938.2511 |

L'outil porte un libellé générique « refused or timed out » ; la durée <1s
depuis ARMstable, contre une échéance4s, établit ici une réponse de refus,
pas une attente4s expirée. AucunCOMMAND_LONG183/3/1450 (ni1550) observé.
Donc ce n'est PAS un refus ArduPilot d'une consigne1450 : elle n'a pas été émise.
La cause exacte d'annulation/rejet dans le chemin produit n'est pas identifiée
par les diagnostics actuels. Ne pas réarmer/contourner les gardes.

Capture brute CRC-validée, décodeur établi inchangé : node1/index0 261transferts,
node2/index1 300, node3/index2 311 ; tousRPM0. Courants bruts max0.06549/
0.07953/0A. Aucune rotation acquise ; aucun signe nouveau validé.
Renouvellements CAN_FORWARD1 explicites autour1933.195/1935.198/1937.215,
ACKresult0. Pas de perte de capture ayant déclenché l'arrêt.

Observation à conserver pour diagnostic, PAS cause définitivement prouvée :
les acquisitions legacy ESC2 distinctes par count progressent environ1Hz,
écart maximal observé1.020492s. Le BladeControl exige fraîcheur1000ms et
réinitialise la preuvezéro lorsque deux acquisitions distinctes dépassent ce
délai. Les republications du même count ne doivent pas rafraîchir cette preuve.
Une concurrence de requêtes OFF ou une autre invalidation n'est pas exclue par
ces logs ; ne pas conclure uniquement à partir de l'écart de réception du banc,
ni allonger un timeout pour masquer l'identité/provenance des observations.

Restauration principale1791531940.9208632 : restored=true/errors=[], exit1
pour l'acceptation FWD refusée, NON pour un échec du nettoyage.
Final connecté/désarmé/MANUAL, rc/out1500x3, intentionoff/mow_enabledfalse,
RPM0x3 valid/rpm_valid, Emergency inactive/nonlatched/HLSIDLE, tractionnulle.
Charge canonique fraîche active, bridge actif. CAN_FORWARD0 acquitté, filtre
REMOVE envoyé ; aucune trame pendant2.2s après drainage0.5s.
Secours normal_done1791531943.1479151, intervened=false, CAN_frames0 sur sa
fenêtre propre2.2s ; exit0. Principal et secours terminés, aucun job moteur actif.

Disposition finale : STARTUP_OFF_RUNTIME_PASS ; FWD_PRODUCT_REFUSED ;
REV/INVERSION/SAFETY_TARGET_NOT_RUN. Acceptation produit reste HARDWARE_REQUIRED.
STOP conformément à la consigne ; aucun code/paramètre corrigé, aucuncommit/push.
Prochain : diagnostiquer en lecture seule le refus service/annulation en corrélant
l'identité des acquisitions ESC et les propriétaires des requêtes OFF. Toute
correction produit, nouveau déploiement ou reprise physique nécessite décision
opérateur ; ne pas présenter ces résultats comme une acceptation FWD/REV.
