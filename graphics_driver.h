#ifndef GRAPHICS_DRIVER
#define GRAPHICS_DRIVER
#define COLOR_SNAKE 0x00FF00 // Green for snake
#define COLOR_FOOD_REGULAR 0xFF0000 // Red for regular food
#define COLOR_FOOD_SPEED_BOOST 0x0000FF // Blue for speed boost
#define COLOR_FOOD_SLOW_DOWN 0xFFA500 // Orange for slow down
#define COLOR_FOOD_REVERSE 0xFF00FF // Magenta for reverse
#define COLOR_FOOD_SHRINK 0x800080 // Purple for shrink
#define COLOR_FOOD_INVINCIBLE 0xFFFF00 // Yellow for invincible
#define COLOR_FOOD_BONUS 0x00FFFF // Cyan for bonus
#define COLOR_OBSTACLE 0x808080 // Gray for obstacles


typedef enum {
    BLOCK_SNAKE,       // Snake block
    BLOCK_OBSTACLE,    // Obstacle block
    BLOCK_BACKGROUND   // Background (erase)
} BlockType;

void DrawFood(int x_coord, int y_coord, int type);
void DrawBlock(int x_coord, int y_coord, bool remove, int type);
void DrawSnake(int size, int x_coord, int y_coord);
void MoveBlock();
void Init_Map();
void Draw_Map();
void DrawSnakeHead(int x_coord, int y, bool remove, int color, bool alive);
#endif /* ADVENTURES_WITH_IP_H_ */
