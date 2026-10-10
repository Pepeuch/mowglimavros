# SERVO3 — résultat essai isolé du 7 octobre 2026

## Conclusion limitée à la baseline ci-dessous

SERVO3_ESC2_BLADE_CONFIRMED

DO_SET_SERVO(3,1550) et DO_SET_SERVO(3,1450), FCU armé en MANUAL,
modifient la sortie3 et font réagir uniquement la télémétrie ESC2.
ESC0/1 restent à 0 RPM. L'identification manuelle antérieure associait ESC2
au moteur de coupe. Ce résultat valide le mapping d'actionnement observé,
PAS la sécurité d'exploitation ni le contrat produit complet.

Aucune capture directe des trames RawCommand DroneCAN : le segment interne
SERVO3→RawCommand est confirmé par l'effet end-to-end, pas par analyse du bus.
Les RPM sont les valeurs rapportées par la télémétrie legacy ArduPilot ; le
sens et la conversion éventuelle en vitesse mécanique restent non démontrés.

## Baseline et autorité

SSH pepeuch UID1000 sur rock-5b, PWD /home/pepeuch, Armbian26.8.3/Debian13.6.
Robot 192.168.10.32 ; essai UTC 2026-10-07 21:15:17→21:15:51.
MowgliNext robot9fb33e59 ; checkout local4c461ba3 ; submodule local
9f62ba4b1afbccbdaa8b22899ecd4874bc28b204.
Image mowgli-mavros sha256:cfb4c921cc2ea23cf05b45843b8941af7a838ddf18a02091eeacc0dd460acf07,
démarrage2026-10-07T18:04:18.7915975Z. Pixhawk5X ArduRover4.7.1 dbe79216.
PID46 bridge, identité starttime2392666, exécutable précisément vérifié ;
MAVROS PID45 laissé actif. Watchdog de restauration indépendant PID1403,
prêt avant SIGSTOP, limite75 s, libéré uniquement après reprise vérifiée.

Autorisation opérateur : robot immobilisé, coupe sécurisée, E-stop physique
accessible, charge interrompue. Relecture paramètres (aucune écriture) :
SERVO1_FUNCTION74 ; SERVO2_FUNCTION73 ; SERVO3_FUNCTION0 ;
SERVO3_MIN1000/TRIM1500/MAX2000. Aucune commande roues non nulle,
aucun cmd_vel publié, aucun changement de mode, code, paramètres ou container.
Uniquement ManualControl neutre entretenu, commandes183/3 et ARM/DISARM explicites.

## Mesures

ACK de chaque CommandLong183 : success=true, result=0.
ACK bruts MAVLink command183/result0 également capturés pour1550 et1450.
ARM et DISARM : success=true, result=0.
ARM stable au moins2 s au neutre avant1550, puis stable pendant les deux impulsions.
MANUAL conservé ; connected=true.

| Phase | rc/out sorties1,2,3 | RPMmax ESC0/1/2 | Courantmax A ESC0/1/2 | Tension V ESC0/1/2 |
|---|---|---|---|---|
| 1550, environ3 s | 1500,1500,1550 | 0 / 0 / 4990 | 0,03 / 0,03 / 0,90 | 27,42–27,59 / 27,26–27,39 / 27,46–27,60 |
| 1450, environ3 s | 1500,1500,1450 | 0 / 0 / 4784 | 0,03 / 0,01 / 0,90 | 27,43–27,57 / 27,21–27,35 / 27,43–27,59 |

ESC2 dernière observation pendant1550 : 4990RPM, 0,60A, 27,46V.
Pendant1450 : 4784RPM, 0,64A, 27,46V.
Compteurs ESC2 avançants :24258→24415 positif,24787→24924 négatif ;
13 observations ESC2 par impulsion, validités RPM/courant/tension true.
Aucune réaction RPM des roues mesurée pendant ARM ou les impulsions.

Chronologie UTC :
- 21:15:17,037 :183/3/1500 accepté, sortie neutre déjà relue.
- 21:15:17,535 :SIGSTOP du seul PID46 ; état T.
- 21:15:22,570 :aucune requête400 ARM/DISARM parasite dans la fenêtre2 s
  après drainage3 s ; neutre1500 accepté de nouveau.
- 21:15:22,710 :ARM explicite accepté ; stable au neutre à21:15:25,102.
- 21:15:25,133 :1550 accepté ;21:15:28,173 retour1500 accepté.
- 21:15:35,633 :sorties toutes1500, troisESC à0RPM, retour ESC2zéro
  validé pendant au moins1 s et5 observations distinctes.
- 21:15:35,652 :1450 accepté ;21:15:38,674 retour1500 accepté.
- 21:15:45,995 :sorties toutes1500 et troisESC à0RPM, même validation.
- 21:15:46,014 :neutre final1500 accepté.
- 21:15:47,654 :troisESC zéro relus, courant0A, tensions27,56/27,35/27,57V.
- 21:15:47,676 :DISARM accepté.
- 21:15:49,690 :SIGCONT du mêmePID46 ; reprise process R puisS.
- 21:15:49,801 :réception status après SIGCONT, FCUdésarmé,
  sorties1500/1500/1500, troisESCzéro ; watchdog libéré.
- 21:16:11,252 :contre-vérification passive5 s : HLS IDLE, bridge actif,
  chargefalse, mowfalse, cmd_vel etManualControlzéro, FCUdésarméMANUAL,
  sorties1500/1500/1500, RPM0/0/0, courant0/0/0A,
  tension27,56/27,37/27,59V, compteurs26527/26817/26528.

Les délais7,49 s (positif) et7,34 s (négatif) vont de la demande1500 jusqu'à
la validation logicielle complète du zéro, incluant réception rc/out et
au moins1 s/5 observations ESC2zéro. Ce ne sont pas des mesures exactes du
temps mécanique de coast-down. Aucune inversion directe ni1000/2000.

## Anomalie de sécurité importante

Après suspension, HLS passe IDLE→EMERGENCY avant ARM et reste EMERGENCY
pendant les impulsions, alors que la sécurité physique FCU (bit15 présent et
enabled), le modeMANUAL, l'armement, les sorties et la télémétrie permettent
l'actionnement natif. La garde du superviseur n'exigeait HLS IDLE qu'au
préflight : elle n'a pas interrompu l'essai sur ce changement. Cela constitue
une limite explicite du test, PAS une validation d'un contournement en produit.
HLS retourne IDLE après reprise, vérifié passivement.

Le diagnostic mavros:System indique niveau2 Sensor health pendant l'essai ;
source ESC niveau0 ardupilot_legacy. La cause exacte de ces états n'est pas
établie par cette capture. Aucun nouvel essai ne doit déduire de ce mapping
que l'autorité EMERGENCY peut être ignorée : prévoir un chemin d'autorité et
un protocole d'isolation sûrs avant tout autre actionnement.

L'arrêt des commandes DISARM après suspension, suivi d'un ARM stable,
confirme le conflit d'autorité du bridge dans cette baseline. Cela ne prouve
pas que le chargeur était la cause du blocage précédent ; chargefalse ici,
sans essai comparatif chargeactive.

## Acceptances restantes — HARDWARE_REQUIRED

Sens physique >1500 et <1500 : réponse opérateur attendue, aucune déduction
du signe RPM legacy. Deadband précis inconnu ; ±50 suffit déjà à produire
environ4800–5000 RPM rapportés, pas une justification d'essai plus puissant.
Aucune valeur100% nominale validée.

La simple cessation DO_SET_SERVO n'a PAS été testée, ni un watchdog/lease
produit. L'arrêt observé résulte d'une vraie commande1500 ; le watchdog de
ce test assure sa restauration, pas celle du futur bridge. Ne pas supposer
que le FCU revient au neutre lors de la disparition du bridge.

Prochain protocole à définir sur cette même baseline : autorité sécurité
cohérente FCU/HLS, arrêt autonome sur perte de commande puis sens physique
avec opérateur et moteur sécurisé ; pass uniquement si ESC2 revient à zéro,
ESC0/1 restentzéro, aucun ARM/non-neutre n'est accepté hors autorité.
Pas d'essai physique supplémentaire autorisé par cette conclusion seule.

Aucun commit/push. Seuls les checkpoints dans ce dossier ont été écrits.

