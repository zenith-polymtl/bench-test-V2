# Documentation – Banc d'essai moteur

Spécifications des exigences logicielles (SRS) du Teensy contrôle et du Teensy capteurs.

| Document | Contenu |
| --- | --- |
| [SRS – Gestion des commandes de puissance](srs-commandes-puissance.md) | Format des commandes, transmission à 20 Hz, ack applicatif, perte du lien, états |
| [SRS – Séquence de démarrage](srs-sequence-demarrage.md) | Préconditions, auto-test, handshake, précharge, armement, abandon |
| [TST-01 – Vérification des périphériques](srs-tst-01-peripheriques.md) | Détail de TST-01 : vérification de l'ADC, de l'IMU et du PM08-CAN |
| [SRS – Sécurité en fonctionnement](srs-securite-fonctionnement.md) | Conditions d'arrêt du Teensy capteurs pendant un essai |
| [Télémétrie](telemetrie.md) | Acquisition et stockage par capteur, journal SD, diffusion au Teensy contrôle et au Pi |
| [Messages](messages.md) | Index et format de tous les messages reçus par chaque module, états et codes d'erreur |

Les valeurs marquées « à confirmer » sont provisoires ; chaque document se termine par sa liste de points ouverts.
