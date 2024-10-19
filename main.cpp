#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
#endif
#include <stdlib.h>
#include <time.h>

#define WIDTH 40
#define HEIGHT 20
#define MAX_SNAKE_LENGTH (WIDTH * HEIGHT)

char field[HEIGHT][WIDTH];
int gameOver = 0;
int score1 = 0, score2 = 0;
int foodX, foodY;
int mode;
int speed = 200000; 
int playAgain = 1;

typedef struct {
    int bodyX[MAX_SNAKE_LENGTH];
    int bodyY[MAX_SNAKE_LENGTH];
    int length;
    char direction;
    char symbol;
    char headSymbol;
    pthread_mutex_t mtx;
} Snake;

Snake snake1, snake2;

pthread_mutex_t drawMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t foodCond = PTHREAD_COND_INITIALIZER;
int foodEaten = 0;

void Setup();
void Draw();
void* Input(void* arg);
void* Logic(void* arg);
void* FoodGenerator(void* arg);
void PlaceFood();
void DisableBuffering();
void EnableBuffering();
void ResetGame();
void PlayGame();
void CleanupGame();
void InitializeGame();

int main() {
    DisableBuffering();

    do {
        printf("Bienvenido al juego Snake!\n");
        printf("Seleccione el modo de juego:\n");
        printf("1. Modo 1 Jugador\n");
        printf("2. Modo 2 Jugadores\n");
        printf("Ingrese su opción: ");
        scanf("%d", &mode);
        getchar(); // Consume el newline

        PlayGame();

        printf("\n¿Quieres jugar de nuevo? \n 1. Sí \n 0. No \n Ingresar: ");
        scanf("%d", &playAgain);
        getchar(); // Consume el newline

        CleanupGame();

    } while (playAgain);

    EnableBuffering();
    return 0;
}

void InitializeGame() {
    gameOver = 0;
    score1 = 0;
    score2 = 0;
    foodEaten = 0;

    pthread_mutex_init(&drawMutex, NULL);
    pthread_cond_init(&foodCond, NULL);

    pthread_mutex_init(&snake1.mtx, NULL);
    if (mode == 2) {
        pthread_mutex_init(&snake2.mtx, NULL);
    }
}

void CleanupGame() {
    pthread_mutex_destroy(&drawMutex);
    pthread_cond_destroy(&foodCond);

    pthread_mutex_destroy(&snake1.mtx);
    if (mode == 2) {
        pthread_mutex_destroy(&snake2.mtx);
    }
}

void PlayGame() {
    InitializeGame();
    ResetGame();
    Setup();

    pthread_t inputThread, logicThread, foodThread;

    pthread_create(&inputThread, NULL, Input, NULL);
    pthread_create(&logicThread, NULL, Logic, NULL);
    pthread_create(&foodThread, NULL, FoodGenerator, NULL);

    pthread_join(logicThread, NULL);
    pthread_cancel(inputThread);
    pthread_cancel(foodThread);

    pthread_join(inputThread, NULL);
    pthread_join(foodThread, NULL);

    printf("\nJuego Terminado!\n");
    if (mode == 1) {
        printf("Puntaje: %d\n", score1);
    } else {
        printf("Puntaje Jugador 1: %d\n", score1);
        printf("Puntaje Jugador 2: %d\n", score2);
    }
}

void ResetGame() {
    gameOver = 0;
    score1 = 0;
    score2 = 0;
    foodEaten = 0;

    // Reiniciar snake1
    snake1.length = 1;
    snake1.direction = 'r';
    snake1.bodyX[0] = HEIGHT / 2;
    snake1.bodyY[0] = WIDTH / 4;
    pthread_mutex_init(&snake1.mtx, NULL);

    // Reiniciar snake2 si es necesario
    if (mode == 2) {
        snake2.length = 1;
        snake2.direction = 'l';
        snake2.bodyX[0] = HEIGHT / 2;
        snake2.bodyY[0] = 3 * WIDTH / 4;
        pthread_mutex_init(&snake2.mtx, NULL);
    }

    // Limpiar el campo
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            field[i][j] = (i == 0 || i == HEIGHT-1 || j == 0 || j == WIDTH-1) ? '#' : ' ';
        }
    }

    PlaceFood();
}


void Setup() {
    srand(time(0));

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            field[i][j] = ' ';
        }
    }

    for (int i = 0; i < WIDTH; i++) {
        field[0][i] = '#';
        field[HEIGHT - 1][i] = '#';
    }
    for (int i = 0; i < HEIGHT; i++) {
        field[i][0] = '#';
        field[i][WIDTH - 1] = '#';
    }

    snake1.bodyX[0] = HEIGHT / 2;
    snake1.bodyY[0] = WIDTH / 4;
    snake1.length = 1;
    snake1.direction = 'r';
    snake1.symbol = 'O';
    snake1.headSymbol = 'V';
    pthread_mutex_init(&snake1.mtx, NULL);

    if (mode == 2) {
        snake2.bodyX[0] = HEIGHT / 2;
        snake2.bodyY[0] = 3 * WIDTH / 4;
        snake2.length = 1;
        snake2.direction = 'l';
        snake2.symbol = 'Q';
        snake2.headSymbol = 'W';
        pthread_mutex_init(&snake2.mtx, NULL);
    }

    PlaceFood();
}

void Draw() {
    pthread_mutex_lock(&drawMutex);
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    for (int i = 1; i < snake1.length; i++) {
        field[snake1.bodyX[i]][snake1.bodyY[i]] = snake1.symbol;
    }
    field[snake1.bodyX[0]][snake1.bodyY[0]] = snake1.headSymbol;

    if (mode == 2) {
        for (int i = 1; i < snake2.length; i++) {
            field[snake2.bodyX[i]][snake2.bodyY[i]] = snake2.symbol;
        }
        field[snake2.bodyX[0]][snake2.bodyY[0]] = snake2.headSymbol;
    }

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            printf("%c", field[i][j]);
        }
        printf("\n");
    }

    if (mode == 1) {
        printf("Puntaje: %d\n", score1);
    } else {
        printf("Puntaje Jugador 1: %d  ", score1);
        printf("Puntaje Jugador 2: %d\n", score2);
    }

    for (int i = 0; i < snake1.length; i++) {
        field[snake1.bodyX[i]][snake1.bodyY[i]] = ' ';
    }
    if (mode == 2) {
        for (int i = 0; i < snake2.length; i++) {
            field[snake2.bodyX[i]][snake2.bodyY[i]] = ' ';
        }
    }

    pthread_mutex_unlock(&drawMutex);
}

void* Input(void* arg) {
    while (!gameOver) {
        char ch;
#ifdef _WIN32
        if (_kbhit()) {
            ch = _getch();
#else
        ch = getchar();
#endif

            pthread_mutex_lock(&snake1.mtx);
            if (ch == 'w' && snake1.direction != 's') snake1.direction = 'w';
            else if (ch == 's' && snake1.direction != 'w') snake1.direction = 's';
            else if (ch == 'a' && snake1.direction != 'd') snake1.direction = 'a';
            else if (ch == 'd' && snake1.direction != 'a') snake1.direction = 'd';
            pthread_mutex_unlock(&snake1.mtx);

            if (mode == 2) {
                pthread_mutex_lock(&snake2.mtx);
                if (ch == 'i' && snake2.direction != 'k') snake2.direction = 'i';
                else if (ch == 'k' && snake2.direction != 'i') snake2.direction = 'k';
                else if (ch == 'j' && snake2.direction != 'l') snake2.direction = 'j';
                else if (ch == 'l' && snake2.direction != 'j') snake2.direction = 'l';
                pthread_mutex_unlock(&snake2.mtx);
            }
#ifdef _WIN32
        }
#endif
    }
    return NULL;
}

void* Logic(void* arg) {
    while (!gameOver) {
        struct timespec req = {0};
        req.tv_sec = 0;
        req.tv_nsec = speed * 1000;
        nanosleep(&req, NULL);

        pthread_mutex_lock(&snake1.mtx);
        int x = snake1.bodyY[0];
        int y = snake1.bodyX[0];

        if (snake1.direction == 'w') y--;
        else if (snake1.direction == 's') y++;
        else if (snake1.direction == 'a') x--;
        else if (snake1.direction == 'd') x++;

        if (field[y][x] == '#' || field[y][x] == snake1.symbol || (mode == 2 && field[y][x] == snake2.symbol)) {
            gameOver = 1;
        } else {
            for (int i = snake1.length; i > 0; i--) {
                snake1.bodyX[i] = snake1.bodyX[i - 1];
                snake1.bodyY[i] = snake1.bodyY[i - 1];
            }
            snake1.bodyX[0] = y;
            snake1.bodyY[0] = x;

            // Verificar colisión con el propio cuerpo
            for (int i = 1; i < snake1.length; i++) {
                if (snake1.bodyX[0] == snake1.bodyX[i] && snake1.bodyY[0] == snake1.bodyY[i]) {
                    gameOver = 1;
                    break;
                }
            }

            if (x == foodX && y == foodY) {
                snake1.length++;
                score1++;
                foodEaten = 1;
                pthread_cond_signal(&foodCond);
            }
        }
        pthread_mutex_unlock(&snake1.mtx);

        if (mode == 2) {
            pthread_mutex_lock(&snake2.mtx);
            int x2 = snake2.bodyY[0];
            int y2 = snake2.bodyX[0];

            if (snake2.direction == 'i') y2--;
            else if (snake2.direction == 'k') y2++;
            else if (snake2.direction == 'j') x2--;
            else if (snake2.direction == 'l') x2++;

            if (field[y2][x2] == '#' || field[y2][x2] == snake2.symbol || field[y2][x2] == snake1.symbol) {
                gameOver = 1;
            } else {
                for (int i = snake2.length; i > 0; i--) {
                    snake2.bodyX[i] = snake2.bodyX[i - 1];
                    snake2.bodyY[i] = snake2.bodyY[i - 1];
                }
                snake2.bodyX[0] = y2;
                snake2.bodyY[0] = x2;

                // Verificar colisión con el propio cuerpo
                for (int i = 1; i < snake2.length; i++) {
                    if (snake2.bodyX[0] == snake2.bodyX[i] && snake2.bodyY[0] == snake2.bodyY[i]) {
                        gameOver = 1;
                        break;
                    }
                }

                if (x2 == foodX && y2 == foodY) {
                    snake2.length++;
                    score2++;
                    foodEaten = 1;
                    pthread_cond_signal(&foodCond);
                }
            }
            pthread_mutex_unlock(&snake2.mtx);
        }

        Draw();
    }
    return NULL;
}

void* FoodGenerator(void* arg) {
    while (!gameOver) {
        pthread_mutex_lock(&drawMutex);
        while (!foodEaten && !gameOver) {
            pthread_cond_wait(&foodCond, &drawMutex);
        }

        if (gameOver) {
            pthread_mutex_unlock(&drawMutex);
            break;
        }

        PlaceFood();
        foodEaten = 0;
        pthread_mutex_unlock(&drawMutex);
    }
    return NULL;
}

void PlaceFood() {
    do {
        foodX = rand() % (WIDTH - 2) + 1;
        foodY = rand() % (HEIGHT - 2) + 1;
    } while (field[foodY][foodX] != ' ');

    field[foodY][foodX] = '@';
}

#ifdef _WIN32
void DisableBuffering() {
    // No es necesario en Windows
}
void EnableBuffering() {
    // No es necesario en Windows
}
#else
void DisableBuffering() {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag &= ~ICANON;
    ttystate.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}
void EnableBuffering() {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag |= ICANON;
    ttystate.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}
#endif
