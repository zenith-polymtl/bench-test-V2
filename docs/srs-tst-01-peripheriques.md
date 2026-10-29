# TST-01 – Vérification des périphériques

Détail de TST-01 (section 3) : pour chaque périphérique, comment le Teensy capteurs vérifie qu'il répond. Une vérification réussit seulement si le bus répond **et** si le contenu lu est cohérent : un ACK seul ne prouve pas que la puce fonctionne.

## 1. Exigences générales

| ID | Exigence | Justification |
| --- | --- | --- |
| TST-01.G1 | Chaque vérification doit avoir un délai maximal ; un dépassement compte comme un échec. | L'auto-test ne bloque jamais. |
| TST-01.G2 | Chaque échec doit produire un code distinct par périphérique et par sous-test (ex. `IMU_WHOAMI`). | L'opérateur sait quoi corriger (cf. TST-05). |
| TST-01.G3 | Les vérifications doivent passer par le HAL du projet. | Permet de les exécuter avec des bus simulés, sans matériel. |

## 2. ADC de la cellule de charge

La spec sheet liste deux options ; garder seulement la section de l'option retenue.

### Option 1 : NAU7802 (I2C, adresse 0x2A)

| ID | Exigence | Justification |
| --- | --- | --- |
| TST-01.ADC1-1 | Le Teensy doit recevoir un ACK à l'adresse 0x2A (`Wire.endTransmission() == 0`). | Puce présente et bus I2C fonctionnel. |
| TST-01.ADC1-2 | Après un reset (bit RR de PU\_CTRL, 0x00) puis la mise sous tension (bit PUD), le bit PUR doit passer à 1 dans le délai. | Confirme que la puce démarre, pas seulement qu'elle acquitte. |
| TST-01.ADC1-3 | Les 4 bits bas du registre de révision (0x1F) doivent valoir 0xF. | Confirme que c'est bien un NAU7802 et que les lectures sont justes. |
| TST-01.ADC1-4 | La calibration interne (bit CALS de CTRL2, 0x02) doit se terminer avec CAL\_ERR = 0. | Valide la chaîne analogique interne. |
| TST-01.ADC1-5 | Après le démarrage des conversions (bit CS), le bit CR doit signaler une donnée prête dans le délai, et la valeur lue (0x12–0x14) ne doit pas être saturée (0x7FFFFF ou 0x800000). | Une valeur saturée indique une cellule débranchée ou un pont ouvert. |

### Option 2 : ADS1256 (SPI)

| ID | Exigence | Justification |
| --- | --- | --- |
| TST-01.ADC2-1 | Après la commande RESET (0xFE), la broche DRDY doit passer à l'état bas dans le délai. | La puce est alimentée et convertit. |
| TST-01.ADC2-2 | Les bits 7:4 du registre STATUS (0x00) doivent valoir l'ID d'usine (3 sur la plupart des puces, à relever sur la carte). | Confirme la communication SPI et la bonne puce. |
| TST-01.ADC2-3 | Une valeur écrite dans DRATE (0x03) doit être relue identique. | Valide l'écriture SPI, pas seulement la lecture. |
| TST-01.ADC2-4 | L'auto-calibration (commande SELFCAL, 0xF0) doit se terminer : DRDY repasse à l'état bas dans le délai. | Valide la chaîne analogique. |
| TST-01.ADC2-5 | La valeur lue (RDATA) ne doit pas être saturée (0x7FFFFF ou 0x800000). | Même raison que ADC1-5. |

## 3. IMU ICM-42688-P

| ID | Exigence | Justification |
| --- | --- | --- |
| TST-01.IMU-1 | Le registre WHO\_AM\_I (0x75) doit valoir 0x47 (en SPI, lecture à l'adresse 0x75 \| 0x80). | Identifie la puce et valide le bus. |
| TST-01.IMU-2 | Après un reset logiciel (bit 0 de DEVICE\_CONFIG, 0x11), le bit RESET\_DONE de INT\_STATUS (0x2D) doit valoir 1. | La puce redémarre correctement. |
| TST-01.IMU-3 | Après activation de l'accéléromètre et du gyroscope (PWR\_MGMT0, 0x4E) et le délai de démarrage de la datasheet, deux lectures successives des données doivent différer. | Détecte des données figées. |
| TST-01.IMU-4 | Banc au repos, la norme de l'accélération doit valoir 1 g ± 10 % (à confirmer) et le gyroscope doit être proche de 0. | Valeurs physiquement plausibles. |
| TST-01.IMU-5 | (Optionnel) L'auto-test intégré de la puce doit réussir. | Couverture plus complète si le temps le permet. |

## 4. Power module PM08-CAN

Le PM08-CAN parle DroneCAN (CAN 2.0B, ID sur 29 bits, 1 Mbit/s par défaut), via le transceiver SN65HVD230. Il ne répond pas à une requête : le Teensy vérifie qu'il publie ses messages périodiques. L'ID du type de message se lit dans les bits 8 à 23 de l'ID CAN.

| ID | Exigence | Justification |
| --- | --- | --- |
| TST-01.PM-1 | Le contrôleur CAN du Teensy ne doit pas être en bus-off, et ses compteurs d'erreurs TX/RX doivent rester bas pendant la vérification. | Détecte un transceiver débranché, un mauvais débit ou une terminaison absente. |
| TST-01.PM-2 | Un message NodeStatus (type 341) du PM08-CAN doit être reçu dans le délai (à confirmer, ex. 2 s), avec health = OK et mode = OPERATIONAL. | Heartbeat DroneCAN : le module est vivant et sain. |
| TST-01.PM-3 | Un message BatteryInfo (type 1092) doit être reçu dans le délai, avec une tension plausible (cf. TST-03). | Confirme que les mesures arrivent, pas seulement le heartbeat. |

## 5. Points ouverts

- [ ] Choisir l'option d'ADC (NAU7802 ou ADS1256) et retirer l'autre section.
- [ ] Vérifier si le PM08-CAN a un node ID fixe ou attend une allocation dynamique (DNA) : sans allocateur sur le bus, il pourrait ne jamais publier.
- [ ] Vérifier que le PM08-CAN est alimenté pendant l'auto-test : s'il est en aval du contacteur, il est éteint à cette étape.
- [ ] Relever l'ID d'usine de l'ADS1256 sur la carte (ADC2-2).
- [ ] Fixer le délai de chaque vérification et la tolérance de IMU-4.
