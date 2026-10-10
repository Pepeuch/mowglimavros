# Déploiement OFF-only — préflight du8octobre2026

Autorisation : déployer uniquement le patch validé, puis vérifier OFF/ONrefusé,
déduplication et étatfinal. AucunFCUparam/FWD/REV/essaimoteur/commit/push.
Sourcepatch9f62ba4b noncommité ; validationlogicielle dansblade-off-build-validation.md.
Source nodeSHAa687fe84cb715080f2787a0e5cca86fa9093c36f26787ac675f1350ea1347ca8.

RobotSSHpepeuch UID1000, rock-5b/aarch64, Armbian26.8.3/Debian13.6,
kernel7.1.8-edge-rockchip64, PWD/home/pepeuch.
Sidecarancienimagecfb4c921cc2ea23cf05b45843b8941af7a838ddf18a02091eeacc0dd460acf07,
start2026-10-08T15:24:52.1600688Z ; MAVROS45/bridge46 actuellementroot.
Le build sera exécuté exclusivementUID1000 dans un conteneur sansdevice/réseau,
pas dans le processusrobotroot. MAVROSinstallé2.16.0.
Headerbridge installéSHA612eb19c07138e945b8816f643db135f5a16d77374fa2480b0175049de3f93e5,
identique au HEADlocal avantpatch. AncienbinaireSHA
13f26d44d1d08465bb5d7ccefe271a9db6f82a602bf24cc3384c21c9ba643f2b.
Clonehistorique/home/pepeuch/mowglimavrosHEAD8e29b196 : pas utilisé/modifié.

Déploiementdurable visé : image dérivéeFROMdigestancien, nouveauxfichiers
bridgeuniquement (binaire, headers/configOFF) ; pas de plugins/launch/GUI/interfaces
nouveaux. ServiceComposeexactmavros depuis
/home/pepeuch/mowglinext/docker/docker-compose.yaml, imagevarMAVROS_IMAGE.
Recréation contrôlée --no-deps du seul sidecar, étatdésarmé/RPMzéro avant.
Sauvegarder ancienchoiximage/config avant miseàjour, restaurer si runtimeéchoue.
Aucuneautrecontainer ni processFCU affecté en dehors du redémarragesidecar
nécessaire au déploiement. Vérifier réglages effectifs identiques saufimage.

ObserverMAVLinksource/sink AVANT démarrage bridge pour capturer premièreOFF183.
BTpeut appelerOFF spontanément : distinguer premièreOFF deconnexion et OFF
explicites déjàdédupliqués. ONservice doitfalse, pas183nonneutre/400.
ObserverauprèsFCU state/rcout/ESC0..2 et statusbridge ; aucun ordreARM/DISARM
oucmd_vel du vérificateur. Contrôlerfinal1500/1500/1500 ettroisRPMzéro.
Si premierOFF provientduBT, le notifier plutôtque l'attribuer au lecteur.
Résultat non encore acquis à la création. STOP après runtime, pasESC2signé.

## Résultat final — OFF_ONLY_RUNTIME_PASS

Déploiement et validation effectués le 2026-10-08 sur le robot décrit ci-dessus.
Les mentions « actuellement » et « non encore acquis » du préflight décrivent
l'état AVANT déploiement, pas l'état final. Aucun commit/push effectué.

### Artefact et périmètre réellement déployés

- Source : submodule main `9f62ba4b1afbccbdaa8b22899ecd4874bc28b204`
  avec le patch OFF-only non commité décrit dans `blade-off-only-patch.md`.
  La validation logicielle complète précédente est dans
  `blade-off-build-validation.md` ; elle n'est pas une validation physique.
- Compilation native ARM64 du seul target `mavros_hardware_bridge_node`,
  sous pepeuch UID 1000, dans un conteneur isolé sans réseau ni device.
  Base : image robot originale. SDK MAVLink header-only 2026.9.9 repris de
  l'environnement épinglé déjà validé. Aucun build root.
- Le build de tous les targets de ce package dans cette base runtime manque
  `asio.hpp` pour le détecteur firmware préexistant. Ce détecteur n'a été ni
  modifié, ni remplacé. Le target bridge natif a compilé avec succès ;
  `CMAKE_SKIP_RPATH=ON`, ELF ARM64 et liaisons dynamiques vérifiés. Un contrôle
  annexe `file` absent a donné exit 127 APRÈS compilation réussie ; ce n'est pas
  un échec de compilation. Aucun résultat de tests ARM64 complet revendiqué.
- Nouveau binaire SHA256 :
  `ebf5dfd600c147899e802a71ca29706fdfc82b1f7e065ea4779c7c04cc760b26`.
- Image locale robot `mowgli-mavros-sidecar:off-only-20261008`, ID :
  `sha256:281e47a4fb7f18ca1bcd72fa2a52141241624bc81c2bd44611beb7ac0d1df913`.
  Dérivée de l'image originale sans remplacer MAVROS, plugins, interfaces,
  détecteur, launch ou code traction/safety. Seuls le binaire bridge, son header,
  le nouveau header blade_off_control et sa config OFF-only sont remplacés.
- Déploiement durable : seule la ligne `MAVROS_IMAGE` de
  `/home/pepeuch/mowglinext/docker/.env` pointe maintenant vers ce tag local.
  Recréation `docker compose up -d --no-deps --pull never --force-recreate mavros`
  du seul sidecar. Aucun dépôt historique robot utilisé comme source ou modifié.
- Dossier de travail robot : `/tmp/mowgli-off-only-20261008.BcGAvI`.
  Sauvegarde privée : `/tmp/mowgli-off-rollback-20261008.WVhWhb`, mode 0700,
  contenant l'ancien `.env`, inspect et identités des autres conteneurs.
  Ces fichiers peuvent contenir des secrets : ne pas les publier.

### Capture et distinction des générations

Observation ROS indépendante, sans accès aux devices et sans client ARM/mode,
sans publication cmd_vel, MANUAL_CONTROL ou MAVLink brut. Seuls les appels
autorisés MowerControl OFF et ON inhibé ont été émis, plus des lectures de
paramètres ROS. Flux observés : `/uas1/mavlink_sink`, `/uas1/mavlink_source`,
`/mavros/state`, `/mavros/rc/out`, observations ESC et status bridge.
Le décodage COMMAND_LONG distingue 183 et 400 ; ACK 183 observé sur le flux
entrant. Cela est une preuve au point MAVROS, pas une capture électrique CAN.

Les premières captures ont été insuffisantes : indisponibilité temporaire du
service au redémarrage, puis première OFF déjà dédupliquée avant la capture.
Elles ne sont pas utilisées comme preuve du premier paquet. Leurs anciens
ACK/commandes 400 concernent le bridge AVANT déploiement, pas les appels de la
capture finale. Aucun contournement du contrôle FCU n'a été effectué.

Le sidecar a été recréé deux fois pour la capture ; son dernier démarrage est
`2026-10-08T18:04:49.171997794Z`. Ensuite, pour observer le premier OFF avec
MAVROS déjà découvert, seul le bridge a été relancé : SIGINT PID 45, sortie
vérifiée, puis même binaire/argv/environnement au PID 135. MAVROS PID 44 est
resté actif pendant cette dernière opération. Aucun SIGSTOP, ARM ou test moteur.
Ce remplacement ponctuel est un enfant lancé hors du suivi de l'ancien enfant
roslaunch ; l'image/Compose assurent le lancement normal au prochain démarrage
du sidecar. Aucun nouveau superviseur n'a été ajouté.

L'observateur a identifié le changement de publisher status par GID :
`01106dc3d1b59431e853bb3900002803` vers
`0110595f19a50214c209bf3c00002803` à epoch UTC `1791483292.1501353`.
Le premier OFF est arrivé avant les appels explicites du vérificateur,
compatiblement avec les appels OFF existants du BT. Il n'est pas attribué
artificiellement au premier appel manuel. Le libellé interne `before_deployment`
de cet événement reste celui d'avant la lecture des paramètres ROS : c'est
bien la NOUVELLE génération, établie par GID et horodatage.

### Valeurs mesurées dans la capture finale réussie

| Vérification | Résultat |
| --- | --- |
| Paramètres ROS du nouveau bridge | canal 3, neutre 1500 |
| Premier OFF de génération | un COMMAND_LONG 183, param1=3.0, param2=1500.0, source MAVLink 255/191 |
| Horodatage émission UTC epoch | 1791483293.4006612 |
| ACK entrant | commande 183, result=0, epoch 1791483293.5043368 |
| Premier OFF explicite du vérificateur | success=true ; aucun nouveau 183, déjà dédupliqué |
| Dix OFF explicites supplémentaires | tous success=true ; aucun nouveau 183/400 |
| ON direction 0 et direction 1 | success=false pour les deux ; aucun 183/400 |
| Consigne servo non neutre | aucune dans les fenêtres OFF/ON observées |
| ARM/DISARM lié aux appels OFF/ON | aucun COMMAND_LONG 400 observé dans leurs fenêtres |
| Stabilité finale | dix contrôles sur environ cinq secondes ; PASS epoch 1791483306.9796927 |

État final télémétrique de cette capture :

| Signal | Valeur |
| --- | --- |
| FCU connected / armed / mode | true / false / MANUAL |
| rc/out sorties 1/2/3 | [1500, 1500, 1500] |
| ESC0 RPM / courant / tension | 0 / 0.0 A / 26.030000686645508 V |
| ESC1 RPM / courant / tension | 0 / 0.0 A / 25.920000076293945 V |
| ESC2 RPM / courant / tension | 0 / 0.0 A / 26.100000381469727 V |
| ESC valid / rpm_valid | true / true pour les trois |
| Compteurs finaux ESC0/1/2 | 41729 / 47263 / 41744 |
| Compteurs au début de la dernière capture | 35175 / 40637 / 35191 |
| Bridge | actif ; status mow_enabled=false |

Les compteurs progressent et les callbacks sont reçus récemment : il ne s'agit
pas seulement d'une valeur zéro historique réaffichée. Ces mesures attestent
l'état publié dans cette fenêtre, pas le sens mécanique ou la capacité FWD/REV.
`is_charging=true` était publié par le bridge ; aucun arming tenté et aucune
conclusion nouvelle sur la sécurité du dock tirée de cet indicateur.

### Intégrité et fin de passage

Contrôle post-validation : sidecar running=true sur le digest ci-dessus.
Environnement conteneur, Entrypoint, Cmd, mounts, NetworkMode, IpcMode,
Privileged et Devices identiques à l'inspect pré-déploiement.
Identités ET StartedAt des conteneurs mowgli-ros2, mowgli-gui et mowgli-gps
identiques au pré-déploiement : aucun autre conteneur redémarré.
Compose YAML inchangé, SHA256 :
`cf5b58328421f59301634b2499d468501be8092204d54c7d0be46f5af4d517fd`.
Aucun paramètre FCU écrit ; aucune modification de politique globale ARM/DISARM,
aucune consigne non neutre, aucun essai ESC2 signé, aucun FWD/REV implémenté.

Rollback préparé, NON exécuté : restaurer le choix d'image précédent depuis
la sauvegarde privée, sélectionner le digest original conservé localement,
recréer uniquement mavros avec --no-deps, puis contrôler désarmement/neutres
et reprise du bridge. Ne pas utiliser un pull latest pour retrouver le baseline.

Conclusion : OFF_ONLY_RUNTIME_PASS sur CE robot, CE binaire et cette image.
Le contrat OFF neutre/déduplication/ON refusé est validé runtime ; les limites
du patch minimal restent celles de `blade-off-only-patch.md`, notamment absence
de lease FCU et de garantie physique d'arrêt par le seul ACK. Failsafe perte
totale du companion NON RÉSOLU ; acceptation des sens ESC2 HARDWARE_REQUIRED.
STOP après cette validation. Aucun déploiement supplémentaire, commit ou push.
