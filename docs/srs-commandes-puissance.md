# SRS – Gestion des commandes de puissance

## 1. Introduction

Ce document fixe les exigences d'envoi, d'acquittement et de surveillance des commandes de puissance entre le Teensy de contrôle et le Teensy capteurs du banc d'essai moteur (Zénith).

**Portée :** lien RF entre les deux Teensy, logique applicative côté Teensy capteurs, et retour d'état vers le Teensy de contrôle et le Raspberry Pi. Le code complet du Teensy de contrôle est hors portée, sauf les exigences qui le visent explicitement.

**Convention :** « doit » = obligatoire. Chaque exigence a un identifiant unique.

| Terme | Définition |
| --- | --- |
| Teensy contrôle | Teensy 4.1 émetteur des commandes, relié au module RF |
| Teensy capteurs | Teensy 4.1 du banc qui pilote le moteur et lit les capteurs ; module RF branché en SPI |
| Module RF | nRF24L01, avec ack matériel, retransmission automatique et CRC (Enhanced ShockBurst) |
| Commande de puissance | Paquet portant une consigne absolue de puissance (0–100 %) |
| seq | Numéro de séquence `uint32_t` attaché à chaque commande |
| last\_applied\_seq | Dernier seq appliqué par le Teensy capteurs |
| ARC | Auto Retransmit Count : nombre maximal de retransmissions (0–15) après l'envoi initial, si aucun ack matériel n'est reçu |
| ARD | Auto Retransmit Delay : temps d'attente entre deux retransmissions, de 250 µs à 4000 µs par pas de 250 µs |
| MAX\_RT | Interruption du nRF24L01 levée quand les ARC retransmissions ont toutes échoué |
| k | Nombre de paquets consécutifs non reçus tolérés avant arrêt |
| Timeout | Durée sans commande valide déclenchant l'arrêt |
| Arrêt sécuritaire | Puissance à 0 et système désarmé, verrouillé |

## 2. Format et contenu des commandes

Les commandes portent un état complet et absolu, pour que les doublons et retransmissions soient sans effet.

| ID | Exigence | Justification |
| --- | --- | --- |
| CMD-01 | Une commande de puissance doit contenir une valeur absolue (ex. 40 %), jamais relative (ex. +5 %). | Idempotence : un doublon ou une perte ne fausse pas la consigne. |
| CMD-02 | Chaque commande doit porter un seq de type `uint32_t`, incrémenté à chaque envoi. | Pas de débordement avant \~6,8 ans à 20 Hz. |
| CMD-03 | La validité d'une commande (valeur dans 0–100 %, format correct) doit être vérifiée sur les deux Teensy : le Teensy contrôle ne doit pas envoyer une commande invalide, et le Teensy capteurs doit rejeter toute commande invalide reçue. | Défense en profondeur : le contrôle bloque les erreurs de saisie ou de logique, le capteurs bloque ce qui passerait quand même. |

## 3. Transmission et heartbeat

Le Teensy contrôle envoie la consigne en continu à 20 Hz ; ce flux sert aussi de heartbeat.

| ID | Exigence | Justification |
| --- | --- | --- |
| TX-01 | Le Teensy contrôle doit envoyer la commande de puissance courante à une fréquence fixe de 20 Hz (période 50 ms), même sans changement de consigne. | Le flux continu sert de heartbeat au watchdog du Teensy capteurs. |
| TX-02 | La fréquence doit être dérivée de `k / timeout` (20 paquets / 1000 ms). | Le taux découle du timeout visé, pas l'inverse. |
| TX-03 | La retransmission et le CRC doivent être assurés par le nRF24L01. | Pas de réimplémentation logicielle. |
| TX-04 | La durée pire cas des retransmissions (≈ ARC × (ARD + temps d'antenne)) doit rester inférieure à la période d'envoi (50 ms). Ex. : ARC 15 × ARD 4000 µs = 60 ms, trop long ; ARC 15 × ARD 1500 µs ≈ 23 ms, acceptable. | Évite l'accumulation de retransmissions derrière les nouvelles commandes. |
| TX-05 | Sur MAX\_RT, le Teensy contrôle doit effacer l'indicateur et vider la FIFO TX. | Sinon le module se bloque. |
| TX-06 | Un paquet en échec ne doit pas être ré-enfilé ; la commande périodique suivante le remplace. | La consigne la plus récente prime. |
| TX-07 | Un paquet au CRC invalide doit être compté comme non reçu. | Jamais d'action sur des données corrompues. |
| TX-08 | Après une nouvelle consigne, le Teensy contrôle doit refuser toute autre consigne tant qu'il n'a pas envoyé 10 paquets de cette consigne (≈ 0,5 s). Une commande d'arrêt (kill) n'est jamais bloquée par ce délai. | Laisse au Teensy capteurs le temps de compléter un cycle de pente (SAF-04) avant tout changement. Cas limite accepté : le délai compte les paquets envoyés, SAF-04 les paquets reçus ; en cas de perte, le cycle peut ne pas être terminé au déblocage. Non traité car le comportement reste sûr et une synchronisation ajouterait des transmissions et de la logique. Seul impact : la puissance monte moins vite que prévu. |

## 4. Acquittement applicatif

L'ack matériel du module RF confirme la réception ; l'ack applicatif confirme l'application.

| ID | Exigence | Justification |
| --- | --- | --- |
| ACK-01 | Le Teensy capteurs doit conserver une variable `last_applied_seq`. | Base de la déduplication et de l'ack. |
| ACK-02 | Le Teensy capteurs doit appliquer une commande seulement si `seq > last_applied_seq`, puis mettre à jour `last_applied_seq`. | Rejette doublons et paquets périmés. |
| ACK-03 | `last_applied_seq` doit être réinitialisé lors de la séquence de démarrage. | Un redémarrage du Teensy contrôle remet son seq à 0. |
| ACK-04 | Chaque paquet de télémétrie du Teensy capteurs doit inclure `last_applied_seq` et la consigne effectivement appliquée. | Ack sans message dédié. |

## 5. Surveillance du lien et arrêt sécuritaire

Chaque Teensy gère sa propre mise en sécurité : le Teensy capteurs arrête le moteur seul, le Teensy contrôle se bloque lui-même.

| ID | Exigence | Justification |
| --- | --- | --- |
| LNK-01 | Le Teensy capteurs doit déclencher l'arrêt sécuritaire si aucune commande valide n'est reçue pendant le timeout (1000 ms, soit k = 20 paquets). | Garantie de sûreté principale en cas de perte du lien. |
| LNK-02 | Une perte isolée ou courte (< k paquets) ne doit pas déclencher l'arrêt. | Évite les arrêts intempestifs dus aux interférences. |
| LNK-03 | Au-delà d'un seuil intermédiaire sans commande (≈ 100 ms, à confirmer), le Teensy capteurs doit maintenir la dernière consigne et signaler l'état DEGRADED dans la télémétrie. | Réponse étagée plutôt que panique immédiate. Permet de connaitre l'état des liens dans la télém. |
| LNK-04 | L'arrêt sécuritaire doit être consigné dans un journal d'erreur avec sa cause. | Diagnostic après essai. Aidera au debugging. |
| LNK-05 | Le Teensy contrôle doit passer en état TIMEOUT s'il ne reçoit aucune télémétrie valide (ack) du Teensy capteurs pendant le timeout (1000 ms). | Le Teensy contrôle détecte la perte du lien de son côté. |
| LNK-06 | En état TIMEOUT, le Teensy contrôle doit cesser l'envoi des commandes continues et refuser toute nouvelle commande. | Il se met en sécurité lui-même ; l'arrêt du moteur reste du ressort du Teensy capteurs. |
| LNK-07 | Le timeout définitif doit être fixé à 2–3 × le plus long trou de réception mesuré (voir section 8). | Valeur basée sur des mesures, pas sur une estimation. |

## 6. États système, démarrage et redémarrage

Le banc démarre désarmé et ne repart jamais seul après un arrêt.

| ID | Exigence | Justification |
| --- | --- | --- |
| ST-01 | Le Teensy capteurs doit démarrer à l'état désarmé, puissance 0, sans reprendre la dernière consigne. | Protège contre un reset par baisse de tension sous charge. |
| ST-02 | Chaque paquet de télémétrie doit inclure l'état courant (ARMED, DEGRADED, SHUTDOWN) et un code d'erreur. | Un message d'arrêt unique peut être perdu ; un état répété, non. |
| ST-03 | Le Teensy contrôle et le Raspberry Pi doivent refléter l'état reçu dans la télémétrie. | Tous les nœuds restent à jour. |
| ST-04 | En état TIMEOUT, l'interface du Teensy contrôle doit indiquer qu'un timeout est survenu et afficher le décompte de 5 s avant déclenchement du kill switch matériel (SAF-05). | L'opérateur sait ce qui se passe et ce qui va se passer. |
| ST-05 | Après un arrêt sécuritaire (Teensy capteurs) ou un TIMEOUT (Teensy contrôle), le seul moyen de reprendre doit être la séquence de démarrage complète, déclenchée manuellement. | Un arrêt intempestif coûte un essai, jamais un redémarrage non commandé. |

## 7. Exigences de sûreté complémentaires

Ces protections couvrent les pannes que le watchdog de lien ne voit pas.

| ID | Exigence | Justification |
| --- | --- | --- |
| SAF-01 | Le Teensy capteurs doit activer son watchdog matériel (WDOG du Teensy 4.1). | Le watchdog de lien est inutile si le firmware du Teensy capteurs gèle. |
| SAF-02 | Le banc doit comporter un kill switch matériel actionnable manuellement, indépendant du lien RF et du firmware. | Dernière ligne de défense. |
| SAF-03 | Une commande d'arrêt (kill) doit être appliquée et verrouillée dès sa première réception, sans attendre la logique des k paquets. | L'arrêt ne doit jamais être retardé. |
| SAF-04 | Le Teensy capteurs doit limiter la variation de puissance à 20 % par cycle (valeur provisoire). Un cycle (≈ 0,5 s) est atteint quand le Teensy capteurs a reçu 10 paquets consécutifs portant la même consigne. La limite ne s'applique pas à l'arrêt sécuritaire ni au kill (SAF-03). Voir TX-08 pour le cas limite lié aux paquets perdus. | Une valeur erronée ne peut pas brusquer le moteur. |
| SAF-05 | Si le banc d'essai ne se met visiblement pas en arrêt dans les 5s suivant le début de la mise en arrêt, l'opérateur doit déclencher le kill switch matériel. | Dernier moyen d'arrêter le banc d'essai si le Teensy capteurs ne s'arrête pas (firmware gelé, WDOG défaillant). |

## 8. Vérification et calibration

Le timeout de 1000 ms est provisoire ; il doit être recalibré sur mesures réelles.

| ID | Exigence | Justification |
| --- | --- | --- |
| VER-01 | Le Teensy capteurs doit pouvoir journaliser les seq reçus avec leur horodatage. | Données brutes de calibration. |
| VER-02 | La mesure doit être faite moteur à pleine puissance. | Pire cas d'interférences (ESC, EMI). |
| VER-03 | La mesure doit rapporter le taux de perte (%) et le plus long trou de réception (ms). | Les pertes réelles arrivent en rafales. |
| VER-04 | Le timeout et k doivent être mis à jour d'après VER-03 (timeout = 2–3 × plus long trou). | Cf. LNK-07. |
| VER-05 | Chaque exigence des sections 5 à 7 doit être testée par injection de faute (coupure RF, CRC invalide, gel du firmware, kill, non-arrêt du Teensy capteurs). | Valide le comportement d'arrêt avant les essais réels. |

## 9. Points ouverts

- [ ] Définir précisément « échec d'application » (paquet non reçu, ou consigne rejetée par le Teensy capteurs ?) et fusionner les compteurs 10 et 20 si c'est la même chose.
- [ ] Confirmer le seuil de l'état DEGRADED (≈ 100 ms).
- [ ] Fixer les valeurs ARC / ARD du nRF24L01 (cf. TX-04).
- [ ] Recalibrer le timeout après les mesures de la section 8.
