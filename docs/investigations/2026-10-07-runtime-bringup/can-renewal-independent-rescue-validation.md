# Capture renouvelée et secours indépendant — 2026-10-08

Préflight : validation passive uniquement, aucun ARM(true), aucun1550/1450,
aucun code produit ni paramètreFCU, aucun commit/push. Résultats à acquérir.
Submodule main9f62ba4b + patch préexistant, baseline robot OFF-only image
281e47a4fb7f18ca1bcd72fa2a52141241624bc81c2bd44611beb7ac0d1df913,
démarrage18:04:49.171997794UTC inchangé, rock-5b/pepeuchUID1000/aarch64.
Mappings et décodeur réutilisés de can-passive-results.md sans modification :
DTID1034, signatureA9AF28AEA2FBB254, CRC16, int18 signé/uint5, nodes1/2/3.

Outils uniquement temporaires dans /tmp/mowgli-off-only-20261008.BcGAvI,
montés readonly dans les conteneurs diagnostic, UID1000, sansdevice/socketDocker.
Étape1 : nouveau conteneur capture, filtreADD1034 surbus1, CAN_FORWARD1
renouvelé toutes2 s, fenêtre17 s après réception initiale des trois sources,
gardesdésarmement/neutres/RPMzéro et fraîcheur1034<=1 s inchangée.
Fin : CAN_FORWARD0 puis filtreREMOVE1034, drainage0,5 s et silence>=2 s.
Aucune commande servo/ARM/DISARM dans cette capture passive.

Étape2 : deux conteneurs distincts, namespacesPID/cgroups/démarrages distincts.
Principal : capture+renouvellement, heartbeatROS token spécifique+compteur
strictement croissant. Secours : abonnementROS et clientsMAVROS propres,
pas de pipe/fichierlogger du principal, pas d'enfant du principal ni socketDocker.
Doublonsheartbeat ignorés ; échéance2 s après dernière progression,
plafond75 s après acquisition. Loggersecours propre au conteneur Docker.
Seulement neutre183/3/1500, DISARMfalse, CAN_FORWARD0, retraitfiltre et vérification.
AgentSSH tue uniquement le conteneur principal diagnostic exactement identifié
après secoursready. Vérifier identité secours inchangée et stillrunning après
kill principal puis ACK/restauration/silenceforwarding et étatfinal.
MAVROS/bridge/sécurité et autresconteneurs produit ne sont jamais arrêtés.

Ce secours de banc ne prouve ni un lease produit ni la sécurité lors de perte
du Rock5B/Docker/MAVROS/liaisonFCU. SigneESC2 et comparaisoncharge toujours
HARDWARE_REQUIRED. STOP après ces deux validations, pas de rotation.

## Résultats acquis — deux PASS

Preuves compactes : `can-renewal-rescue-proof-2026-10-08.jsonl`.
Outils temporaires SHA256 :
- passive-can-rescue.py : 11e902e155964b4141e3d642cd99988bb8f097af4c58ae654fcc45a136104179
- mowgli_blade_charge_ab.py (décodeur réutilisé) : 132c95e6018e101fc03e085e42b53af7c011300ab919e307874514c3e1795d08

### Étape 1 — PASS

Conteneur diagnostic `mowgli-can-renewal-20261008`, exit0.
Capture commune surveillée : epochUTC1791485776.7916675 à environ
1791485793.7946732, durée monotone17.001402109 s.
Un démarrage CAN_FORWARD puis **8 renouvellements** success=true/result0,
aux offsets1.920 /3.950 /5.972 /8.020 /10.021 /12.026 /14.030 /16.056 s.
Les offsets sont relatifs au début de fenêtre, après l'acquisition initiale.
Intervalles de renouvellement proches de2 s ; aucune rupture commune vers5 s.

| Source | Transferts1034 complets CRC validés | Âge maximal surveillé | Écart maximal entre transferts validés | RPM min/max |
| --- | --- | --- | --- | --- |
| node1/index0 | 711 | 0.256838 s | 0.279305 s | 0/0 |
| node2/index1 | 744 | 0.239810 s | 0.279293 s | 0/0 |
| node3/index2 | 814 | 0.061633 s | 0.101029 s | 0/0 |

Fraîcheur vérifiée à chaque boucle, garde1 s non assouplie. Seulement les
transferts complets CRC-validés alimentent la mesure. La capture reste best-effort :
les comptes ne sont ni une preuve de zéro perte, ni une cadence exhaustive VESC.
Signature et découpage int18/uint5 inchangés. Aucun signe en rotation validé.
Flux COMMAND_LONG observé : seulement32000, aucun183/400 ni servo non neutre.
FCU désarmé, roues et coupeRPM0 pendant toute la fenêtre surveillée.

Fin normale : CAN_FORWARD0 acquitté, filtreREMOVE1034 envoyé. Après drainage0,5 s,
**aucune CAN_FRAME/CANFD_FRAME pendant2.200130224 s**. Retrait du filtre sans
ACK dédié ; la preuve d'arrêt est le silence après ACK de CAN_FORWARD0, pas
une prétendue lecture de table filtre.

### Étape 2 — PASS : survie réellement observée

Principal `mowgli-can-principal-20261008` :
ID08569da51e5203ce6df4be4815bba12a76e95248389de5a5743b107818a6ce32,
PID hôte46677, démarrage2026-10-08T18:56:59.872008382Z.
Secours `mowgli-can-rescue-20261008` :
ID9a6e481f73c2af100ed852f7958c7d4380133d256c1ada9f01417f07d15d7e19,
PID hôte46946, PID interne1, démarrage2026-10-08T18:57:03.668719109Z.
PidMode vide pour les deux : conteneurs distincts, pas de partagePID.
Secours exécuté directement comme PID1 de SON conteneur et non comme enfant
du principal ; logger Docker propre, clientsMAVROS propres, aucun pipe partagé.

Ready secours epoch1791485830.0902877, heartbeatseq85. Agent SSH revalide les
deux IDs/noms/Running/PIDs avant `docker kill --signal KILL` du SEUL principal.
Preuve aprèskill epoch1791485850.3209388 : principalRunning=false/exit137,
secoursRunning=true, PID46946 et StartedAt identiques. Il n'a pas été relancé
pour effectuer le nettoyage : la même instance a réellement survécu.

Perte de lease : dernier compteur561, déclenchement epoch1791485852.1527998,
âge2.061360765 s. Les répétitions d'un même compteur ne renouvellent pas la lease.
Séquence autonome du secours après mort du principal :

| Action | EpochUTC réponse service | Réponse / ACK brut |
| --- | --- | --- |
| DO_SET_SERVO183,3,1500 | 1791485852.1737137 | success=true/result0 ; ACK183/result0 |
| DISARMfalse | 1791485852.194443 | success=true/result0 ; COMMAND_LONG400,param1=0 et ACK400/result0 |
| CAN_FORWARD32000,param1=0 | 1791485852.2161272 | success=true/result0 ; ACK32000/result0 |
| Retrait filtre1034 | 1791485852.2164843 | message envoyé, pas d'ACK dédié |

Puis **aucune CAN_FRAME/CANFD_FRAME pendant2.220078327 s** après drainage0,5 s.
rescue_pass epoch1791485854.9375715, failures=[], finexit0 après vérification.
Aucun ARMtrue, aucune commande servo autre que183/3/1500. Le principal passif
se bornait à capture/renouvellement/heartbeat jusqu'à son SIGKILL.

### État final et limites

Au checkpoint final du secours :
- FCU connecté=true, armed=false, modeMANUAL ; rc/out1/2/3=1500/1500/1500.
- ESC0/1/2 valid=true, rpm_valid=true, RPM0/0/0.
- Courants0.01/0.02/0 A ; tensions25.95/25.85/26.04 V.
- Compteurs35282/42195/35303 ; bridge actif, mow_enabled=false.
- Emergency inactive/non latched, HLS IDLE ; traction publiée nulle.
- is_charging=false et charger_enabled=false ; aucun essai de charge A/B.

MAVROS sidecar, ros2, gui et gps sont toujours actifs, StartedAt inchangés
(sidecar18:04:49, autres15:24:52 UTC). Aucun arrêt/redémarrage produit.
Conteneurs diagnostics terminés et logs conservés, seul principal sacrifié.
Aucun paramètreFCU écrit, code produit modifié, build produit, commit ou push.
Syntaxe outil PASS, gitdiff--check PASS. Changements préexistants conservés.

Ces deux PASS lèvent les prérequis capture et survie au SIGKILL du conteneur
principal sur cette baseline. Ils ne démontrent ni l'arrêt d'un moteur en rotation
par le secours, ni la survie à perte de Docker/Rock5B/MAVROS/liaisonFCU.
Failsafe total companion NON RÉSOLU. L'ancien script A/B n'est pas rendu sûr
par simple présence de ces nouveaux fichiers : avant reprise, intégrer la
capture renouvelée et le secours distinct au protocole de banc, vérifier ses
gardes/restauration et obtenir la confirmation opérateur requise.
STOP ici : aucun1550/1450 et aucune acceptation signeESC2 ou conclusioncharge.
