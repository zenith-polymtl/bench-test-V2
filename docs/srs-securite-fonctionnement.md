# SRS – Sécurité en fonctionnement

Oct 25, 2026 · @Griselda

## 1. Introduction

Ce document fixe les conditions dans lesquelles le Teensy capteurs doit arrêter le banc d'essai moteur (Zénith) pendant un essai, c'est-à-dire une fois l'état ARMED atteint.

**Portée :** surveillance en continu par le Teensy capteurs, types d'arrêt et actions associées. Complète le *SRS – Gestion des commandes de puissance* (LNK, ST, SAF) et le *SRS – Séquence de démarrage* (reprise par séquence complète).

**Convention :** « doit » = obligatoire. Les seuils marqués « à confirmer » sont des propositions, à fixer par mesure ou d'après les datasheets.

| Terme | Définition |
| --- | --- |
| Condition d'arrêt | Événement ou mesure hors limites qui déclenche un arrêt |
| Arrêt contrôlé | Consigne à 0, attente de la baisse du courant, puis ouverture du contacteur |
| Arrêt d'urgence | Consigne à 0 et ouverture immédiate du contacteur, sans attente |
| Persistance | Nombre d'échantillons consécutifs hors limites requis avant de déclencher l'arrêt |
| SHUTDOWN | État verrouillé après un arrêt ; seule la séquence de démarrage permet d'en sortir |

## 2. Principes de surveillance

La surveillance tourne en continu, en priorité, et filtre le bruit sans masquer un vrai défaut.

| ID | Exigence | Justification |
| --- | --- | --- |
| SUR-01 | Le Teensy capteurs doit évaluer toutes les conditions d'arrêt à fréquence fixe (à confirmer, ex. toutes les 10 ms), quel que soit l'état du lien RF. | La sécurité ne dépend jamais des communications. |
| SUR-02 | L'évaluation des conditions d'arrêt doit être prioritaire sur la télémétrie et le journal. | Un traitement lent ne retarde pas un arrêt. |
| SUR-03 | Une condition doit être vraie pendant sa persistance (à confirmer par condition) avant de déclencher l'arrêt, sauf les conditions marquées « immédiat ». | Évite les arrêts intempestifs dus au bruit ou à une interférence isolée. |
| SUR-04 | Une mesure invalide ou absente doit être traitée comme hors limites, jamais comme normale. | Un capteur muet ne doit pas masquer un défaut. |
| SUR-05 | Chaque condition d'arrêt doit avoir un code distinct, utilisé dans la télémétrie et le journal. | Diagnostic après essai. |

## 3. Types d'arrêt et actions

Deux types d'arrêt : le contrôlé ménage le contacteur, l'urgence coupe tout de suite. Chaque condition des sections 4 à 7 indique lequel s'applique.

| ID | Exigence | Justification |
| --- | --- | --- |
| ARR-01 | Un arrêt contrôlé doit mettre la consigne à 0, attendre que le courant passe sous un seuil (à confirmer) ou un délai maximal (à confirmer, ex. 2 s), puis ouvrir le contacteur. | Ouvrir un contacteur sous fort courant l'use et crée un arc. |
| ARR-02 | Un arrêt d'urgence doit mettre la consigne à 0 et ouvrir le contacteur immédiatement. | Pour les défauts où attendre est plus dangereux qu'user le contacteur. |
| ARR-03 | La mise à 0 de la consigne lors d'un arrêt ne doit pas être soumise à la limite de pente (SAF-04). | L'arrêt n'est jamais ralenti. |
| ARR-04 | Tout arrêt doit verrouiller l'état SHUTDOWN ; la reprise doit passer par la séquence de démarrage complète. | Cf. ST-05 ; jamais de redémarrage automatique. |
| ARR-05 | Tout arrêt doit être signalé dans chaque paquet de télémétrie (état + code) et consigné dans le journal avec les dernières mesures. | Cf. ST-02 ; les mesures aident à comprendre la cause. |
| ARR-06 | Si plusieurs conditions sont vraies en même temps, l'arrêt d'urgence doit primer. | Le type le plus sûr l'emporte. |

## 4. Conditions liées au lien et aux commandes

Ces conditions reprennent les règles du SRS commandes et en fixent le type d'arrêt.

| ID | Condition d'arrêt | Type | Justification |
| --- | --- | --- | --- |
| COM-01 | Aucune commande valide reçue pendant le timeout (LNK-01, 1000 ms). | Contrôlé | L'opérateur n'a plus la main sur le moteur. |
| COM-02 | Réception d'une commande kill (SAF-03). | Urgence, immédiat | Demande explicite d'arrêt. |
| COM-03 | Réception de l'état OFF de l'arming switch (ARM-06). | Contrôlé | Désarmement volontaire de l'opérateur. |
| COM-04 | Commandes rejetées comme invalides (CMD-03) pendant toute la persistance. | Contrôlé | Signe d'un défaut côté contrôle ou de corruption sur le lien. |

## 5. Conditions électriques

Mesures fournies par le PM08-CAN (tension, courant). Le fusible de 80 A reste la protection matérielle ; les seuils logiciels doivent agir avant lui.

| ID | Condition d'arrêt | Type | Justification |
| --- | --- | --- | --- |
| ELE-01 | Courant au-dessus du seuil de surintensité (à confirmer, sous 80 A et sous le maximum de l'ESC et du moteur), persistance courte. | Urgence | Évite la fonte du fusible et la dégradation de l'ESC ou du moteur. |
| ELE-02 | Tension batterie sous 3,3 V par cellule (à confirmer). | Contrôlé | Protège la LiPo d'une décharge profonde. |
| ELE-03 | Tension batterie au-dessus de 4,25 V par cellule (à confirmer). | Contrôlé | Signe d'une mauvaise batterie ou d'un mauvais nombre de cellules configuré. |
| ELE-04 | Chute de la tension côté ESC alors que le contacteur est commandé fermé. | Urgence | Contacteur ou fusible ouvert de façon inattendue : l'état réel ne correspond plus à l'état commandé. |
| ELE-05 | Plus de données du PM08-CAN pendant le délai (à confirmer), ou contrôleur CAN en bus-off. | Contrôlé | Sans mesure de courant, la surintensité n'est plus détectable (SUR-04). |

## 6. Conditions mécaniques

Mesures de la cellule de charge (capacité 50 lb) et de l'IMU.

| ID | Condition d'arrêt | Type | Justification |
| --- | --- | --- | --- |
| MEC-01 | Force mesurée au-dessus de 90 % de la capacité de la cellule, soit 45 lb (à confirmer). | Contrôlé | Protège la cellule de charge d'une surcharge permanente. |
| MEC-02 | Valeur de la cellule de charge absente ou saturée pendant la persistance. | Contrôlé | Cellule débranchée ou ADC en défaut : la poussée n'est plus surveillée (SUR-04). |
| MEC-03 | Vibration (accélération RMS de l'IMU) au-dessus d'un seuil fixé d'après des essais normaux (à mesurer), persistance courte. | Urgence | Signe d'une hélice déséquilibrée ou cassée, ou d'une fixation qui lâche. |
| MEC-04 | Données de l'IMU absentes ou figées pendant la persistance. | Contrôlé | Les vibrations ne sont plus surveillées (SUR-04). |

## 7. Conditions matérielles et organes de sécurité

L'E-stop et l'interlock coupent le circuit matériellement ; le Teensy capteurs doit quand même détecter la coupure et verrouiller son état.

| ID | Condition d'arrêt | Type | Justification |
| --- | --- | --- | --- |
| MAT-01 | E-stop enfoncé ou interlock ouvert (détecté par lecture directe si câblée, sinon par ELE-04). | Urgence, immédiat | Relâcher l'E-stop ou refermer l'interlock ne doit jamais relancer le moteur : l'état SHUTDOWN reste verrouillé (ARR-04). |
| MAT-02 | Redémarrage du Teensy capteurs (watchdog, baisse de tension) : il repart à l'état initial, contacteur ouvert (PRE-02), et doit consigner la cause du reset. | Urgence (de fait) | Le reset coupe tout ; la cause aide au diagnostic. |
| MAT-03 | Cycle de surveillance (SUR-01) dépassé de façon répétée (à confirmer). | Contrôlé | Le firmware ne tient plus son rythme avant même que le watchdog ne déclenche. |

## 8. Vérification

Chaque condition d'arrêt est déclenchée volontairement au moins une fois avant les essais réels.

| ID | Exigence | Justification |
| --- | --- | --- |
| VER-01 | Chaque condition des sections 4 à 7 doit être testée par injection de faute (valeurs simulées via le HAL, puis matériel réel quand c'est sûr). | Valide la détection et le bon type d'arrêt. |
| VER-02 | Pour chaque test, le type d'arrêt, le code, l'état SHUTDOWN verrouillé et l'entrée du journal doivent être vérifiés. | Valide toute la chaîne, pas seulement la détection. |
| VER-03 | Le seuil de vibration (MEC-03) doit être fixé d'après des mesures en fonctionnement normal, sur toute la plage de puissance. | Évite les arrêts intempestifs à haute puissance. |
| VER-04 | Le temps de baisse du courant après consigne 0 doit être mesuré pour fixer le seuil et le délai de ARR-01. | Valeurs fondées sur le matériel réel. |
| VER-05 | Les persistances doivent être validées moteur à pleine puissance (pire cas d'interférences). | Aucun arrêt intempestif sur bruit, aucun défaut réel masqué. |

## 9. Points ouverts

- [ ] Fixer le seuil de surintensité (ELE-01) d'après les maximums de l'ESC et du moteur.
- [ ] Confirmer les seuils de tension par cellule (ELE-02, ELE-03) et la façon de configurer le nombre de cellules (4S à 14S).
- [ ] Vérifier le pouvoir de coupure du GX12BA sous charge, pour savoir si l'arrêt d'urgence (ARR-02) est acceptable à pleine puissance.
- [ ] Définir où la tension côté ESC est mesurée (ELE-04) ; même question que dans le SRS démarrage.
- [ ] Décider si l'E-stop et l'interlock sont lus directement par le Teensy capteurs (MAT-01).
- [ ] Fixer la fréquence de surveillance (SUR-01) et la persistance de chaque condition.
- [ ] Ajouter ou non une surveillance de température (moteur, ESC) : aucun capteur prévu dans la spec sheet.
- [ ] Ajouter ou non une condition de poussée incohérente (consigne élevée, force quasi nulle : hélice perdue).
