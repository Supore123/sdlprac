#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <cJSON.h>
#include "app.h"

#define TILE_SIZE 32

typedef struct
{
    int width;
    int height;
    int tile_width;
    int tile_height;
    int *tile_data; // Tile indices (flattened array)
} TileLayer;

typedef struct
{
    SDL_Texture *texture;
    int tile_width;
    int tile_height;
    int columns; // Number of columns in the tileset
} Tileset;

typedef struct
{
    TileLayer *layers;
    int num_layers;
    Tileset *tileset;
    int map_width;
    int map_height;
} Map;

// Function prototypes
int load_map(const char *file_path, Map *map);
void render_map(SDL_Renderer *renderer, Map *map);

appStatus_t mapInit()
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    // Initialize SDL_image
    if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0)
    {
        SDL_Log("IMG_Init Error: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    // Create an SDL window
    SDL_Window *window = SDL_CreateWindow("SDL2 Tiled Map Rendering",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          800, 600, SDL_WINDOW_SHOWN);
    if (!window)
    {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create an SDL renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load map from Tiled JSON file
    Map map;
    if (load_map("imgs/json/map.tmj", &map) != 0)
    {
        SDL_Log("Failed to load map");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Main game/render loop
    SDL_bool running = SDL_TRUE;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = SDL_FALSE;
            }
        }

        // Clear the screen (optional)
        SDL_RenderClear(renderer);

        // Call render_map to render the tiles
        render_map(renderer, &map);

        // Present the rendered image
        SDL_RenderPresent(renderer);

        // Delay for a short time (optional)
        SDL_Delay(16); // Approximately 60 FPS
    }

    // Clean up resources
    SDL_DestroyTexture(map.tileset->texture);
    free(map.tileset);
    free(map.layers);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}

int load_map(const char *file_path, Map *map)
{
    // Open the JSON file
    FILE *file = fopen(file_path, "r");
    if (!file)
    {
        SDL_Log("Failed to open file: %s", file_path);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer)
    {
        SDL_Log("Memory allocation failed");
        fclose(file);
        return -1;
    }

    fread(buffer, 1, file_size, file);
    fclose(file);
    buffer[file_size] = '\0'; // Null-terminate the string

    // Parse JSON
    cJSON *json = cJSON_Parse(buffer);
    free(buffer);
    if (!json)
    {
        SDL_Log("Error parsing JSON: %s", cJSON_GetErrorPtr());
        return -1;
    }

    // Extract map dimensions
    cJSON *width_json = cJSON_GetObjectItem(json, "width");
    cJSON *height_json = cJSON_GetObjectItem(json, "height");
    cJSON *tile_width_json = cJSON_GetObjectItem(json, "tilewidth");
    cJSON *tile_height_json = cJSON_GetObjectItem(json, "tileheight");
    cJSON *layers_json = cJSON_GetObjectItem(json, "layers");
    cJSON *tilesets_json = cJSON_GetObjectItem(json, "tilesets");

    if (!width_json || !height_json || !tile_width_json || !tile_height_json || !layers_json || !tilesets_json)
    {
        SDL_Log("Missing required fields in JSON");
        cJSON_Delete(json);
        return -1;
    }

    map->map_width = width_json->valueint;
    map->map_height = height_json->valueint;
    map->tileset = (Tileset *)malloc(sizeof(Tileset));
    map->tileset->tile_width = tile_width_json->valueint;
    map->tileset->tile_height = tile_height_json->valueint;

    // Load the tileset texture
    cJSON *tileset_json = cJSON_GetArrayItem(tilesets_json, 0);
    const char *tileset_image = cJSON_GetObjectItem(tileset_json, "image")->valuestring;
    map->tileset->texture = IMG_LoadTexture(NULL, tileset_image); // Assuming the texture is in the working directory

    if (!map->tileset->texture)
    {
        SDL_Log("Failed to load tileset texture: %s", IMG_GetError());
        cJSON_Delete(json);
        return -1;
    }

    // Read layer data
    map->num_layers = cJSON_GetArraySize(layers_json);
    map->layers = (TileLayer *)malloc(sizeof(TileLayer) * map->num_layers);
    for (int i = 0; i < map->num_layers; i++)
    {
        cJSON *layer_json = cJSON_GetArrayItem(layers_json, i);
        cJSON *data_json = cJSON_GetObjectItem(layer_json, "data");

        map->layers[i].tile_data = (int *)malloc(sizeof(int) * map->map_width * map->map_height);
        cJSON *tile_json = data_json->child;
        int index = 0;
        while (tile_json)
        {
            map->layers[i].tile_data[index++] = tile_json->valueint;
            tile_json = tile_json->next;
        }
    }

    cJSON_Delete(json);
    return 0;
}

void render_map(SDL_Renderer *renderer, Map *map)
{
    // Iterate through each layer
    for (int layer_idx = 0; layer_idx < map->num_layers; layer_idx++)
    {
        TileLayer *layer = &map->layers[layer_idx];

        // Iterate through each tile in the layer
        for (int y = 0; y < map->map_height; y++)
        {
            for (int x = 0; x < map->map_width; x++)
            {
                // Get the tile index for the current position
                int tile_index = layer->tile_data[y * map->map_width + x];
                if (tile_index == 0)
                    continue; // Skip empty tiles (index 0)

                // Query the tileset texture dimensions
                int tileset_width, tileset_height;
                SDL_QueryTexture(map->tileset->texture, NULL, NULL, &tileset_width, &tileset_height);

                // Calculate the source rect for the tile in the tileset image
                int tile_x = (tile_index - 1) % (tileset_width / map->tileset->tile_width);
                int tile_y = (tile_index - 1) / (tileset_width / map->tileset->tile_width);

                SDL_Rect src_rect = {tile_x * map->tileset->tile_width, tile_y * map->tileset->tile_height, map->tileset->tile_width, map->tileset->tile_height};
                SDL_Rect dest_rect = {x * map->tileset->tile_width, y * map->tileset->tile_height, map->tileset->tile_width, map->tileset->tile_height};

                // Render the tile
                SDL_RenderCopy(renderer, map->tileset->texture, &src_rect, &dest_rect);
            }
        }
    }
}
