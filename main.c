#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// define the size of the image
#define WIDTH 1400
#define HEIGHT 1200

// file path constants
#define OUTPUT_FILE_PATH "output.ppm"

// color constants
#define COLOR_RED 0xFF0000FF
#define COLOR_GREEN 0xFF00FF00
#define COLOR_BLUE 0xFFFF0000
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLACK 0xFF000000
#define SEED_MARKER_RADIUS 5
#define SEED_MARKER_COLOR COLOR_BLACK

// GRUVBOX COLOR PALLETE
#define GRUVBOX_BRIGHT_RED 0xFF3449FB
#define GRUVBOX_BRIGHT_GREEN 0xFF26BBB8
#define GRUVBOX_BRIGHT_YELLOW 0xFF2FBDFA
#define GRUVBOX_BRIGHT_BLUE 0xFF98A583
#define GRUVBOX_BRIGHT_PURPLE 0xFF9B86D3
#define GRUVBOX_BRIGHT_AQUA 0xFF7CC08E
#define GRUVBOX_BRIGHT_ORANGE 0xFF1980FE

// TSODES FAVE COLOR ( GOLDEN KNOWLEDGE )
#define BACKGROUND_COLOR 0xFF181818

// seed count constant
#define SEEDS_COUNT 30

// define a color type
typedef u_int32_t Color32;

// define a Point type/struct
typedef struct
{
    int x, y;
} Point;

typedef struct
{
    uint16_t x;
    uint16_t y;
} Point32;

// allocate some static memory
// rgba
// so the rows of the image matrix are stored sequentally in memory
static Color32 image[HEIGHT][WIDTH];

// static Point32 *image_as_points = image;

// define a seeds constant
static Point seeds[SEEDS_COUNT];

// define color palatte constant
static Color32 palette[] = {
    GRUVBOX_BRIGHT_RED,
    GRUVBOX_BRIGHT_GREEN,
    GRUVBOX_BRIGHT_YELLOW,
    GRUVBOX_BRIGHT_BLUE,
    GRUVBOX_BRIGHT_PURPLE,
    GRUVBOX_BRIGHT_AQUA,
    GRUVBOX_BRIGHT_ORANGE,
};

#define palette_count (sizeof(palette) / sizeof(palette[0]))

void fill_image(Color32 color)
{
    for (size_t y = 0; y < HEIGHT; ++y)
    {
        for (size_t x = 0; x < WIDTH; ++x)
        {
            image[y][x] = color;
        }
    }
}

int sqr_distance(int x1, int y1, int x2, int y2)
{
    int dx = x1 - x2;
    int dy = y1 - y2;
    return dx * dx + dy * dy;
}

void fill_circle(int cx, int cy, int radius, uint32_t color)
{
    // the c in cx and cy is center
    // so the idea here from the tsode is that we find the bounding box of the cirlce
    // and then we have a square inside which we must render a circle?

    // find the bounding box
    int x0 = cx - radius;
    int x1 = cx + radius;

    int y0 = cy - radius;
    int y1 = cy + radius;

    for (int x = x0; x <= x1; ++x)
    {
        // MAKE SURE THIS ISNT SOME INSANE VALUE and is in the range of our matrix
        if (0 <= x && x < WIDTH)
        {
            for (int y = y0; y <= y1; ++y)
            {
                // MAKE SURE THIS ISNT SOME INSANE VALUE and is in the range of our matrix
                if (0 <= y && y < HEIGHT)
                {
                    // 1) find distance between point at current indexes and the center
                    // 1.1) find the change in x and y
                    // 1.2) To find the distance add the sqaures and apply sqrt to that value
                    // int distance = sqrt(dx * dx + dy * dy);
                    // if(distance <= radius)
                    // but as a little perf bonus we can dump the sqrt and then just square the radiues and because math that is fine too
                    // so we do
                    if (sqr_distance(cx, cy, x, y) <= radius * radius)
                    {
                        // if the distance is less than the radius fill the line
                        image[y][x] = color;
                    }
                }
            }
        }
    }
}

void save_image_as_ppm(const char *file_path)
{
    // open the file wb = write binary
    FILE *f = fopen(file_path, "wb");

    // no file found error
    if (f == NULL)
    {
        fprintf(stderr, "Error: could not write into file %s: %s\n", file_path, strerror(errno));
    }

    // Write the appropriate file headers
    // first indicate its a p6 variation of the format
    // on the next line we provide the sizes
    // then we hardcode 255 as the component size or something
    fprintf(f, "P6\n%d %d 255\n", WIDTH, HEIGHT);

    for (size_t y = 0; y < HEIGHT; ++y)
    {
        for (size_t x = 0; x < WIDTH; ++x)
        {
            // so we take the pixel from the image
            // the sape of which is expalined in fill image
            // but that spec defines the rgba parts and we need to rip that out now here
            // 0xAABBGGRR
            uint32_t pixel = image[y][x];

            uint8_t bytes[3] = {
                // so apparently we can extract sht like this?
                // pixel & 0x00FF,
                // so here we extract like above but also shift n bytes to the right
                (pixel & 0x0000FF) >> 0,
                (pixel & 0x00FF00) >> 8,
                (pixel & 0xFF0000) >> 16,

            };
            fwrite(bytes, sizeof(bytes), 1, f);
            // IF THIS ERROR"D DONT GO ON ???
            assert(!ferror(f));
        }
    }
    int ret = fclose(f);
    assert(ret == 0);
}

void generate_random_seeds(void)
{
    // itterate seed count
    for (size_t i = 0; i < SEEDS_COUNT; ++i)
    {
        // set every seeds x and y value to random numbers between 0 and width/height
        seeds[i].x = rand() % WIDTH;
        seeds[i].y = rand() % HEIGHT;
    }
}

void render_seed_markers()
{
    for (size_t i = 0; i < SEEDS_COUNT; ++i)
    {
        // fill the circles so we can se the points
        fill_circle(seeds[i].x, seeds[i].y, SEED_MARKER_RADIUS, SEED_MARKER_COLOR);
    }
}

void render_voronoi_naive()
{

    for (size_t y = 0; y < HEIGHT; ++y)
    {
        for (size_t x = 0; x < WIDTH; ++x)
        {
            // we need to remember the last seed so for now we set it to 0 and assume the first seed is the closest unless we change j
            int j = 0;
            // this seems like a perf weirdness but he loops through the seeds here
            for (size_t i = 0; i < SEEDS_COUNT; ++i)
            {
                // find the distance between seed and point for this seed and the last and if it is less for this one
                if (sqr_distance(seeds[i].x, seeds[i].y, x, y) < sqr_distance(seeds[j].x, seeds[j].y, x, y))
                {
                    j = i;
                }
            }
            // so here we need to set the color based on j to do that we need to create a kind of color pallet
            // so here we apparently wrap around ( or rotates colors something like that  )
            image[y][x] = palette[j % palette_count];
        }
    }
};

Color32 point_to_color(Point p)
{
    // must be positive
    assert(p.x >= 0);
    assert(p.y >= 0);

    // must be less than int limits
    assert(p.x < UINT16_MAX);
    assert(p.y < UINT16_MAX);

    // uint16_t x = p.x;
    // uint16_t y = p.y;

    return (p.y << 16) | p.x;
}

Point color_to_point(Color32 c)
{
    return (Point){
        .x = (c & 0x0000FFFF) >> 0,
        .y = (c & 0xFFFF0000) >> 16,
    };
}

void apply_next_seed_color(Color32 next_seed_color)
{
    Point next_seed = color_to_point(next_seed_color);
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            Point curr_seed = color_to_point(image[y][x]);
            if (sqr_distance(next_seed.x, next_seed.y, x, y) < sqr_distance(curr_seed.x, curr_seed.y, x, y))
            {
                image[y][x] = next_seed_color;
            }
        }
    }
}
// actually slowr
void render_voronoi_interresting()
{
    // so in the last commit we did all the loops
    // no we want to make it better
    // we start with seed a and fill the plane with that color
    // then we take the second seed and only fill closer items
    // etc
    // first render
    fill_image(point_to_color(seeds[0]));

    // we need a function that will take a point and paint that color to the current picture we already added to above and then do that itteratively
    for (int i = 1; i <= SEEDS_COUNT; ++i)
    {
        apply_next_seed_color(point_to_color(seeds[i]));
    }

    // iterate through the seeds and render them
    render_seed_markers();
}

// A vornoi diagram is a plane with random seed points, we then look for the closest point and color acourdingly
int main(void)
{
    // DONT FORGET TO SEED THE RANDON NUMBER GENERATOR XD
    srand(time(0));
    // fill it up with the color red
    // tsoding rambles here about LSB and little endian machines
    // which means to define color we neeed the following template
    // 0x(cause hex)AA(APLHA)BB(BLUE)GG(Green)RR(Red)
    // so the below is 0x Alpha:FF Blue:00 Green:00 Red:FF
    fill_image(BACKGROUND_COLOR);

    // generate the random seeds for the vornoi diagram
    generate_random_seeds();

    // because we have the seeds we can drav the v-diagram also if we dont do this first we wont be able to see
    // the markers because they will be rendered over

    // render_voronoi_interresting();

    render_voronoi_naive();
    render_seed_markers();
    // ppm seems to be a pretty easy image format to grok so tsode decided to go with that
    save_image_as_ppm(OUTPUT_FILE_PATH);
    return 0;
}