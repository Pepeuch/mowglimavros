# Analyse — dérive GNSS MAVROS, 23 octobre 2026

Le défaut est confirmé dans les fichiers installés de l'image fournie, sans accès au robot. Deux corrections sont nécessaires : aligner le contrat ROS généré et recopier les deux identités dans l'adaptateur. Changer seulement les définitions laisserait les identités vides.

## Baselines et propriété

MowgliNext : `fix/gui-warnings-errors@9686b37556f1d2ba7f7a295eb006f870c86e8095`, inchangé. MAVROS embarqué : `4673f43cb6b2b6b9df0685d36f8f8bb43e6e1456`, propre et inchangé ; c'est aussi le main amont consulté. Travail préparé dans un dépôt autonome, branche `review/gnss-interface-20261023`, même base. Aucun sous-module interne ni `.gitmodules` dans MowgliMAVROS. Le Dockerfile clone séparément MAVROS et Universal GNSS à des commits exacts ; le gitlink MowgliNext ne met pas à jour une image déjà publiée.

## Provenance exacte de l'image en défaut

L'index OCI actuel `latest` lu une fois est exactement `sha256:c57fd39322bbbdc74c7a2744efd0346623ded03866d08e7ec99675da0b2f068c`, fourni par l'opérateur. Les manifests/configs et attestations BuildKit référencés par cet index donnent, pour les deux architectures :

- source `Pepeuch/mowglimavros@4673f43cb6b2b6b9df0685d36f8f8bb43e6e1456` ;
- [build 39382305156, tentative 1](https://github.com/Pepeuch/mowglimavros/actions/runs/39382305156/attempts/1), le 20 octobre, de 06:46 à 07:04 UTC ;
- MAVROS `2.16.0@5c68b905ab30de6ce630822dc46c33467e8f23ea` ; Universal GNSS `d02e23f17d34e58f75f4c7a7f72f8d34ea766bfa` ; MAVLink `2026.9.9` ; ROS Lyrical ;
- paquet installé `mowgli_interfaces` **0.1.0**, sous-ensemble de huit interfaces issu de MowgliNext **`e789ccc2ecc249377c385e79b34fb48ff5a90927`**. La version package seule ne distingue pas les contrats.

Les couches workspace amd64 et ARM64 ont été téléchargées par digest et vérifiées SHA256 ; leurs `GnssStatus.msg`, IDL, en-têtes et descriptions JSON installés confirment tous deux **`RIHS01_6765a75d0f9bc5858eea44f685f485335e8c877b770f35d1474ca08e1ad5285a`**, sans les deux champs. Voir `image-provenance.json` et `evidence/reference/`. L'attestation est une preuve de provenance adressée par le digest OCI fourni ; aucune vérification de signature de release n'est revendiquée.

L'autre index fourni, `sha256:7489a4a68b1544a8b545d6bbc1673b7d8393340fb2ec8f65f42e13361613a706`, a été lu par digest : ses configs identifient MowgliNext `59051b41b5f288d737e482791155a47ebc0f4d61`, builds du 22 octobre. Son filesystem complet n'a pas été téléchargé ; le hash observé sur robot est corroboré par la génération native de la même définition et par les types MowgliNext installés localement.

## Pourquoi latest reste ancien

MowgliNext a ajouté les champs au commit **`1e1af89902c1969ea6df136bbdc6204146521e8f`**, le **20 octobre à 21:44:33 UTC**, après la construction du sidecar ce matin-là. Le contrat de MAVROS est une copie vendue et verrouillée dans `ros2/src/mowgli_interfaces/interface-contract.lock.json`, pas une dépendance automatiquement synchronisée. Parmi les huit définitions requises, seule `msg/GnssStatus.msg` diffère actuellement. Empreinte agrégée ancienne : `f3dd10cbbfbe64e1ae5e2e7412b58fefe3903d1593c61965f823e5cdb4117f20`.

L'ancien `tools/mowgli_interface_contract.py check` valide uniquement la copie contre son propre verrou. Son argument `--source-root`, pourtant accepté, n'était vérifié que par `sync`. Une copie ancienne cohérente passait donc le contrôle Docker même avec le chemin du consommateur actuel. La preuve `baseline-false-assurance.log` conserve ce PASS trompeur. Relancer le même Dockerfile au même pin n'aurait pas suffi.

Le workflow `.github/workflows/docker.yml` publie main sous `latest`/`lyrical`, avec images multiarchitecture, sans comparaison au consommateur réel ni test de propagation des identités. Le déploiement MowgliNext autorise des aliases indépendants (`docker/.env.example`, `install/compose/docker-compose.mavros.yml`). Ce sont des mécanismes distincts : mise à jour du code, construction, publication et choix de l'image déployée.

## Chaîne de données vérifiée

Le nom `/mavros/mowgli_gnss` désigne le nœud du plugin ; son topic canonique par défaut est **`/gps/status`**, et non un nouveau contrat privé portant le nom du nœud. `gnss_adapter_plugin.cpp:39-65` conserve les souscriptions privées SensorDataQoS, la sortie reliable/volatile/depth-10 et le timer existant. `direct` ne crée aucun publisher canonique. Aucun topic, QoS ou seuil modifié.

| Champ | Traitement conservé/corrigé |
|---|---|
| `source_id`, `source_incarnation` | Ajout exact du contrat MowgliNext à `GnssStatus.msg:177-181`, copie opaque à `status_mapping.hpp:105-106`. Vide reste vide ; pas d'identité reconstruite ni d'autorisation implicite. |
| `position_observation_sequence` | Copie exacte, jamais incrémentée par l'adaptateur. Changement d'incarnation réinitialise le suivi existant ; répétition d'une séquence ne renouvelle pas la position. |
| `fix_type` | Mapping symbolique de neuf cas, dont 2D/3D/DGPS aux valeurs volontairement différentes entre UG et Mowgli ; inconnu devient no-fix. |
| `rtk_mode` | Mapping symbolique UNKNOWN/NONE/FLOAT/FIXED depuis la source ; aucune extrapolation depuis le GUI ou la qualité. |
| `capability_flags` | Mapping symbolique des 23 bits pris en charge ; capacités inconnues abandonnées. |
| `value_flags` | Mapping de `input.value_flags & input.capability_flags` ; un bit sans capacité ne devient jamais disponible. |

`adapter_state.hpp:51-89,146-160` reste byte-identique : durée de trois secondes sur horloge monotone, stamp de position conservé lors d'enrichissements RTK, séquence inchangée à l'expiration, no-fix/UNKNOWN/valeurs vides après expiration, aucune position synthétique. Les identités ajoutées restent celles de l'observation initiale sur les sorties périmées. Aucun timeout ajouté pour masquer une absence d'identité.

Dans MowgliNext, le tracker ROS de fraîcheur distingue séquence/stamp/temps de livraison ; il n'est pas transformé ici en nouveau contrôle de session basé sur les chaînes. Le GUI conserve le statut canonique brut (`gui/pkg/providers/transform.go:340-347`) et copie déjà les identités lors de l'adaptation d'un statut UG privé (`:398-400`). Les types Go/CDR et TypeScript possèdent les champs. `gnssTelemetry.ts` vérifie les types et les masques ; les identités y sont actuellement des champs passifs, pas une nouvelle preuve d'acquisition. Les gardes existantes ne sont ni désactivées ni revendiquées plus fortes qu'elles ne sont.

Le test DDS inter-préfixes utilise le vrai plugin MAVROS et un processus abonné avec les types installés MowgliNext : réception de 20 statuts, tuple producteur non vide intact, types FIXED/2D/3D/DGPS et sortie périmée no-fix/UNKNOWN/flags=0 ; hash de l'endpoint **`RIHS01_ca48e4f1007c03b476e559280a5d9a5b1b03dd4061b2f8956aea93ad22a17fdf`**. Voir `cross-mowglinext-consumer.log`. Les tests Go existants vérifient ensuite l'adaptation JSON et le tuple CDR vers le GUI. Ce sont des preuves complémentaires, pas une capture de bout en bout du navigateur du robot.
