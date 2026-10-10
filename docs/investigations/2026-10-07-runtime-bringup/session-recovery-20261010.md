# Réintégration complète de la session du 9 octobre — 10 octobre 2026

## Périmètre et dépôt

Récupération depuis `c93548f36d12549653bc35e26d2d33bf558f76eb`, sans stash pop,
reset, merge sur branche existante, commit/push, commande FCU ou redémarrage.
Main choisi : `9f62ba4b1afbccbdaa8b22899ecd4874bc28b204`, identique à la base
du stash et au main vérifié lors de l'audit de récupération.

Le produit est préparé dans un **vrai dépôt Git MowgliMAVROS autonome**,
origin `https://github.com/Pepeuch/mowglimavros.git`, avec objets non hard-linkés
à la copie MowgliNext : `/tmp/mowglimavros-blade-reintegration-repo-20261010`.
Son worktree de session est `/tmp/mowglimavros-session-recovery-20261010`,
branche nouvelle `recovery/full-session-20261010`, HEAD inchangé9f62ba4.
Le précédent worktree lame seul est conservé séparément, sans être le livrable.
La copie d'audits sous MowgliNext et l'ancien clone dirty
`/tmp/mowglimavros-current` restent intacts. Cette préparation n'est PAS encore
appliquée à ces clones ni au checkout du robot. Rien n'a été publié à GitHub.

## 1. Inventaire exhaustif

Parents du stash :

| Rôle | Objet |
| --- | --- |
| Base | `9f62ba4b1afbccbdaa8b22899ecd4874bc28b204` |
| Index | `cd43c9cef79435dc849d5e3d4060c0de5fb64fc6` |
| Non suivis | `45d7740a35fe2b7e61f7f7a693a0ae661686c744` |

L'index est identique à la base : **0 changement staged**.
Arbre worktree du stash : **9 modifications**, toutes unstaged ; aucune
suppression, aucun renommage, aucun nouveau fichier suivi à ce stade.
Troisième parent : **38 nouveaux fichiers**. Total : **47 fichiers utiles**,
pas seulement le contrôleur de lame.

[Inventaire fichier par fichier](session-recovery-inventory-20261010.csv) indique
la fonction, la couche Git, la catégorie, les états main/feat/current-audits,
la décision et la destination de chaque fichier.

| Catégorie exclusive | Nombre |
| --- | ---: |
| Sources/headers produit | 4 |
| Configuration | 1 |
| Enregistrement CMake des tests | 2 |
| Tests unitaires, contrats et graphs mocks | 6 |
| README contrat | 1 |
| Rapports historiques | 21 |
| Données de preuve JSON/JSONL/CSV | 8 |
| Checkpoints | 2 |
| Index de checkpoints | 1 |
| TODO | 1 |
| Total | 47 |

Les47 changements ne sont intégrés ni dans main ni dans feat/mavros-refresh.
Cette dernière est quatre commits derrière main. Les éléments de main qui
diffèrent déjà de cette branche sont conservés : pas de retour à l'ancienne
branche pour appliquer le stash. Les autres fichiers inchangés du dépôt ne
sont pas remplacés par une archive ancienne.

## 2. Archives ARM64 et outils complémentaires

[Inventaire de l'archive package](session-arm64-archive-inventory-20261010.csv) :
40 fichiers dans `blade-product-package-20261009.tar`, dont35 sources/config/tests
et5 caches Python générés. Les35 fichiers correspondent au snapshot stash/base ;
les fichiers déjà dans main restent main. Les5 caches sont classés obsolètes,
non importés dans le produit et restent récupérables dans l'archive d'origine.

Douze fichiers auxiliaires locaux du9octobre (Dockerfile, build/validation,
préflight/capture, intégrité, déploiement/rollback) sont préservés dans
[historical-tools-20261009](historical-tools-20261009/README.md), hors du compte47.
Ils ne sont ni installés ni exécutés. Les scripts d'actionnement/déploiement
ne constituent pas des outils actuels autorisés ; leurs baselines et chemins
privés sont historiques. Aucun .env privé, secret ou binaire ARM64 n'est importé.

L'image historique ARM64788d5831… et son binaire47527d46… sont distincts de
l'image activecfb4c921…/binaire13f26d44… vérifiés dans l'audit précédent.
Ce travail ne change aucune de ces images. Le stash original reste accessible
dans le dépôt source et sous refs/recovery/blade-oct09 dans le dépôt autonome.

## 3. Réconciliation et préservation

**41 des47 fichiers restaurés sont identiques, blob Git à blob Git**, au stash.
Six fichiers nécessitent une adaptation documentée : INDEX, TODO, README,
node bridge, test_blade_control.cpp et test_blade_graph.py.
Les38 nouveaux fichiers sont tous restaurés, avec une seule destination
historique différente pour éviter de remplacer le rapport plus récent.

Conflits/évolutions traités :

- `mavlink-runtime-frequency-audit.md` : le stash contient le9octobre ; le worktree
  récent contient aussi le10octobre. Le premier est restauré à l'identique sous
  `mavlink-runtime-frequency-audit-20261009-original.md`. La version récente
  conserve son nom et ses preuves10octobre. Les CSV/JSON9octobre originaux ET
  les fichiers précédemment reconstruits sont conservés sous leurs noms distincts.
- TODO/index : préserver les ajouts historiques et ajouter la situation actuelle,
  sans déclarer l'ancien déficit1Hz encore actif ni revendiquer le déploiement
  de la version réconciliée. Le checkpoint de récupération du10octobre est conservé.
- README : corriger la chronologie devenue historique et préciser les deux
  adaptations de sécurité ci-dessous. Pas de schéma ROS ni de GUI changé.
- Sources/tests : les différences historiques de bridge/config/provider sont
  réutilisées. Pas de réécriture de BladeControl ni de normalisation spéciale ESC2.
- Aucune suppression utile, aucun conflit source main/stash non résolu, aucun
  gitlink modifié. Les scripts de banc anciens sont classés historiques plutôt
  que promus à des points d'entrée opérationnels.

La copie d'audits source n'a pas été éditée ; les empreintes des rapports/données
du10octobre sont inchangées. Les nouvelles données importées sont des copies,
pas des déplacements, et les preuves9octobre originales restent dans le stash.

## 4. Contrôle lame, DISARM et adaptations nécessaires

Chemin ancien : SetMowerEnabled::tick / FollowStrip::setBladeEnabled / service
BladeControlService → MowerControl → send_arm_command(blade_authorized).
OFF ou ON non autorisé provoque donc DISARM. Le BT peut réémettre à10Hz ;
useManualMode a un joystick10Hz mais pas de keepalive lame périodique.
Les logs passifs et l'audit précédent établissent la rafale ; ils ne donnent
pas une identité DDS par requête pour attribuer chaque RPC individuellement.

Chemin récupéré : mêmes producteurs/contrat → BladeControl → CommandLong183,
channel3/PWMneutre ou directionnel configurable. Aucun ARM/DISARM/mode depuis
mower_control ou son driver. ARM reste une précondition globale extérieure.
Les répétitions OFF et ON refusé n'appellent donc plus le DISARM global ;
les mocks vérifient cette propriété. Cela ne vaut pas une mesure après
déploiement, puisque le produit du robot n'a pas été changé ici.

Les chemins emergency/hardware-safety/double-lift/tilt gardent les politiques
DISARM/HOLD existantes et ajoutent un neutre explicite prioritaire. Une nouvelle
demande de safety invalide le cache OFF et n'est pas supprimée par déduplication.

Deux adaptations ciblées, non présentes dans le stash :

1. **OFF pendant absence du service CommandLong** doit annuler une intention ON
   différée avant de retourner false. Le stash rejetait trop tôt cette requête.
   La neutralisation reste en attente pour la reprise du service ; une mise en
   file n'est jamais annoncée comme ACK, et l'ancien ON ne doit pas repartir.
2. **Âge d'acquisition ESC réel** : la lame exige <=1000ms côté stamp ROS avant
   d'admettre une observation dans son suivi monotone. Le tracker d'affichage
   générique a un délai3s ; il ne doit pas rendre saine une acquisition retardée.
   Identité source/stamp/count et fraîcheur monotone restent séparées, sans
   allonger les délais pour masquer un défaut de provenance.

Tests ajoutés/enrichis : OFF et ON refusé à10Hz, nouveau neutre safety malgré
cache OFF, absence d'autorité ARM dans ce service, disparition/reprise de service
pendant inversion et acquisition ESC2 retardée avec compteur avançant.
Une attente du graph ancien basée sur le nombre de commandes est remplacée
par l'étape explicite de transition ; aucune assertion de neutralisation supprimée.

OFF/FWD/REV et inversion gardent le contrôleur existant : passage au neutre,
ACK, >=5 acquisitions zéro distinctes/fraîches et >=1s coast-down. Ni PWM ni
ACK ni metadata seule ne prouvent l'arrêt. Watchdog ESC1s, heartbeat2.5s,
hardware safety3s, ACK3s, attente arrêt15s, réponse service20s ; aucun départ
opposé sur simple expiration. Une indisponibilité VESC/stale/rejet annule ON
et provoque une tentative de neutre. Le Status distingue intention et ACK ;
success du service est acceptation du FCU, PAS activation physique prouvée.

## 5. Mappings et limites physiques

Preuves historiques sur Rock5B/Pixhawk5X, ArduRover4.7.1light dbe79216… :
SERVO1_FUNCTION74 → roue droite/ESC0node1 ; SERVO2_FUNCTION73 → roue gauche/
ESC1node2 ; SERVO3_FUNCTION0 → coupe/ESC2node3. MIN/TRIM/MAX1000/1500/2000,
CAN_ESC_BM7/RV7/OF0. Le banc natif a établi DO_SET_SERVO183/channel3,
neutre1500,1450 sens mécanique avant et1550 arrière sur CE profil VESC.
Paramètres du patch configurables, aucune affectation inventée ni écrite au FCU.

Ces excursions sont faibles, pas100%. Le banc natif A/B et les sens physiques
ne constituent pas l'acceptation du binaire produit réconcilié. Le produit du
9octobre avait startup/OFF PASS, puis FWD refusé avant toute sortie non neutre.
REV/inversions/safety sur cible restent HARDWARE_REQUIRED. Aucun nouvel essai ici.

Deux limites bloquent une revendication de sécurité production complète :
failsafe FCU à perte totale Rock5B/Docker/MAVROS non résolu ; une commande externe
identique peut être indistinguable dans la fenêtre du matching MAVROS partagé
(pas de preuve d'ownership exclusif). Le watchdog host ne résout pas ces cas.
Le transport legacyESC11030 reste unsigned avec direction_valid=false ; les
tests signés rétablis prouvent que COMMON conserve le signe et que legacy ne
l'invente pas. Ils ne transforment pas la télémétrie réelle en RPM signé.
La calibration métrique ticks_per_meter343 reste provisoire.

## 6. Validation

SDK local épinglé image d98e8e9899e57cdcfb9b3b1aec5588bd0c64fc31956dcc726ec5c31ca6825316,
MAVROS2.16.0 commit5c68b905…, ROS2Lyrical. Build/test UID1000, networknone,
aucun device/socketDocker/transportrobot dans le banc. Dépendances GNSS actuelles
et Cyclone récupérées depuis la validation historique sans changer leurs sources.

- Compilation initiale6/6 packages PASS ; sources réconciliées, pas vieux binaire.
- Tests purs BladeControl16/16 et contrats statiques2/2 PASS.
- Interface publique3/3 et external backend10/10 PASS.
- Provider graph quatre variantes, graph lame enrichi et graph safety PASS
  au passage intermédiaire ; relance finale après garde d'âge ESC en cours.
- ESC5 suites, power1 et adapter GNSS1 PASS au passage intermédiaire.
- Première attente graph lame incorrecte après enrichissement corrigée ; graph
  ESC inchangé a passé une fois et rencontré ensuite l'intermittence connue
  left_raw_ticks. Cette intermittence est conservée comme WARN, pas masquée.
- Format clang18 fichiers nouveaux + lignes node/header modifiées PASS,
  git diff --check PASS ; AST des outils Python et bash -n des scripts historiques
  PASS. Aucun outil historique d'actionnement/déploiement exécuté.

La première tentative de banc Docker a échoué avant build, car /tmp du client
ne correspondait pas au /tmp du daemon pour les bind mounts. Résolu par copie
des sources/deps dans un conteneur local isolé ; aucun processus produit affecté.

### Dernière passe — PASS logiciel, WARN historique conservé

Build6/6 ; bridge16/16 suites, ESC5/5, power1/1 et adapterGNSS1/1 PASS.
Les23 rapportsXML finaux ont zéro failure/error. La suite bridge finale dure
130.07s ; elle inclut le graph enrichi avec indisponibilité service et âge
réel d'acquisition, provider4variantes et safety/GNSS/ESC. Contrats3+10 PASS.
L'intermittence ESC du passage précédent reste WARN, malgré le dernier PASS.

SHA256 source node final (identique au snapshot effectivement compilé) :
`dbd60f82dd350ce4390f13e50845eca1a7d2f6062d3bcf280f9605ffee95d4b9`.
Binaire installé SDK x86_64 validé, NON déployé :
`322070cb42ec80a106216383348b21a4c713ebfbe930d6e028aba3157d8ef610`.
Voir [preuve compacte](session-recovery-validation-20261010.json).

Diff final complet, produit/tests/documents/données/scripts nouveaux inclus :
`/tmp/mowglimavros-session-validation-20261010/full-session-final.diff`.
Vue code/tests/config seule : `product-tests-final.diff` dans le même dossier.
Ces artefacts sont générés avec un index temporaire distinct ; l'index réel
des worktrees reste non staged. Aucun nouveau commit n'est créé.
Le diff complet couvre78 fichiers : les47 du stash, les preuves/outils récents
et les nouveaux inventaires/checkpoints de cette récupération. Il inclut les
fichiers non suivis, contrairement à un simple git diff du worktree.
git apply --check PASS sur un quatrième worktree propre au même HEAD ; aucune
application ni modification de son code. Aucune entrée de suppression/gitlink.

## Suite autorisée

STOP après présentation des preuves et du diff : aucun commit/push ou déploiement.
Revue opérateur de la réconciliation complète, particulièrement des deux
adaptations, puis décision distincte pour intégration/publi ou acceptation cible.
Ne jamais déduire l'acceptation physique VESC des graphs logiciels.
