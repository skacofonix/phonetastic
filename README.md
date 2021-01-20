# Phonetastic

Cache interractive autour d'un téléphone S63.

## GPIO EXPANDER

Le GPIO EXPANDER permet de fournir des ports GPIO complémentaire accessible depuis une interface I2C.
La carte ESP32 Lyra-T possède tous ces ports GPIO soudés physiquement.
Les GPIO complémentaires sont utilisé pour la lecture des positions de JACKS en facade.

Vitesse de fonctionnement constatée : 100 000 Hz

2 modes de lecture possible :
    - **Polling**, le plus simple à mettre en oeuvre mais le moins efficient
        - Fait consommer des ressources dans une boucle d'attente avec de la tempo
        - Si un evenement à lieu sur une entrée entre 2 scrutation, l'evenement peux être raté
        - En limitant le nombre de scuration (en augmentant le délai entre chaque boucle, 500ms OK)
    - **Evenementiel**, plus efficace
        - Consomme moins de ressources car il n'y a pas de boucle d'attente
        - Nécessite un port GPIO physique sur l'ESP32 pour déclencher l'interruption
        - Lecture déclenché sur interruption
        - Peux provoquer des erreurs sur le bus I2C