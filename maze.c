#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define MAX_MAZE_ROWS 21
#define MAX_MAZE_COLUMNS 41

typedef struct {
  int x;
  int y;
} Position;

Position player_pos;
Position previous_player_pos;

char maze_map[MAX_MAZE_ROWS][MAX_MAZE_COLUMNS];
int movimentos[4][2] = { {0, 2}, {2, 0}, {0, -2}, {-2, 0} };

time_t start_time;
int elapsed_time = 0;
int running = 1;
int gameIsRunning = 1;

int maxTreasures;
int treasuresCollected = 0;

void clear_console() {
#ifdef _WIN32
  system("cls");
#else
  system("clear");
#endif
}

void embaralharMovimentos() {
  for (int i = 0; i < 4; i++) {
    int j = rand() % 4;
    int temp1 = movimentos[i][0], temp2 = movimentos[i][1];
    movimentos[i][0] = movimentos[j][0];
    movimentos[i][1] = movimentos[j][1];
    movimentos[j][0] = temp1;
    movimentos[j][1] = temp2;
  }
}

void gerarLabirintoDFS(int x, int y) {
  maze_map[y][x] = '.';
  embaralharMovimentos();
  
  for (int i = 0; i < 4; i++) {
    int nx = x + movimentos[i][0];
    int ny = y + movimentos[i][1];

    if (nx > 0 && ny > 0 && nx < MAX_MAZE_COLUMNS - 1 && ny < MAX_MAZE_ROWS - 1 && maze_map[ny][nx] == '#') {
      maze_map[ny - movimentos[i][1] / 2][nx - movimentos[i][0] / 2] = '.';
      gerarLabirintoDFS(nx, ny);
    }
  }
}

void gerarLabirinto() {
  for (int y = 0; y < MAX_MAZE_ROWS; y++) {
    for (int x = 0; x < MAX_MAZE_COLUMNS; x++) {
      if (y == 0 || y == MAX_MAZE_ROWS - 1 || x == 0 || x == MAX_MAZE_COLUMNS - 1) {
        maze_map[y][x] = '#';
      } else {
        maze_map[y][x] = '#';
      }
    }
  }
  gerarLabirintoDFS(1, 1);
  
  maxTreasures = rand() % 5 + 2;
  int placedTreasures = 0;
  while (placedTreasures < maxTreasures) {
    int tx = rand() % (MAX_MAZE_COLUMNS - 2) + 1;
    int ty = rand() % (MAX_MAZE_ROWS - 2) + 1;
    if (maze_map[ty][tx] == '.') {
      maze_map[ty][tx] = 'T';
      placedTreasures++;
    }
  }
}

void print_maze() {
  clear_console();
  for (int y = 0; y < MAX_MAZE_ROWS; y++) {
    for (int x = 0; x < MAX_MAZE_COLUMNS; x++) {
      char ch = maze_map[y][x];
      if (ch == '#') {
        printf("\033[31m%c\033[0m", ch);
      } else if (ch == 'P') {
        printf("\033[34m%c\033[0m", ch);
      } else if (ch == 'T') {
        printf("\033[33m%c\033[0m", ch);
      } else {
        printf("%c", ch);
      }
    }
    printf("\n");
  }
  printf("\nTempo: %d segundos | Tesouros: %d/%d\n", elapsed_time, treasuresCollected, maxTreasures);
}

void *update_timer(void *arg) {
  while (running) {
    sleep(1);
    elapsed_time = (int)(time(NULL) - start_time);
    print_maze();
  }
  return NULL;
}

void set_player() {
  srand(time(NULL));
  int playerXPos, playerYPos;
  
  do {
    playerXPos = rand() % MAX_MAZE_COLUMNS;
    playerYPos = rand() % MAX_MAZE_ROWS;
  } while (maze_map[playerYPos][playerXPos] != '.');
  player_pos.x = playerXPos;
  player_pos.y = playerYPos;
  previous_player_pos.x = playerXPos;
  previous_player_pos.y = playerYPos;
  maze_map[player_pos.y][player_pos.x] = 'P';
}

char catch_inputs() {
  struct termios oldt, newt;
  char ch;
  
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}

int can_travel(int x, int y) {
  return (maze_map[y][x] == '.' || maze_map[y][x] == 'T');
}

void move_player(char key) {
  int newX = player_pos.x;
  int newY = player_pos.y;
  
  switch (key) {
    case 'a': newX = player_pos.x - 1; break;
    case 'd': newX = player_pos.x + 1; break;
    case 'w': newY = player_pos.y - 1; break;
    case 's': newY = player_pos.y + 1; break;
    default: return;
  }
  
  if (newX < 0 || newX >= MAX_MAZE_COLUMNS || newY < 0 || newY >= MAX_MAZE_ROWS || !can_travel(newX, newY))
    return;
  
  if (maze_map[newY][newX] == 'T') {
    treasuresCollected++;
  }
  
  maze_map[player_pos.y][player_pos.x] = '.';
  player_pos.x = newX;
  player_pos.y = newY;
  maze_map[player_pos.y][player_pos.x] = 'P';
  previous_player_pos = player_pos;
  
  print_maze();
  
  if (treasuresCollected >= maxTreasures) {
    gameIsRunning = 0;
    running = 0;
    sleep(1);
    clear_console();
    printf("\033[32m\n\n########################################\n");
    printf("#                                      #\n");
    printf("#             YOU WIN!                 #\n");
    printf("#                                      #\n");
    printf("########################################\n\n");
    printf("Tempo: %d segundos\n", elapsed_time);
    exit(0);
  }
}

int main() {
  srand(time(NULL));
  gerarLabirinto();
  set_player();
  print_maze();
  start_time = time(NULL);
  
  pthread_t timer_thread;
  pthread_create(&timer_thread, NULL, update_timer, NULL);

  while (gameIsRunning) {
    move_player(catch_inputs());
  }
  return 0;
}
