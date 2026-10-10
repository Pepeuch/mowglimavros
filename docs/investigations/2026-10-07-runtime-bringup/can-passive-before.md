# Capture CAN passive autorisée — avant exécution

Autorisation opérateur explicite CAN_FORWARD temporaire filtré ESC Status1034,
sans ARM ni commande moteur. Aucun changement câblage ou paramètre FCU,
aucune suspension bridge/MAVROS. Exécution bornée15 s, arrêt CAN_FORWARD
param1=0 en finally, plus expiration native sans renouvellement.
Filtre CAN_FILTER_MODIFY388 limité1034 avant démarrage, retiré après arrêt.
Aucune CAN_FRAME émise vers le bus ; seules observations386 reçues.
FCU doit rester connecté/désarmé, troisESCà0RPM au préflight.

Checkpoint agent historique relu MM-ESC-ODOMETRY-20261005 : droiteESC0/RPM1,
gaucheESC1/RPM2 signedF+/R- établi sur TYPE5/masks1,2/scaling1 et image
2032c3d... du5octobre. Réutiliser cette preuve sans généraliser à l'image
actuelle cfb4c921... ni répéter un essai powered inutile.
Stock MAVROS esc_status exigeINFO pour tableauSTATUS et a un problème d'échelle
température ; le plugin direct existant contourne déjà ces deux défauts.
Capture présente diagnostique uniquement disponibilité et identité source au
repos ; zéro signé ne valide PAS les sens. Réassemblage des transferts1034,
conservation nodeID/index/transferID/payload brut, comparaison226/11030/observation.
Pas de changement de normalisation ni de fallbackPWM.
