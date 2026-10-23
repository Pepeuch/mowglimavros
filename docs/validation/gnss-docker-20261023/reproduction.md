# Reproduction locale — pas de publication

Les commandes exactes sont conservées dans evidence/*-result.json. Les sources du build proviennent uniquement de git archive61d3e37, jamais du working tree après rédaction du bilan. Les recettes complètes exécutées sont dans evidence/recipes/ ; la différence non-root au Dockerfile committé est nonroot-recipe-61d3e37.diff.

Sur le même environnement Linux/Docker, en utilisateur normal : créer un contexte temporaire à partir de git archive61d3e37, copier les trois recettes de validation dans sa racine, ainsi que evidence/capture_gnss_graph.py. Créer un builder docker-container isolé disposant de QEMU embarqué, sans installation binfmt hôte. Exemple, depuis ce contexte, sans --push :

```bash
docker buildx build --builder <builder> --platform linux/amd64 \
  --file Dockerfile.validation --tag <local-amd64> --load .
docker buildx build --builder <builder> --platform linux/arm64 \
  --file Dockerfile.validation --tag <local-arm64> --load .
docker buildx build --builder <builder> --platform linux/amd64 \
  --file Dockerfile.software-validation --target validation-results \
  --output type=local,dest=<results-amd64> .
docker buildx build --builder <builder> --platform linux/arm64 \
  --file Dockerfile.software-validation --target validation-results \
  --output type=local,dest=<results-arm64> .
```

Exécuter les suites complètes séquentiellement entre architectures si elles partagent le namespace réseau BuildKit : les tests historiques utilisent des domaines DDS fixes. Le runtime-smoke utilise un domaine distinct219, l'interimages223. Ne jamais lancer l'entrypoint matériel par défaut d'une image ; les tests créent leurs propres nœuds et une liaison MAVLink synthétique loopback.

Pour les interfaces, créer un conteneur **sans le démarrer**, entrypoint /bin/false et networknone, extraire /ros2_ws/install/mowgli_interfaces avec docker cp, puis supprimer uniquement ce conteneur de test. Exécuter tools/mowgli_interface_contract.py check --installed-root <prefix> et --peer-root <actual-MowgliNext-source>/ros2/src/mowgli_interfaces. Comparer aussi chaque description générée issue de l'autre image, par son digest exact. Les8hashes sont dans interface-comparison.json.

Les images, SDK et tarballs ne sont pas copiés dans ce rapport. Les versions apt/matériaux exacts observés sont archivés ; le Dockerfile historique ne garantit pas de reconstruction binaire bit-identique à une date future. Pour déployer il faudra un nouveau GO et les digests réellement publiés, pas les IDs locaux de images.json.
