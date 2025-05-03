# Pupitre de commande
## Matériel utilisé

- 1 microcontrôleur ESP32-32D
- 2 Tags RFID 13,56 MHz de 2,8 cm de diamètre (taille standard)
- 8 LEDs WS2812B de 60 leds/mètre, coupées en 2 bandes de 4 leds
- 1 lecteur RFID RC522
- 1 socle pour ESP32 MRD068A
- 1 câble USB vers USB-C
- 1 adaptateur USB vers prise électrique 5V (Il est aussi possible de brancher le câble sur une prise USB d’un ordinateur.)

**ATTENTION : Si l’adaptateur USB est supérieur à 5V, le microcontrôleur ESP32 risque de griller.**

## Schéma électrique

![Schéma électrique du montage](./schema-electrique.png)

## Contenu

- Dossier ESP32-DEV-Module
- STM32-F4
- Modèles 3D

## Modèles 3D

Ce dossier contient les fichiers pour l'impression 3D du boîtier et des figurines.

## STM32-F4

Ce dossier contient une version abandonnée du code pour un microcontrôleur STM32.
Ce code fonctionne en liaison série USB, mais n’a pas été maintenu ni mis à jour.

## ESP32-DEV-Module

Ce dossier contient le code pour le microcontrôleur ESP32 utilisé pour ce projet.

### Bibliothèques utilisées

- Adafruit NeoPixel by Adafruit (version 1.12.5)
- MFRC522 by GithubCommunity (version 1.4.12)

### Modifier le tag RFID utilisé

Il est possible, pour différentes raisons, que le tag ne puisse plus être utilisé.

Si c’est le tag du fossile, il suffit de le remplacer.
Mais si c’est le tag du Compsognathus, veuillez suivre ces étapes pour le remplacer :

1. Installer Arduino IDE (https://www.arduino.cc/en/software/)
2. Ouvrir avec Arduino IDE le fichier ESP32-DEV-Module.ino
3. Dans Arduino IDE --> Préférences --> Settings, sous "Additional boards manager URLs :", écrire "https://dl.espressif.com/dl/package_esp32_index.json" puis cliquer sur OK  
   ![Capture de settings](./capture1.png)
4. Dans le menu principal, cliquer sur la carte puis sur "Select other board and port..."  
   ![Capture du menu principal](./capture2.png)
5. Brancher l’ESP32 à l’ordinateur avec un câble qui permet le transfert de données
6. Choisir pour BOARDS "ESP32 Dev Module" et le port qui s’est affiché lorsque l’ESP32 a été branché à l’ordinateur puis cliquer sur OK  
   ![Capture du menu Select other board and port](./capture3.png)
7. Appuyer sur le logo ![logo loupe](./capture4.png) situé en haut à droite
8. Présenter le nouveau tag sur le lecteur. Le message "Nouveau tag détecté: ..." doit apparaître. Noter la valeur indiquée après le message (dans cet exemple 7F E8 2 51).  
   ![Capture du menu Serial monitor](./capture5.png)
9. Dans le fichier ESP32-DEV-Module.ino, aller à la ligne de la variable suivante :
   ```cpp
   const uint8_t EXPECTED_UID[] = {...}
   ```
   Remplacer l’intérieur des {} par la valeur notée à l’étape 8 avec les modifications suivantes :
* Si un groupe est composé d’un seul caractère, ajouter un 0 devant (Exemple : 7F E8 2 51 --> 7F E8 02 51)
* Ajouter 0x devant chaque groupe (Exemple : 7F E8 02 51 --> 0x7F 0xE8 0x02 0x51)
* Séparer les groupes par une virgule (Exemple : 0x7F 0xE8 0x02 0x51 --> 0x7F, 0xE8, 0x02, 0x51)

Pour notre exemple, la ligne sera:
   ```cpp
   const uint8_t EXPECTED_UID[] = {0x7F, 0xE8, 0x02, 0x51};
   ```
10. Appuyer sur le logo ![logo upload](./capture6.png) situé en haut à gauche 
    11. Après la fin de l’upload, vérifier que le tag est bien reconnu et que le message "RFID Tag reconnu !" s’affiche ![capture du menu Serial moniteur](./capture7.png)
    Si le tag n'est pas reconnu, recommencer à partir de l'étape 2