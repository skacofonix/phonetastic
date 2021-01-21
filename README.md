# GC Phonetastic

GeoCache interractive sur le thème téléphone S63 rétro.

## Materiel

Carte Lyra-t v4.3 basé sur un ESP-32

## Audio

Le dispositif est équipé de 2 périphériques de sortie audio.

- L'un permettant de simuler une sonnerie de téléphone.
- Le second faisant office d'écouteur de téléphone.

Historiquement, une sonnerie de téléphone d'un S63 utilise une sonette qui fonctionne à l'aide d'un marteau qui viens taper entre 2 cloches.
Le marteau est mis en mouvement à l'aide d'une bobine et d'un aimant.
Ce dispositif, est trop energivor epour le faire fonctionner sur pile, aussi une solution de remplacement utilisant une enceinte jouant un son de "vielle sonnerie" est a préferer.

La carte Lyra-T est muni de 2 canaux audio.
Chaqun de ces canaux sera utilisé pour

Techniquement il y'a plueirus façon pour mettre en oeuvre ces 2 canaux :

- 2 canaux de lecture, un LEFT, un RIGHT
  -- Le plus "simple" d'un point de vue player
  -- Oui mais il faut gérer la synchronization ente les canaux, lorsque l'un joue, l'autre doit s'arreter

- 1 canal de lecture que l'on reconfigure
  -- Le plus "malin" d'un point de vue player
  -- On reconfigure à la volé sur quel sortie on veux lire RIGH ou LEFT

- Des fichiers audio uniquement LEFT ou RIGHT
  -- Le plus simple (1 seul canal nécessaire)
  -- A voir les risque de bruit parasite ou de débordement sur l'autre canal

Après plusieurs essais réalisée il apparait qu'il faut 2 pipeline avec des fichiers gauche et des fichiers droits.
L'usage des 2 pipeline permet de jongler plus facilement sur l'utilisation du create_fatfs_stream_writer qui peux être utiliser en RIGH only ou LEFT only.
1 seule pipeline audio, avec des fichiers audio LEFT ou RIGHT.
Attention, pour que le son ne sorte que par un seul des canals, il faut bien enregistrer le son en stéré avec l'autre piste muette.
Attention, via la prise jack le son sort en stéréo.



## GPIO EXPANDER

Le GPIO EXPANDER permet de fournir des ports GPIO complémentaire accessible depuis une interface I2C.
La carte ESP32 Lyra-T possède tous ces ports GPIO soudés physiquement.
Les GPIO complémentaires sont utilisé pour la lecture des positions de JACKS en facade.

Voir `diag_gpio_expander.c` pour un code d'exemple fonctionnel.

Vitesse de fonctionnement constatée : 100 000 Hz

2 modes de lecture possible :

- **Polling**, le plus simple à mettre en oeuvre mais le moins efficient
    -- Fait consommer des ressources dans une boucle d'attente avec de la tempo
    -- Si un evenement à lieu sur une entrée entre 2 scrutation, l'evenement peux être raté
    -- En limitant le nombre de scuration (en augmentant le délai entre chaque boucle, 500ms OK)
- **Evenementiel**, plus efficace
    -- Consomme moins de ressources car il n'y a pas de boucle d'attente
    -- Nécessite un port GPIO physique sur l'ESP32 pour déclencher l'interruption
    -- Lecture déclenché sur interruption
    -- Peux provoquer des erreurs sur le bus I2C

Lors des derniers tests, les deux modes ont fonctionnés avec succès.
Le mode **Evenementiel** est à privilégier.