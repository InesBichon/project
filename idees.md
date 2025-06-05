- problèmes : gérer les collisions
- calculer la parabole dans le vertex shader pour éviter de transférer des données
- skybox (pb pour trouver une bonne image, jolie & de bonne res, etc)
- murs aux bords de la map
- texture pour la baballe
- la caméra de base est nulle : il faut ajouter une manière de se déplacer -> WASD !!
- reset avec "z"
- gérer les pentes : ne ralentir la balle que si la pente est peu verticale
- rebonds goofy + bug (décalage des positions sur le terrain -> normales sont fausses)
- afficher dans la console une liste des commandes

- même avec WASD, pas pratique du tout avec une caméra euler... il faudrait une caméra FPS !

- mettre une texture sur la balle n'était pas du tout convaincant parce qu'elle ne roulait pas

à améliorer : terrains plus complexes ; niveaux de difficulté

expliquer les contrôles dans le terminal
avant de soumettre :

- SUPPRIMER BUILD !!!

- vérifier que ça compile bien !!

remplacer les cgp::vec3 par des vec3
vérifier aussi les terrain
nettoyer les shaders, etc