# Validation Docker GNSS MAVROS — revue requise

Le correctif est committé **localement**, sans push ni publication :

- `7a2a1a0963886dd19f151681ccc4eae445600fc8` : les14 chemins approuvés, patch équivalent par fichier et SHA256.
- `61d3e37b4bc2f7081b317e955877bafcf2f0b5db` : complément Docker de deux lignes après défaut réellement découvert. Aucun commit réécrit. Ce second SHA est celui des images candidates et du contexte figé.

**PR MowgliNext #1017 / release v1.6 : NON PRÊTE À FUSIONNER.** Cette dépendance MAVROS est bloquante, pas un suivi indépendant. Les builds locaux ne publient pas une paire de release et ne lèvent pas HARDWARE_REQUIRED.

## Résultats réellement acquis

Les codes de sortie sont présentés sans transformer un échec en validation verte :

| Architecture | Build runtime | Suite C++/ROS | Smoke runtime Cyclone |
|---|---:|---:|---:|
| amd64 | 0 | 0 | 0 |
| arm64 | 0 | 0 | 0 |

Voir build-results.json, test-summary.json et les logs. Les résultats JUnit comptent224 cas par suite réussie, dont17 d'adaptateur GNSS, répartis dans26 fichiers ; le dashboard CTest ajoute ses26 inscriptions et ne doit pas être additionné comme de nouveaux cas. Le build principal exécute le nouveau contrôle des8 types et le gtest GNSS avant d'exporter l'image. La validation supplémentaire compile et teste les autres paquets, avec assertions historiques conservées.

AMD64 a aussi passé175 tests GUI GNSS existants et les tests de classification sur les messages réels du runtime. Les contrôles incluent source_id/source_incarnation opaques, position_observation_sequence inchangée, FIX/FLOAT disponibles, expiration no-fix/UNKNOWN/flags=0. Les tests sur deux architectures et leurs résultats exacts figurent dans les logs frontend. Aucun code GUI modifié. Les NaN de JSON ROS sont normalisés en null par le pilote de test, comme le contrat de transport GUI existant ; les exports bruts sont conservés. Cela n'est pas une preuve de navigation navigateur réelle sur robot.

Un test **entre les deux images réelles AMD64** a confirmé la réception de22 statuts par l'image MowgliNext de référence7489a4a6..., avec ses propres types installés, les identités non vides et le hash attendu. Domain223, boucle réseau partagée isolée, utilisateurs1000, aucun périphérique, entrypoints matériels non exécutés. États Fixed/Float/2D/3D/DGPS et périmé observés sur messages synthétiques. Pas de réception physique ni de nouveau candidat MowgliNext v1.6 inventé.

## Contrats générés dans les vrais artefacts

Les8 descriptions des images MAVROS construites sont comparées au verrou, au source MowgliNext actuel9686b375 et aux descriptions extraites par digest des couches de l'image MowgliNext7489a4a6... pour les deux architectures. Le hash GNSS attendu est **`RIHS01_ca48e4f1007c03b476e559280a5d9a5b1b03dd4061b2f8956aea93ad22a17fdf`**.

| Type | Hash ROS attendu |
|---|---|
| `msg/Emergency.msg` | `RIHS01_a9f717cea3855f92a7666cbeae778c059d4f7dbc8d5dc4b8d71d449f54560307` |
| `msg/GnssStatus.msg` | `RIHS01_ca48e4f1007c03b476e559280a5d9a5b1b03dd4061b2f8956aea93ad22a17fdf` |
| `msg/HighLevelStatus.msg` | `RIHS01_4c22623995ef23359ec365dd98b93a3f83e82c42cb8f5e938a6b17dce266652b` |
| `msg/Power.msg` | `RIHS01_912a9cb0e93d5b86dd7a45561c70e114902fec459c59ac41c257b7f3cb5f87fe` |
| `msg/Status.msg` | `RIHS01_9450476cd7696c7180f30030792287ebceee344a1bfd4304ed81abd7c3062086` |
| `msg/WheelTick.msg` | `RIHS01_a553ee260f2ad9b8d768f73924167b0cea46962bd2da76ff7c37dcb2aaaa5ec8` |
| `srv/EmergencyStop.srv` | `RIHS01_5a3f4f7da4f0e357b2602c0cc164948f457c6f50318fed886c6e54df11697d82` |
| `srv/MowerControl.srv` | `RIHS01_d38a990e3b41755a69a5877f414803d9f9f24c04532c6035b98cf31ff6e0ff7a` |

interface-comparison.json donne, pour chaque paire/architecture disponible, les hashes et les SHA256 des descriptions extraites. Les types et bibliothèques ne sont pas remplacés isolément dans un ancien plugin : tous les paquets sidecar sont reconstruits. Le runtime-smoke et le test interimages exercent les bibliothèques réellement chargées, pas seulement des labels.

## Défaut découvert et correction limitée

Sur7a2a1a0, les six paquets AMD64 compilent mais le contrôle post-build échoue : son interface-root par défaut pointe vers ros2/src alors que le Dockerfile a WORKDIR /ros2_ws et COPY ros2/src vers /ros2_ws/src. Ajouter explicitement `--interface-root src/mowgli_interfaces` résout le problème. RED puis GREEN conservés ; la régression du contrat Docker est renforcée et le nouveau commit61d3e37 ne modifie aucun contrôle moteur, watchdog ou garde GNSS. Anciennes exécutions7a2 conservées, aucune affirmation qu'elles avaient réussi. La tentative ARM7a2 a été annulée explicitement (130) pour ne pas dupliquer la compilation corrigée ; son défaut de chemin avait déjà été démontré en AMD64. Les deux premiers tests interimages ont échoué parce que docker start --attach avait perdu READY et le publisher n’avait pas démarré. Les logs du conteneur ont permis de corriger seulement la barrière de départ du pilote ; le recheck utilise les mêmes assertions et reçoit22 statuts.

## Limites des builds

Les images utilisent les mêmes sources committées, pins MAVROS2.16.0/5c68b905, UniversalGNSSd02e23f1 et MAVLink2026.9.9. Versions apt réelles et matériaux de build sont enregistrés ; apt et ros:lyrical-ros-base restent historiquement mutables. Contrat reproductible ne signifie pas binaire bit-identique.

Le Dockerfile committé compile historiquement en root. La recette temporaire livrée ajoute uniquement ownership/USER1000/cache UID-GID pour respecter la règle non-root ; toutes les commandes de compilation, contrôles et tests sont conservées. Le runtime garde ses instructions et utilisateur de production. Cette différence est explicite dans nonroot-recipe-61d3e37.diff et les SHA du contexte. Aucun build root n'est revendiqué comme validation du Dockerfile original octet pour octet. ARM64 est exécuté via QEMU intégré au builder, pas sur Rock5B ; aucune configuration binfmt hôte changée.

## Identités et publication

images.json distingue les IDs de configuration Docker, les digests de manifests exportés et les **digests GHCR encore inconnus**. Aucun des IDs locaux n'est substitué à un pin publié. Les candidats sont uniquement dans le moteur Docker local ; aucun tag distant, push Git ou workflow déclenché.

release-plan.md traite la paire comme un critère de #1017 : prochain GO pour publication MAVROS/ARM64, pin réel de l'index dans sensors/mavros/image.env, gitlink+registre de pins explicites si requis, sourcegate et imagegate, et descriptor apparié avant acceptation. La proposition MowgliNext est concrète et testée dans mowglinext-proposal/, mais **non appliquée** au dépôt principal pour respecter son gel et l'interdiction de modifier ses gitlinks dans cette opération. Le fichier de paire pending échoue volontairement tant qu'il manque un digest ; aucun fallback vers latest. Les refs synthétiques des tests ne sont pas des images publiées.

HARDWARE_REQUIRED reste ouvert. Avant réception passive sur Rock5B, enregistrer les deux digests de release finaux, sourceSHA/gitlinks, unité, firmware Pixhawk et récepteur, topologie/configuration ; opérateur présent, robot immobilisé, traction/lame désénergisées et arrêt physique disponible. Sans flash, moteur ou tonte : vérifier hashes endpoints, réception, tuple d'identité, progression/péremption/reconnexion et affichage réel cohérent avec les seuls états du récepteur. Aucun FLOAT/FIX artificiel et aucun résultat historique généralisé au nouveau candidat. Procédure détaillée et critères initiaux : ../gnss-interface-20261023/proposal.md.

Arrêt pour revue. MowgliNext, ses136 historiques,1827 preuves antérieures et trois sous-modules sont conservés ; preuve de vérification dans evidence/. Réserve Fleet inchangée. Aucun déploiement, SSH, flash, commande moteur ou suivi CI.

## État des validations complémentaires

Les175 tests GNSS frontend existants passent, puis6 assertions supplémentaires sur les exports réels AMD64/ARM64 passent (FLOAT/FIX/périmé et tuple). Le gate de paire proposé passe8 cas, y compris une dérive des constantes RTK dans une copie de fixture alors que les artefacts générés restent inchangés ; la source canonique est comparée, pas seulement le nom/type du topic. Le pending descriptor sans digest échoue explicitement comme prévu. Le wrapper CI d'extraction est vérifié en syntaxe ; aucune paire GHCR candidate n'a encore pu être exécutée puisqu'elle n'est pas publiée.

Les durées de clients BuildKit se chevauchent sur des étapes partagées : ne pas les additionner comme plusieurs reconstructions complètes. Les images locales ont été chargées sans publication. Le pull de l'image publique MowgliNext a d'abord été refusé avec la configuration Docker habituelle ; une configuration temporaire anonyme a permis la lecture par digest, sans modifier les identifiants existants. Les sept premiers tests du prototype de gate ont révélé une attente de fixture erronée (l'ancien sidecar n'installait aucun lock) : le cas a été précisé, le test de verrou incomplet conservé séparément ; aucun cas du produit n'a été affaibli.

## État Git final

HEAD61d3e37, index vide, aucun changement de production non committé. Les deux seuls fichiers suivis modifiés après build sont TODO.md et l'index des checkpoints, pour enregistrer la validation ; ils restent non committés pour revue, avec le nouveau checkpoint partagé et les rapports. Leur delta est validation-docs.diff. final.diff et source-sha256.json décrivent exactement le code committé qui a construit les images, pas ces métadonnées de bilan. Les rapports antérieurs176 fichiers et les historiques MowgliNext restent inchangés.
