# Imported physical acceptance section — historical

Original Next docs/validation/gui-idle-blade-rollback-20261011/README.md; source SHA256 e8576bf0f48b94997bc140c31d9f466fea490ec30aa560996637e7630560925b.
Source execution baseline: Next7748af56acd98449938750714d2286193cf6143a, backend8eecc57.
No physical acceptance was acquired during this reorganization.
The historical Next blade policy remains authoritative. No idle-ON requirement is added.
Current backend gates: [FCU architecture](../../architecture/fcu-session-controller.md).

## Preserved original section

<!-- source: MowgliNext:docs/validation/gui-idle-blade-rollback-20261011/README.md; lines 84-97; sha256 e09eddae28f333a700926e75e85236ad338512fe640ea2e6c7e883f98fa9b3eb -->

## Limites physiques inchangées

HARDWARE_REQUIRED pour le backend8eecc57 : confirmation des modes/ARM avant les
sorties attendues dans les sessions BT existantes, signes/unités GUIDED, ordres
DISARM et garantie indépendante d'arrêt en cas de perte totale du companion.
Aucune validation de démarrage GUI au repos n'est exigée par cette décision.
Le baseline physique doit identifier unité robot, Pixhawk/image/paramètres,
VESC/canaux, MAVROS/OS/topologie/configuration et binaires exacts. Essais futurs
séparément autorisés : lames retirées, propulsion isolée/confinée, opérateur
qualifié et coupure physique indépendante ; vérifier la neutralisation au repos,
les modes/ARM confirmés avant actuation BT, OFF/arrêts/fin de session et perte de
liaison avec mesure indépendante. Les critères exacts et le délai d'arrêt doivent
être établis avant le run ; la garantie companion-loss demeure non établie.
Aucune disponibilité production n'est déclarée sur ces seules preuves logicielles.
