# Proposition de correction et de publication

## Patch préparé

La copie autonome constitue la proposition reviewable ; le sous-module MowgliNext reste intact. Aligner le sous-ensemble de huit interfaces sur un commit canonique exact, enregistrer les hashes ROS générés et copier les deux chaînes producteur. Aucune interprétation GNSS, limitation, garde-fou, GUI ou firmware changés. Recompiler tous les paquets consommateurs du sidecar, pas seulement son paquet interfaces.

Le contrôle existant est étendu avec :

- `--source-root` : exige le commit exact et le contenu canonique ; fonctionne désormais aussi en mode check.
- `--peer-root` : compare les définitions du consommateur réellement candidat, sans confondre son commit GUI avec une révision d'interface.
- `--installed-root` : exige les huit descriptions/type hashes générés, absences et divergences bloquantes.

CI sidecar proposée : checkout sparse du commit canonique verrouillé, contrôle et tests Python avant build ; contrôle des types générés et 17 gtests de mapping dans le workspace Docker avant publication ; verrou installé avec le paquet et labels OCI source/fingerprint/hash. Les labels documentent le résultat ; ils ne remplacent jamais la lecture des types installés.

## Alignement durable des deux images

Le pin exact ne prétend pas suivre automatiquement le prochain contrat MowgliNext. Lors d'une évolution d'interface, la release MowgliNext doit bloquer sa publication appariée tant que le sidecar candidat ne passe pas la comparaison. Intégration MowgliNext à faire après revue et autorisation d'un changement de gitlink/workflow ; **aucun de ces changements n'est appliqué dans cette livraison**.

Exemple de gate à exécuter avant publication/déploiement d'une paire, avec trois racines vérifiées :

```bash
python3 tools/mowgli_interface_contract.py check \
  --peer-root /candidate/mowglinext/ros2/src/mowgli_interfaces \
  --installed-root /extracted-sidecar/ros2_ws/install/mowgli_interfaces
python3 tools/mowgli_interface_contract.py check \
  --installed-root /extracted-mowgli/ros2_ws/install/mowgli_interfaces
```

Les fichiers extraits doivent provenir des images adressées par digest, pas d'un workspace voisin ; comparer aussi les hashes des endpoints avec les bibliothèques réellement chargées lors d'un smoke DDS passif. Une erreur bloque la paire. L'ancien sidecar est rejeté par ces contrôles malgré son propre verrou cohérent. Ne jamais résoudre la divergence en revenant à l'ancienne IDL côté MowgliNext, en désactivant la provenance ou en inventant FLOAT/FIX.

Après GO distinct : commit revu de MowgliMAVROS, construction native amd64/ARM64 depuis ce commit, hash gate + tests avant publication, publication sous tag qualifié SHA, enregistrement du digest de l'index et du manifest ARM64. Valider ensuite la paire par digests ; mise à jour du pin/lock MowgliNext dans une opération explicite séparée. Un tag de branche/latest peut subsister comme alias informatif, pas comme preuve d'identité du déploiement.

Le Dockerfile conserve ses bases ROS/apt historiques mutables : cette proposition rend le **contrat** reproductible, sans promettre un build binaire bit-identique. Les digests de base et versions de paquets devront être conservés dans l'attestation du build candidat. Aucun remaniement de packaging ou de sécurité de publication indépendant n'est inclus.

## Identité de la future image

Voir `future-image.json` : base MowgliMAVROS, SHA256 des sources livrées, révision canonique, empreinte des huit interfaces, hash GNSS attendu, pins MAVROS/UG/MAVLink. Tag proposé `ghcr.io/pepeuch/mowglimavros/mowgli-mavros-sidecar:sha-<full-reviewed-MAVROS-commit>-lyrical`.

**Aucune nouvelle image OCI construite ou publiée ici. Aucun SHA de commit candidat, digest ARM64 ou index OCI futur n'est inventé.** Les champs restent null jusqu'au build autorisé. Les six paquets natifs amd64 et leur graphe sont validés, ce qui ne remplace ni le build complet Docker ni l'exécution ARM64. Ne pas remplacer l'image du robot sur la seule base du tag proposé.

## HARDWARE_REQUIRED — réception passive sur Rock 5B

Baseline logicielle avant essai : enregistrer les deux commits, index/manifests d'images par digest, trois gitlinks, OS/kernel Rock 5B, identité de l'unité, modèle/firmware Pixhawk, configuration ArduPilot et transport, récepteur/révision/pin Universal GNSS, source GPS sélectionnée. Ces identifiants physiques et l'image candidate n'ont pas été acquis : HARDWARE_REQUIRED, pas HARDWARE_PENDING.

Après autorisation distincte seulement, opérateur présent, robot immobilisé, lame et traction désénergisées, arrêt physique disponible, sans flash ou commande moteur : relever les types/hashes chargés aux deux extrémités et l'unicité du propriétaire `/gps/status` ; recevoir les champs producteur et observer séquence/stamps sur des positions réelles ; vérifier les transitions RTK uniquement si réellement déclarées par le récepteur. Un état non disponible reste inconnu. Contrôler passivement l'expiration lors d'une perte de télémétrie autorisée, puis une nouvelle observation/reconnexion ; vérifier le GUI sans fabriquer de statut.

Réussite : hashes identiques au verrou, statuts effectivement reçus, tuple et bits préservés, republication identique ne renouvelle pas une position, perte de télémétrie ne laisse pas un fix valide artificiellement frais, nouvelle incarnation identifiable, GUI cohérent avec les seules capacités/valeurs disponibles. Échec : mismatch, non-réception, identité perdue/fabriquée, séquence artificielle ou FLOAT/FIX sans preuve producteur. Aucune acceptation de déplacement/tonte ou de qualité de position absolue n'est déduite de ce contrôle. Aucun essai effectué dans cette mission.
