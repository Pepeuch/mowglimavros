# Produit lame OFF/FWD/REV — avant code, 8 octobre 2026

Autorisation : implémentation logicielle dans MowgliMAVROS seulement, build/tests
MAVROS épinglés, STOP après validation. Aucun SSH/déploiement/essai physique,
FCU/VESC write, GUI, commit/push ou refonte globale ARM/modes.
Submodule main9f62ba4b + patch OFF-only et modifications RPM/tests préexistants,
conservés. Baselines : blade-off-build-validation.md, blade-off-deployment-runtime.md,
blade-charge-ab-acceptance.md et can-renewal-independent-rescue-validation.md.
La demande jointe autorise explicitement forward1450/reverse1550 configurables
pour CETTE baseline, malgré la décision antérieure de réglage VESC par opérateur.
Il ne s'agit pas d'une inversion déduite automatiquement du RPM.

Architecture visée : état pur testable, états OFF/FWD/REV/TRANSITION_TO_OFF/
WAIT_STOP_BEFORE_REVERSE/FAILED, intention séparée de commande confirmée,
transactionACK et générationconnexion. MONOTONIC pour durée/coast-down/ACK ;
stamp/source/compteurESC pour identité d'observation. Répétitions de callbacks
avec observation identique ne doivent pas confirmer un arrêt.

Timeout, télémétrie manquante, safety ou rejet : neutralisation et annulation
de l'intentionON ; jamais lancement opposé sur simple expiration. ACK confirme
transport uniquement, pas arrêtmécanique. Une inversion exige observations
zéro distinctes/fraîches après neutral et délai de stabilité.
Startup/reconnexion/safety ajoutent une transactionneutral dédiée ; chemins
HOLD/DISARM/tractionexistants conservés, mower_control ne les appelle pas.
Le FCU doit être déjà armé pour ON, lame ne possède aucuneautoritéARM.

Service : ne pas annoncer success après simple enqueue si MAVROS refuse ensuite.
RéponseROS différée vers résultatACK ou échec borné, sans attente bloquante
empêchant callbacks safety/ESC. Déduplicationpending/confirmed, échecs sans
retry par tick. OFF demandé reste prioritaire sur inversion/ON en vol.
Status : blade_requested_direction est intention, mow_enabled seulement lorsque
consigneON acquittée et encoreautorisée ; transitions/safety ne prétendent pas
arrêt physique, télémétrieESC2/stamp continuent leur projection existante.

Invalider cache sur reconnexion et changements de sortie/commandes externes
observables. Ne pas confondre une réponseACK et ownership exclusif : la sortie
MAVROS partage une identitéFCU et une commande externe identique peut être
indistinguable sans contrat de propriétaire. Documenter cette limite et ne pas
présenter déduplication comme preuve physique permanente.

Validation prévue : état pur + graphprovider réel (ArduPilot/PX4), graphblade
mockservices avec ackrejet/délais/inversions/safety/startup/reconnect/status,
suitesbridge/ESC/safety existantes, buildcomplet submodule dans SDK épinglé
(image d98e8e… et Cyclone importé du checkpointOFF), clang-format18 etdiffcheck.
Ne pas utiliser anciensbinaires installés comme preuve du nouveaupatch.
Failsafe totalcompanion NON RÉSOLU ; aucun readinessproduction revendiqué.

## Reprise en cours après interruption — état logiciel, non déployé

NouveauBladeControl.hpp +9gtests purs, node/header/config/CMake et graphblade
intégrés. ServiceROS différé header/request (pas d'attente bloquante), ACK
result0 requis ; startup/reconnexion/safety neutral explicite, callbacks anciens
fencés, messagesexternes183 unmatched et sortiesrc/out distinctes invalident
cache. Paramètreschannel/neutral/forward/reverse startupread_only, baseline
3/1500/1450/1550 ; Status intention et confirmationtransport séparées.
TélémétrieESC et politiqueHOLD/DISARM/traction conservées.
Pas deSSHrobot, déploiement, essai, paramètreFCU/VESC, commit/push.

PASS :9gtests purs actuels ; compilation complète6packages du submodule.
Dernier graphblade passe startup/OFF/FWD/REV/inversions/dédup/ACKrejet/stale/
reconnect/status/safety ; testsajoutés doublelift/tilt/commandexternes.
Graphprovider4variantes PASS, graph safety existant PASS. Validationfinale
pas encore acquise à ce jalon : CTestsuite13/16PASS, trois échecs identifiés :
- test_blade_control : l'attente ancienne « aucune commande » aprèsstale devait
  vérifier une commandeNEUTRAL supplémentaire, pas opposée ; corrigé,9/9PASS.
- test_esc_graph : assertionleft_raw_ticks conservée documentée intermittente
  dans MM-SAFETY-INPUTS ; codeESC/test inchangé, rerun isolé requis.
- graphGNSS : ancienSDKUniversalGNSS ne produit pas les types2D/3D/DGPS attendus.

SDKMAVROS d98e8e…/MAVROS2.16.0/commit5c68b905 reste épinglé ; Cyclone importé
comme validationOFF. Conteneur localmowgli-blade-off-validation-20261008 UID1000,
--networknone/sansdevice. Sourcesactuelles /tmp/blade-product-src,
build/install/log /tmp/blade-product-{build,install,validation.log}.
Scriptlocaltemp /tmp/run-blade-product-validation.sh conserve log etrc ;
ne pas utiliser un ancienrc comme preuve pendant une nouvelle exécution.

GNSS : prefixSDKlocal importé depuis image64027b… dans/tmp/blade-product-gnss-sdk.
DiffGnsStatus.msg : uniquement trois constantes manquantes ; messagesactuels
du sous-moduleUniversalGNSS383caba3… générés sans modification dans
/tmp/blade-product-gnss-msgs-install, levant le défaut de compilation.
Le graph nécessite aussi le producteurGNSS actuel, pas son ancienbinaireSDK.
Prochaine étape : reconstruire ROS2/MAVROS GNSS deps actuelles en/tmp, puis
rerun buildcomplet/testsbridge+ESC/provider/safety/format/static/installedgraph.
Le conteneur local a terminé son sleep3600 ; le relancer avant dockerexec,
ce n'est pas un containerrobot. Aucun codeUniversalGNSS modifié.

## Snapshot produit final — logiciel, non déployé

Disposition : RETAINED pour l'évidence logicielle ; acceptation nouveau binaire
sur robot HARDWARE_PENDING, perte totalecompanion NON RÉSOLUE.
Branchemain, HEAD9f62ba4b1afbccbdaa8b22899ecd4874bc28b204, patchnoncommité.
Fichiers : node/header, blade_control.hpp, confighardware_bridge_mavros.yaml,
CMake, test_blade_control.cpp, test_blade_graph.py, contratsOFF statiques et
graphprovider, README/TODO. ChangementsESC/tests/checkpoints précédents conservés.
L'ancienhelperOFF-only et ses tests restent historiques ; le node utilise
uniquement le nouveauBladeControl pour le contrôle produit.

Paramètresstartupread_only :channel3, neutral1500, forward1450, reverse1550,
validationrange1..32/1000..2000 avant narrowing, directions distinctes et de
part/d'autre du neutre. Valeurs d'acceptation àfaibleexcursion uniquement.
Aucune écritureFCU/VESC, aucun mapping déduit du RPM, aucun changementGUI/schema.

Intention, pendingRPC, commandeACK-confirmée et échec sont séparés. ON requiert
FCU connecté/déjàarmé/frais, mowing_enabled, hardware safety released/frais,
aucune urgenceactive/latched et ESCqualifié. AucunARM/DISARM dansmower_control.
OFF préempte unONenRPC sans attendre sonACK et invalide les retours anciens.
Une inversion passe par neutral, puis au moins5 mesureszéro distinctes après
sonACK, stabilitémonotone>=1 s et donnéesfraîches<=1 s. Legacy exigecompteur
valide/progressant ET stamp progressant ; COMMON exige unstamp nouveau.
Source inconnue, replay, metadata seule, PWM ou ACK ne prouvent pas un arrêt.
La cohérence de validité/source/count/RPM/stamp vient du même snapshotESC accepté.

ACK3 s et arrêt15 s sont bornés : surstale/timeout/rejet, intentionoff/FAILED,
neutralisation, jamaisPWMopposé. Une échéance de réponse20 s annule aussi tout
ON différé et garde l'échec latched ; pas de démarrage tardif après réponsefalse.
Servicesrépétés pending coalescés (32 réponsesmaximum), confirmés dédupliqués ;
pas de retryautomatique par tick après échec. La capacitéréponses ne peut pas
empêcher OFF de préempterON. Resetconnexion/safety explicite autorise une
nouvelle générationneutral sans confondre sesACK avec les anciens.

Startup, disconnect/reconnect, emergency, hardware safety engaged, doublelift,
tilt : neutral explicite ajouté ; plans existantsHOLD/DISARM/traction inchangés.
Une déconnexion ne garantit qu'une tentative de neutral, pas son effetphysique.
Status : blade_requested_direction représenteintentionoff/forward/reverse
(unknown réservé), mow_enabled uniquementONautorisé/ACK-confirmé sanstransition.
Transitions/failure/safety publientmowfalse ; RPM/courant/température/stamp restent
la projectionESC2 existante. Une lame encoast-down peut donc montrerRPMnonzéro
avec intentionoff ; aucun faux arrêt physique n'est publié.

Cache invalidé par connexion, commande183 externe observable unmatched (mêmePWM
compris) et changements distinctsrc/out. Limite importante : l'endpointMAVROS
partage identité émetteur entreclients. Une commande externe identique racing
unmessage local attendu reste indistinguable. La file de matchingwire bornée
est une déduplication de transport, PAS un contrat d'origineexclusive ; ne pas
présenter ce cache comme propriétéexclusive ou preuve physique. Qualification
d'autoritéexclusive/lease reste nécessaire avant revendicationproduction.

## Validation finale du snapshot

SDKlocal image d98e8e9899e57cdcfb9b3b1aec5588bd0c64fc31956dcc726ec5c31ca6825316,
MAVROS2.16.0 commit5c68b905ab30de6ce630822dc46c33467e8f23ea,
MAVLink2026.9.9, ROSLyrical, GCC15.2, ubuntuUID1000, x86_64.
Conteneur sansréseau/devices/volume robot :mowgli-blade-off-validation-20261008.
Cyclone importé comme validationOFF, sansinstallationhôte/apt.
SDKUniversalGNSSancien insuffisant : messages + ROS2/MAVROSproducer actuels
reconstruits SANS modification depuisUniversalGNSS383caba3de94e16167764393d5a4ef046078b015,
dans /tmp/blade-product-{gnss-msgs-install,ug-install}. Aucun codeUG ou gitlinkchangé.
Les anciensbinairesGNSSdonneurs ne sont pas utilisés comme preuve finale.

Buildcomplet : **6/6 packages PASS**, dernierbuild46.0 s.
CTests : **bridge16/16, ESC5/5, power1/1, adapterGNSS1/1 PASS**.
Inclusprovidergraph4variantesArduPilot/PX4×mowingenabled, graphblade actuel,
graphsafety/ESC/GNSS réels, 14gtests nouvel état et régressions existantes.
**23rapportsXML** finaux audités, aucunfailure/error ; pas de somme artificielle
des caswraps/gtest comme nombre de preuves physiques.
Les ancienséchecs restent enregistrés ci-dessus : mismatchSDK GNSS levé en
reconstruisant les producteurs actuels, attente neutral corrigée en testpur,
assertionESCintermittente nonmodifiée passée au dernierpassagecomplet.
Logs persistants /tmp/blade-product-validation.log et .rc=0, tests/build/install
sous/tmp/blade-product-{build,install}. Aucun ancienbinaire substitué.

Provenance finale sourceNode identiqueworkspace/SDK :
a8cbaee9bae1fe050a8479ec72e5100745a283003c30414e13901e06649584cf.
BladeControl.hpp identiqueworkspace/SDK :
206a8c272bce9815ed3c7e3a0235489cee0c381159df6b377872a88ff9a03014.
BinaireINSTALLÉ testé :
1d1bf5b479c10d3605cc9f78f39fd247b91c6661c33d374085cd58215b49845c.
Formatclang18 nouveauxC++ PASS, ligneschangéesnode/header PASS (diff vide),
diff--check PASS, syntaxegraphsPython etcontratsOFF statiques PASS.

Contrôle supplémentaire du binaireinstallé :provider4variantes PASS ; première
tentativegraphblade arrêtée par domaineDDS241 horsplageports, pas un défaut
produit. Relance surdomaine219 : **provider4/4 et graphblade PASS**, rc=0 dans
/tmp/blade-product-installed.log/.rc. Le binaire installé vérifié est bien celui
dont le SHA est indiqué ci-dessus, pas l'ancien OFF-only.

STOP software après cette vérification : aucunSSHrobot danscetteimplémentation,
déploiement/imageARM64, paramètreFCU/VESC, actuationphysique, commit/push.
Prochaine étape seulement après revue et nouvelleautorisation explicite :
buildARM64/déploiementcontrôlé du nouveaupatch, puis acceptationphysique sûre
startup/safety/inversions/retourzéro/échecs et cohérenceprofilVESC.
Ne pas généraliser le bancnatiffaiblePWM A/B au nouveaunode, à100% ou àune
configurationVESC modifiée. LeaseFCUtotalcompanion toujours NON RÉSOLU.
