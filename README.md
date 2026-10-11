# Le Sauvetage en Mer
![Capture du jeu Le Sauvetage en Mer](image%20jeux.png)
## Description

Le Sauvetage en Mer est un jeu développé en langage C dans lequel le joueur contrôle un radeau afin d'attraper des objets qui tombent.

Le jeu propose trois niveaux de difficulté et un système de score.

## Fonctionnalités

- Déplacement du radeau en temps réel
- Apparition aléatoire des objets
- Trois niveaux de difficulté
- Système de score
- Sauvegarde et chargement d'une partie

## Commandes

- `a` : déplacer le radeau vers la gauche
- `d` : déplacer le radeau vers la droite
- `q` : quitter le jeu

## Technologies

- Langage C
- Gestion des entrées clavier dans le terminal
- Génération aléatoire
- Gestion du temps et de l'affichage dans le terminal

## Exécution

Le programme utilise des fonctionnalités de terminal compatibles avec un environnement POSIX, par exemple Linux ou WSL sous Windows.

Compilez le programme avec GCC :

```bash
gcc sauvetage.c -o sauvetage
```

Lancez ensuite le jeu :

```bash
./sauvetage
```

## Auteurs

Mame Thierno Thiam et Aminata Ba
