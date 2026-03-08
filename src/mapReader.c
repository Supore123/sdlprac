#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// Function prototypes updated to accept renderer
int load_map(SDL_Renderer *renderer, const char *file_path, Map *map);
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

    // Initialize map structure pointers to NULL for safe cleanup
    Map map;
    map.tileset = NULL;
    map.layers = NULL;

    // Load map from Tiled JSON file - passing renderer here
    if (load_map(renderer, "imgs/json/map.tmj", &map) != 0)
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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Call render_map to render the tiles
        render_map(renderer, &map);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); 
    }

    // Clean up resources
    if (map.tileset) {
        if (map.tileset->texture) SDL_DestroyTexture(map.tileset->texture);
        free(map.tileset);
    }
    if (map.layers) {
        for(int i = 0; i < map.num_layers; i++) {
            free(map.layers[i].tile_data);
        }
        free(map.layers);
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}

int load_map(SDL_Renderer *renderer, const char *file_path, Map *map)
{
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
    buffer[file_size] = '\0';

    cJSON *json = cJSON_Parse(buffer);
    free(buffer);
    if (!json)
    {
        SDL_Log("Error parsing JSON: %s", cJSON_GetErrorPtr());
        return -1;
    }

    map->map_width = cJSON_GetObjectItem(json, "width")->valueint;
    map->map_height = cJSON_GetObjectItem(json, "height")->valueint;

    map->tileset = (Tileset *)malloc(sizeof(Tileset));
    map->tileset->tile_width = cJSON_GetObjectItem(json, "tilewidth")->valueint;
    map->tileset->tile_height = cJSON_GetObjectItem(json, "tileheight")->valueint;

    // Get the image path from the JSON
    cJSON *tilesets_json = cJSON_GetObjectItem(json, "tilesets");
    cJSON *tileset_node = cJSON_GetArrayItem(tilesets_json, 0);
    const char *tileset_image_path = cJSON_GetObjectItem(tileset_node, "image")->valuestring;

    // FIX: Use the path directly if your JSON already includes 'imgs/'
    // If the JSON says "imgs/png/...", this will now load correctly.
    map->tileset->texture = IMG_LoadTexture(renderer, tileset_image_path);

    if (!map->tileset->texture)
    {
        SDL_Log("Failed to load tileset texture at %s: %s", tileset_image_path, IMG_GetError());
        cJSON_Delete(json);
        return -1;
    }

    cJSON *layers_json = cJSON_GetObjectItem(json, "layers");
    map->num_layers = cJSON_GetArraySize(layers_json);
    map->layers = (TileLayer *)malloc(sizeof(TileLayer) * map->num_layers);

    for (int i = 0; i < map->num_layers; i++)
    {
        cJSON *layer_json = cJSON_GetArrayItem(layers_json, i);
        cJSON *data_json = cJSON_GetObjectItem(layer_json, "data");

        map->layers[i].tile_data = (int *)malloc(sizeof(int) * map->map_width * map->map_height);
        cJSON *tile_node = data_json->child;
        int index = 0;
        while (tile_node && index < (map->map_width * map->map_height))
        {
            map->layers[i].tile_data[index++] = tile_node->valueint;
            tile_node = tile_node->next;
        }
    }

    cJSON_Delete(json);
    return 0;
}

void render_map(SDL_Renderer *renderer, Map *map)
{
    if (!map->tileset || !map->tileset->texture) return;

    int tileset_width, tileset_height;
    SDL_QueryTexture(map->tileset->texture, NULL, NULL, &tileset_width, &tileset_height);
    int cols_in_tileset = tileset_width / map->tileset->tile_width;

    for (int layer_idx = 0; layer_idx < map->num_layers; layer_idx++)
    {
        TileLayer *layer = &map->layers[layer_idx];

        for (int y = 0; y < map->map_height; y++)
        {
            for (int x = 0; x < map->map_width; x++)
            {
                int tile_index = layer->tile_data[y * map->map_width + x];
                if (tile_index == 0) continue; 

                // Tiled indices are 1-based (0 is empty)
                int local_index = tile_index - 1;
                int tile_x = local_index % cols_in_tileset;
                int tile_y = local_index / cols_in_tileset;

                SDL_Rect src_rect = {
                    tile_x * map->tileset->tile_width, 
                    tile_y * map->tileset->tile_height, 
                    map->tileset->tile_width, 
                    map->tileset->tile_height
                };
                
                SDL_Rect dest_rect = {
                    x * map->tileset->tile_width, 
                    y * map->tileset->tile_height, 
                    map->tileset->tile_width, 
                    map->tileset->tile_height
                };

                SDL_RenderCopy(renderer, map->tileset->texture, &src_rect, &dest_rect);
            }
        }
    }
}
