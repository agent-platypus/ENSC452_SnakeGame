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
    FOOD_REGULAR,      // Increases length by 1
    FOOD_SPEED_BOOST,  // Increases speed
    FOOD_SLOW_DOWN,    // Decreases speed
    FOOD_REVERSE,      // Reverses controls
    FOOD_SHRINK,       // Reduces length by 1
    FOOD_INVINCIBLE,   // Makes snake invincible
    FOOD_BONUS,         // Awards extra points

} FoodType;

typedef enum {
    BLOCK_SNAKE,       // Snake block
    BLOCK_OBSTACLE,    // Obstacle block
    BLOCK_BACKGROUND   // Background (erase)
} BlockType;
typedef struct {
    FoodType type;   // Type of food
    int lifetime;   // Remaining lifetime (in timer interrupts)

} Food;

void DrawFood(int x_coord, int y_coord, int type);
void DrawBlock(int x_coord, int y_coord, bool remove, int type);
void DrawSnake(int size, int x_coord, int y_coord);
void MoveBlock();
void Init_Map();
void Draw_Map();
#endif /* ADVENTURES_WITH_IP_H_ */
