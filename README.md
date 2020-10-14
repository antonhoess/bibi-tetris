# Bibi Tetris
A simple Tetris version for my two beloved little ones (Bibis).

It's implemented in C using GTK+ 2 and libcanberra for playing sounds.

It's graphics and layout is in some ways based on the ones from the good old the Nintendo Entertainment System (NES).

## Install
```bash
sudo apt-get install libcairo2-dev
sudo apt-get install libpthread-stubs0-dev
sudo apt-get install libcanberra-dev
```

## Compile and run
```bash
make
./bibi-tetris
```

## Here's a screenshot on how it looks so far

![Bibi Tetris in action](images/bibi-tetris.png "Bibi Tetris in action.")

## How to play

### Choose starting level
After loading the game, you can choose the starting difficulty (level) as follows:
* **Arrow Up**: Increase level (max. level 9).
* **Arrow Down**: Decrease level (min. level 0).

### Start / pause / continue the game
After setting the initial level, you can start the game by hitting **Enter**.
By pressing **Enter** again, you can switch between
* Pausing the game. When pausing the game, the preview of the upcoming piece is deactivated.
* Continuing the game.

### Keys
* **S**: Toggles the preview of the upcoming piece on / off.
* **D**: Rotate piece forwards.
* **F**: Rotate piece backwards.
* **Arrow Left**: Move the piece to the left.
* **Arrow Right**: Move the piece to the right.
* **Arrow Down**: Make the piece moving downwards faster than normal (has no effect in very high levels).

### Play / goal
* Crate full horizontal lines (which will disappear).
* The more lines at once (max. 4) the more points you'll get.
* After each ten rows, the level will increase (except you have already set a higher starting level).
* The higher the level in which you make full lines, the more points you'll get.
* When there's no more space to the top for the next piece, the game is over.
* The more points you gain, the better.