#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H
typedef enum {
    FOOD_REGULAR,      // Increases length by 1
    FOOD_SPEED_BOOST,  // Increases speed
    FOOD_SLOW_DOWN, 	// Awards extra points
	FOOD_BONUS,   		// Decreases speed
	FOOD_INVINCIBLE, 	// Makes snake invincible
    FOOD_REVERSE,      // Reverses controls
    FOOD_SHRINK,       // Reduces length by 1



} FoodType;

typedef struct {
    volatile unsigned long COMM_VAL; // COMM_VAL as part of the structure
    char filename[32]; // Filename of the audio file
    int command;       // Command (e.g., play background music, play sound effect)
    volatile int theVolume;
} __attribute__((packed)) audio_command_t; // Pack the structure to avoid padding


	extern volatile audio_command_t *shared_memory; // Adjust address as needed


typedef struct {
    FoodType type;   // Type of food
    int lifetime;   // Remaining lifetime (in timer interrupts)

} Food;

extern Food food;

enum StartMenuOption {
	Start = 1,
	Options = 2
};
enum OptionSelection {
	Volume = 0,
	Difficulty = 1,
	Snake_Color = 2
};

enum State {
	StartMenu = 0,
	GameStart = 1,
	GameOptions = 2
};

enum Difficulty {
	Easy = 1,
	Medium = 2,
	Hard = 3
};

typedef struct {
    int x;  // X coordinate
    int y;  // Y coordinate
} Obstacle;

extern int GameState;
extern Obstacle obstacles[];
extern bool GameOver;
extern bool isInvincible;
extern int invincible_duration;
extern bool speed_boost_active; // Is the speed boost active?
extern int speed_boost_duration; // Remaining duration of the speed boost (in timer interrupts)
extern bool soundFX;
extern bool collisionFX;
extern int snakeBody_X[200], snakeBody_Y[200];
extern int food_x, food_y; // Food position
extern int difficulty;
extern int current_direction;
extern int SNAKE_LENGTH;
extern int x_coord;
extern int y_coord;
extern int counter;
extern int score;
extern int num_obstacles;
int is_position_valid(int x, int y, int snakeBody_X[], int snakeBody_Y[], int snake_length);
void spawn_obstacles(int snakeBody_X[], int snakeBody_Y[], int snake_length);
void get_random_block(int* block_x, int* block_y, int snakeBody_X[], int snakeBody_Y[], int snake_length);
void spawn_food(Food* food, int snakeBody_X[], int snakeBody_Y[], int snake_length);

void restart_game();
//void update_block();
void change_direction(int direction);
void CheckState(int btn_val);

#endif
