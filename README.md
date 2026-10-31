# zenith-bench-test

Firmware pour le banc de test moteur — Équipe Contrôle, Zenith.

## Architecture

Teensy 4.1 → capteurs (IMU, load cell, power module) → UART → RPi 4 → dashboard web(https://app.notion.com/p/zenithpolymtl/Bench-test-3e762814d76880be9475fa3a6612a50c?source=copy_link)

## Setup

### Développement (recommandé)
1. Installer VS Code + extension PlatformIO
2. Ouvrir `firmware/` dans VS Code
3. Build : Ctrl+Alt+B

### Avec Docker (CI / sans installation)
1. `make build


## Contribuer

- Les librairies du teensy se gèrent dans `firmware/platformio.ini` sous `lib_deps`
- Ne pas commit les dossiers `.pio/` ou `.vscode/`

## TODO

Tag **[HAL]** = interfaces de bus (`communication/`), leurs implémentations Teensy et leurs mocks.

## Todo – Développement firmware banc d'essai

**Phase 0 – Fondations**
- [X] Arborescence : `communication/`, `devices/`, `protocol/`, `safety/`, `logging/`, `modules/`
- [X] `platformio.ini` : `env:sensor`, `env:control`, `env:native`
- [X] `protocol/` : structs des messages, états, codes d'erreur + `static_assert` sur les tailles
- [ ] **[HAL]** Interfaces `communication/` (SPI, I2C, CAN, UART, PWM, GPIO)
- [ ] Unity en `env:native` + CI qui lance les tests

**Phase 1 – Communication**
- [ ] **[HAL]** Implémentations Teensy de chaque interface
- [ ] **[HAL]** Mocks scriptables de chaque interface
- [ ] **[HAL]** Smoke test matériel par bus

**Phase 2 – RF**
- [ ] Interface `devices/radio.hpp` (`send`, `receive`, `setAckPayload`) + mock (injection et perte de paquets)
- [ ] Driver `devices/nrf24` sur le HAL SPI + GPIO (CE, IRQ) : envoi, ack payload, MAX_RT + flush
- [ ] Test de bout en bout entre deux Teensy : `CMD_POWER` / `TLM_STATUS`
- [ ] Mesure de la perte de paquets

**Phase 3 – Tranche verticale sécurité**
- [ ] Seq + `last_applied_seq` + ack
- [ ] Watchdog de lien (timeout, DEGRADED)
- [ ] Kill verrouillé
- [ ] Driver `devices/esc` sur le HAL PWM + limite de pente
- [ ] Tests natifs avec le mock de `radio.hpp`
- [ ] Test sans hélice : couper la RF → le moteur s'arrête

**Phase 4 – Capteurs**
- [ ] Driver de cellule de charge (choisir NAU7802 ou ADS1256)
- [ ] Driver IMU : FIFO 1 kHz + RMS
- [ ] Driver PM08-CAN : vérifier le type 1091 ou 1092, puis parser
- [ ] Auto-test TST-01

**Phase 5 – Safety complet**
- [ ] Machine d'états du Teensy capteurs
- [ ] Séquence de démarrage (précharge, contacteur, armement) via le HAL GPIO
- [ ] Conditions d'arrêt COM, ELE, MEC, MAT
- [ ] WDOG matériel (registres du Teensy, hors HAL)
- [ ] Injection de faute pour chaque code d'erreur

**Phase 6 – Télémétrie**
- [ ] JSON vers le Pi (`tlm`, `event`, `selftest`, `boot`) via le HAL UART
- [ ] Journal SD (données + événements)

**Phase 7 – Teensy contrôle**
- [ ] Machine d'états du contrôle + TIMEOUT
- [ ] Entrées (arming switch, potentiomètre) via le HAL GPIO, et écran
- [ ] Scénario complet : en natif, puis sur matériel réel

**Phase 8 – Intégration**
- [ ] Essais sans hélice, puis avec hélice à faible puissance
- [ ] Calibration : timeout, vibrations, précharge, cellule de charge
- [ ] Mise à jour des seuils « à confirmer » dans les SRS

Les phases 2 et 4 peuvent avancer en parallèle. La phase 3 dépend seulement des phases 1 et 2.
