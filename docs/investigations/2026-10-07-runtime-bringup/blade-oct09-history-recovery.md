# Retrouver le patch lame du 9 octobre — 10 octobre 2026

## Conclusion

Le patch n'est pas perdu : il est conservé dans **refs/stash**, objet
`c93548f36d12549653bc35e26d2d33bf558f76eb`, intitulé
`On main: wip-blade-fwd-rev-before-dev-sync`.
Il n'est intégré à aucune branche publiée vérifiée. Aucun commit de livraison
du patch n'a été trouvé. Techniquement le stash est lui-même un commit Git à
trois parents ; dire « jamais commité » sans cette précision serait inexact.

Le code est absent du worktree actuel ET de l'image active sur le robot.
Ce n'est donc pas simplement une image en retard sur une branche qui contiendrait
déjà la fonctionnalité. Une archive locale, une archive sur le robot et l'ancienne
image ARM64 la conservent également. Aucune réintégration n'a été effectuée.

## Baselines et recherche effectuée

- Parent MowgliNext : `feat/mavros-refresh`, HEAD
  `582e6fe96eabaa45d5381fc7f27dbdab7f62ed22`.
- Sous-module : `main`, HEAD `9f62ba4b1afbccbdaa8b22899ecd4874bc28b204`.
- Branches locales/distantes, log --all, reflog --all, stash et fsck
  --full --no-reflogs --unreachable examinés dans le sous-module.
- Aucune autre version orpheline du patch : fsck du sous-module ne signale
  qu'un blob vide non accessible, aucun commit non accessible.
- Parent : reflogs, deux stashes et fsck également examinés. Son stash
  `32272986b9a76b8c0629c4cdcbf6c3a981d37bfd` du 9 octobre 11:02:27 UTC
  ne contient dans son diff suivi qu'AGENTS.md ; ce n'est pas le patch du bridge.
  Les commits non accessibles du parent examinés sont antérieurs au travail lame.
- `git ls-remote --heads origin` vérifie les cinq branches publiées, sans fetch.
  `main=9f62ba4…`, `feat/mavros-refresh=1aa2616…` ; différence `0 / 4`.
  Les quatre commits supplémentaires n'incluent pas le patch du stash.
- Clone robot `/home/pepeuch/mowglimavros` : `feat/esc-odometry`,
  HEAD `8e29b196f3c1c31979cb437f76f8b01866194466` ; reflogs/branches/fsck
  ne révèlent aucun patch du 9 octobre. Ce checkout n'identifie pas la source
  du binaire actuellement exécuté.

Limite : cette recherche couvre les objets disponibles et les branches publiées
du dépôt connecté, pas d'éventuelles copies privées sur d'autres machines.

## Chronologie prouvée

| Date UTC | Événement |
| --- | --- |
| 7 octobre 13:32:01 | `7466cc77e4439db4b72c99d5e2a6d241c9932cb2` : ancien contrôle, encore `send_arm_command(blade_authorized)` |
| 8–9 octobre | Développement OFF-only puis OFF/FWD/REV sur main9f62ba4 avec changements non committés, d'après checkpoints retrouvés |
| 9 octobre 07:23:26 | Création de l'image ARM64 produit `788d5831…` ; checkpoints de déploiement/startup/OFF et refus FWD conservés |
| 9 octobre 11:02:35 | Création du stash `c93548f…`, immédiatement suivie dans le reflog de `reset: moving to HEAD` vers 9f62ba4 |
| 10 octobre 12:43:52 | Démarrage du sidecar actuel avec image `cfb4c921…`, différente de l'image produit du 9 octobre |

Le reflog démontre une sauvegarde avant nettoyage du worktree, pas la destruction
du patch par un merge. Il ne précise pas la commande ou l'opérateur ayant ensuite
remplacé la sélection d'image : cette causalité reste non établie.

Le fichier source du bridge de 7466cc77 est identique à celui du HEAD9f62ba4
(SHA256 `580f5ab7d538bb2910896c00809703c34826cb6f6e3873c9d92df6889f60dc8f`).
Récupérer les quatre commits de main ne rétablirait donc pas le nouveau contrôleur.

## Où se trouve le snapshot complet

- Premier parent du stash : base `9f62ba4…`.
- Arbre du stash : versions modifiées des neuf fichiers suivis.
- Troisième parent `45d7740…` : nouveaux headers, tests, checkpoints et preuves
  alors non suivis. Un simple diff du stash sans son troisième parent serait
  incomplet et oublierait notamment `blade_control.hpp`.
- Archive locale `/tmp/blade-product-package-20261009.tar` et snapshot extrait
  uniquement en `/tmp/disarm-audit-20261010/mowgli_mavros_bridge` pour revue/tests.
- Archive/sources robot `/tmp/mowgli-blade-product-20261009.GZZlAs`.

Empreintes égales entre stash, archive locale et snapshot robot :

| Source | SHA256 |
| --- | --- |
| Node du patch | `a8cbaee9bae1fe050a8479ec72e5100745a283003c30414e13901e06649584cf` |
| BladeControl.hpp | `206a8c272bce9815ed3c7e3a0235489cee0c381159df6b377872a88ff9a03014` |

## Diff présenté, non appliqué

Voir [diff exact source/tests et changements suivis](blade-oct09-recovered-review/blade-product-exact-stash.diff).
Il combine `git diff c93548f^1 c93548f` et les sept nouveaux fichiers de code/tests
du troisième parent, sans réécriture : 16 fichiers, 1496 insertions, 27 suppressions.
Il inclut les modifications documentaires suivies et le test signé ESC préexistant ;
ce dernier n'est pas à attribuer au seul contrôleur de lame.
Les autres nouvelles preuves historiques restent dans le stash, non appliquées
par-dessus les rapports du 10 octobre. Deux checkpoints historiques sont copiés
à l'identique dans le sous-dossier de revue.

| Chemin | HEAD / ancien bridge | Patch retrouvé |
| --- | --- | --- |
| mower_control OFF | `send_arm_command(false)` | COMMAND_LONG183, canal/PWM configurables, baseline3/1500 |
| mower_control ON | ARM si autorisé, sinon DISARM | FCU déjà armé requis ; direction0/1 vers1450/1550 sur cette baseline |
| Réponse service | Acceptation locale de l'envoi ARM | Réponse différée selon ACK, refus/échéances ; pas preuve d'effet physique |
| Inversion | Pas de commande lame indépendante | Neutralisation, observations zéro distinctes/fraîches et coast-down >=1s |
| Safety | HOLD/DISARM existants | Neutralisation explicite ajoutée AVANT les politiques existantes |
| Status | `mow_enabled` intention locale | Intention `blade_requested_direction`, état ON ACK-confirmé/autorisé |

Le patch sépare bien les commandes, ne modifie pas le GUI et garde les mappings
configurables. Il ne résout pas le failsafe FCU lors d'une perte totale du companion.
Le matching des commandes sortantes ne prouve pas une propriété exclusive lorsque
deux clients envoient une commande identique. Ce sont des limites connues, pas
des raisons de réinventer le code sans revue.

## Image et binaire réellement exécutés

Revalidation SSH en lecture seule : pepeuchUID1000, hostname rock-5b,
Armbian26.8.3 / Debian13.6, PWD `/home/pepeuch`.

| Artefact | SHA256 / digest |
| --- | --- |
| Image active, démarrage10 octobre12:43:52.639646551Z | `cfb4c921cc2ea23cf05b45843b8941af7a838ddf18a02091eeacc0dd460acf07` |
| Binaire actif `/ros2_ws/install/mowgli_mavros_bridge/lib/mowgli_mavros_bridge/mavros_hardware_bridge_node` | `13f26d44d1d08465bb5d7ccefe271a9db6f82a602bf24cc3384c21c9ba643f2b` |
| Ancienne image produit toujours présente `blade-product-20261009` | `788d583129ffeec5cebd660cff1ddfe15357bae272b7741bf7be76bdba0759e1` |
| Binaire ARM64 conservé sous `/tmp/.../build/mavros_hardware_bridge_node` | `47527d460563bbb4656a328cb78e772a7d3ce1fd7f9b1e6787e303bd4b03f22c` |

L'image active est référencée par `latest`, mais son digest n'est PAS celui du
patch. Les labels inspectés ne fournissent aucun commit source du bridge ; aucun
SHA source n'est déduit arbitrairement du tag ou du clone robot. Aucun source
node/BladeControl n'a été trouvé sous `/ros2_ws` dans l'image active.
Le Dockerfile historique copie uniquement le nouveau binaire, deux headers et
la configuration dans l'image OFF-only ; il est conservé sur le robot.

Les logs production relus passivement montrent `SetMowerEnabled` OFF à environ
100ms d'intervalle. Ils corroborent le producteur BT et le chemin source ancien,
en plus des 600 DISARM/60s de l'audit conservé. Aucune commande service/moteur
n'a été envoyée pour cette recherche. La provenance exacte de chaque RPC reste
un volet de l'audit DISARM à poursuivre, pas une acceptation du nouveau patch.

## Tests du patch retrouvé

Nouvelles vérifications du 10 octobre, uniquement sur l'archive locale :

- Compilation C++17 UID1000, `-Wall -Wextra -Wpedantic -Werror` : PASS.
- `test_blade_control` : **14/14 PASS** ; déduplication, FWD/REV, deux inversions,
  rejeu, stale, délais ACK/service, préemption OFF, safety/reconnexion, paramètres.
- `test_blade_off_contract.py` : **2/2 PASS**, absence ARM/mode dans mower_control
  et transport neutre183 sans fallback ARM.
- `git apply --check` du diff de revue : PASS avant les ajouts documentaires
  de cette recherche, sans application. Après mise à jour TODO/index, le check
  reste PASS en excluant uniquement ces deux fichiers documentaires désormais
  divergents ; le diff complet ne s'applique plus tel quel sur leur nouveau texte.
  Ce résultat ne vaut ni revue sémantique ni build intégré.
- `git diff --check` : PASS. Aucun C++ produit n'a été édité ou reformaté.

Preuves historiques, NON réexécutées ici :
[checkpoint logiciel original](blade-oct09-recovered-review/blade-product-fwd-rev-implementation.md)
documente build6/6, bridge16/16 suites, ESC5/5, power1/1, GNSS1/1,
graphs provider/blade sur binaire installé et format18 PASS.
[checkpoint déploiement original](blade-oct09-recovered-review/blade-product-arm64-deployment.md)
documente startup/OFF cible PASS mais **premier FWD refusé**, sans1450/1550 émis ;
REV/inversion/safety cible non exécutés. Ne pas transformer ces tests logiciels
ou startup/OFF en acceptation physique complète.

## Disposition et suite

**RECOVERED_REVIEW_ONLY** : pas de reset, merge, cherry-pick, stash apply/pop,
fetch, commit, push, déploiement ou redémarrage. Tous les fichiers d'audit du
10 octobre gardent leurs empreintes initiales. Seuls les nouveaux documents de
revue/checkpoint et la navigation documentaire sont ajoutés.

Prochaine étape après décision opérateur : revoir ce patch existant et ses limites,
puis envisager une réintégration sélective avec les nouveaux tests demandés
(répétition10Hz et compatibilité Mowgli). Ne pas cherry-pick le commit de stash :
ses nouveaux fichiers résident dans un parent distinct. La préparation d'un build
et toute acceptation cible doivent rester séparées ; aucun ordre matériel ici.
