#include <iostream> // For file manipulation
#include <vector>   // For using std::vector to represent the grid
#include <cstdlib>  // For rand(), srand(), atoi()
#include <chrono>   // For general timming
#include <omp.h>    // For OpenMP parallelization
#include <numeric>

#include <cmath> //Because compiling with windows or c++ 20 or whatever apparently might autoinclude atleast these 2?
#include <cstdint> //^^


// --- Function Declarations ---
// Initializes a 2D grid with random live (1) or dead (0) cells.
std::vector<std::vector<int>> initialize_grid(int HEIGHT, int WIDTH);

// Counts the number of live neighbors for a given cell (y, x) on the grid.
// Handles non-wrapping boundaries.
int count_live_neighbors(const std::vector<std::vector<int>>& grid, int y, int x, int HEIGHT, int WIDTH);

// Applies Conway's Game of Life rules to the current grid to calculate the next generation.
// This function is parallelized using OpenMP.
std::vector<std::vector<int>> update_grid(const std::vector<std::vector<int>>& current_grid, int HEIGHT, int WIDTH);

// Prints the current state of the grid to the console.
// Live cells are represented by '#', dead cells by ' '.
void print_grid(const std::vector<std::vector<int>>& grid);

// Annoying to rewrite
void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name
              << " [width] [height] [generations] [display_interval] [num_threads] [seed]\n"
              << "  width            - Grid width (positive integer)\n"
              << "  height           - Grid height (positive integer)\n"
              << "  generations      - Number of generations to simulate (non-negative integer)\n"
              << "  display_step - Interval for displaying grid in output (0 no playback)\n"
              << "  num_threads      - Number of OpenMP threads to use (1 to max available)\n"
              << "  seed             - Random seed (unsigned integer)\n";
}

int main(int argc, char* argv[]) {
    // Default values
    int WIDTH = 20;
    int HEIGHT =  20;
    int NUM_GENERATIONS = 100;
    int display_step = 50;
    int num_threads = omp_get_max_threads();
    unsigned int seed = 1749994962; // It made a dog looking grouping once, can't remember the other setting though :/

    if (argc > 7) {
        std::cerr << "Error: Too many arguments.\n";
        print_usage(argv[0]);
        return 1;
    }

    // Parse and validate arguments
    if (argc > 1) WIDTH = std::atoi(argv[1]);
    if (argc > 2) HEIGHT = std::atoi(argv[2]);
    if (argc > 3) NUM_GENERATIONS = std::atoi(argv[3]);
    if (argc > 4) display_step = std::atoi(argv[4]);
    if (argc > 5) {
        int requested_threads = std::atoi(argv[5]);
        if (requested_threads <= 0 || requested_threads > omp_get_max_threads()) {
            std::cerr << "Error: Invalid number of threads.\n";
            print_usage(argv[0]);
            return 1;
        }
        num_threads = requested_threads;
    }
    if (argc > 6) seed = static_cast<unsigned int>(std::atoi(argv[6]));

    std::srand(seed);

    if (WIDTH <= 0 || HEIGHT <= 0) {
        std::cerr << "Error: Grid dimensions must be positive.\n";
        print_usage(argv[0]);
        return 1;
    }
    if (NUM_GENERATIONS < 0) {
        std::cerr << "Error: Number of generations cannot be negative.\n";
        print_usage(argv[0]);
        return 1;
    }


    omp_set_num_threads(num_threads);


    // Display simulation parameters
    std::cout << "--- Conway's Game of Life Parallel Benchmark ---" << std::endl;
    std::cout << "Grid Dimensions: " << WIDTH << "x" << HEIGHT << std::endl;
    std::cout << "Total Generations: " << NUM_GENERATIONS << std::endl;
    std::cout << "Number of Threads: " << num_threads << std::endl;
    if (display_step == 0)
    {
        std::cout << "Not displaying grid, since display_step is 0" << std::endl;
    }
    else
    {
        std::cout << "Grid displaying interval: " << display_step << " generations" << std::endl;
    }

    std::cout << "Seed: " << seed << std::endl;
    std::cout << "---------------------------------------------" << std::endl ;

    // Initialize the starting grid with random cell states
    std::vector<std::vector<int>> current_grid = initialize_grid(HEIGHT, WIDTH);

    // Vector to store the time taken for each generation update
    std::vector<double> generation_times;
    generation_times.reserve(NUM_GENERATIONS); // Pre-allocate memory for efficiency

    std::vector<std::vector<std::vector<int>>> buffered_grids;
    if (display_step==0);
    else
    {
        // Creating a buffer for the grids to be displayed at the end
        buffered_grids.reserve((NUM_GENERATIONS / display_step) + 2);
    }



    // Record the total simulation start time
    auto total_start_time = std::chrono::high_resolution_clock::now();

    // --- Main Simulation Loop ---
    for (int generation = 0; generation < NUM_GENERATIONS; ++generation) {

        auto start_time = std::chrono::high_resolution_clock::now(); // Record the start time for the current generation's update

        current_grid = update_grid(current_grid, HEIGHT, WIDTH);// Update the grid to the next generation using the parallelized function

        auto end_time = std::chrono::high_resolution_clock::now();// Record the end time for the current generation's update
        std::chrono::duration<double, std::milli> duration_ms = end_time - start_time;// Calculate the duration of this generation's update in milliseconds
        generation_times.push_back(duration_ms.count()); // Store the duration

        if (display_step==0); // Skip if no display step provided
        else
        {
            if ((generation + 1) % display_step == 0 || generation == NUM_GENERATIONS - 1) {
                buffered_grids.push_back(current_grid); // Store a copy of the current grid
            }
        }

    }

    // Record the total simulation end time
    auto total_end_time = std::chrono::high_resolution_clock::now();
    // Calculate the total duration of the simulation
    std::chrono::duration<double, std::milli> total_duration_ms = total_end_time - total_start_time;





    // --- Playback Buffered Grids ---
    if (display_step==0)
    {
        std::cout << "--- There is no playback, since display_step is 0 (default)---" << std::endl;
    }
    else{
        std::cout << "--- Playing back buffered grid states ---" << std::endl;
        int buffered_generation_count = 0;
        for (const auto& grid_to_display : buffered_grids) {
            std::cout << "Displaying Buffered Grid " << ++buffered_generation_count*display_step << ":" << std::endl;
            print_grid(grid_to_display);
            std::cout << std::endl;
        }
        std::cout << "--- End of Playback ---" << std::endl;

    }
    std::cout << "---------------------------------------------" << std::endl;
    // Calculate and display the overall average generation time across all simulated generations
    if (!generation_times.empty()) {
        double overall_sum_times = std::accumulate(generation_times.begin(), generation_times.end(), 0.0);
        double overall_avg_time_ms = overall_sum_times / generation_times.size();
        std::cout << "Overall average generation time: " << overall_avg_time_ms << " ms" << std::endl;
    } else {
        std::cout << "No generations were simulated, no average time to report." << std::endl;
    }

    // --- Final Diagnostics (Benchmarks) ---

    std::cout << "Simulation finished after " << NUM_GENERATIONS << " generations." << std::endl;
    std::cout << "Total simulation compute time: " << total_duration_ms.count() / 1000.0 << " seconds." << std::endl;
    return 0;
}


// --- Function Definitions ---

std::vector<std::vector<int>> initialize_grid(int HEIGHT, int WIDTH) {
    std::vector<std::vector<int>> grid(HEIGHT, std::vector<int>(WIDTH));
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            grid[y][x] = rand() % 2; // Randomly assign 0 (dead) or 1 (live)
        }
    }
    return grid;
}


int count_live_neighbors(const std::vector<std::vector<int>>& grid, int y, int x, int HEIGHT, int WIDTH) {
    int live_neighbors = 0;
    // Iterate over the 8 surrounding cells (3x3 neighborhood excluding the center)

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {

            if (dy == 0 && dx == 0) continue; // Skip the cell itself

            int ny = y + dy; // Neighbor's row coordinate
            int nx = x + dx; // Neighbor's column coordinate
            if (ny >= 0 && ny < HEIGHT && nx >= 0 && nx < WIDTH) {
                live_neighbors += grid[ny][nx]; // If neighbor is 1, IE alive
            }
        }
    }
    return live_neighbors;
}

std::vector<std::vector<int>> update_grid(const std::vector<std::vector<int>>& current_grid, int HEIGHT, int WIDTH) {
    // Create a new grid for the next generation.
    std::vector<std::vector<int>> next_grid(HEIGHT, std::vector<int>(WIDTH));
    #pragma omp parallel for collapse(2) private(neighbors)
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            // Count live neighbors for the current cell (y, x)
            int neighbors = count_live_neighbors(current_grid, y, x, HEIGHT, WIDTH);

            // Apply Conway's Game of Life rules:
            if (current_grid[y][x] == 1) // Current cell is ALIVE
                {
                // Rule 1: A live cell with fewer than two live neighbours dies (underpopulation).
                // Rule 2: A live cell with two or three live neighbours lives on to the next generation.
                // Rule 3: A live cell with more than three live neighbours dies (overpopulation).
                next_grid[y][x] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            } else // Current cell is DEAD
            {
                // Rule 4: A dead cell with exactly three live neighbours becomes a live cell (reproduction).
                next_grid[y][x] = (neighbors == 3) ? 1 : 0;
            }
        }
    }

    return next_grid;
}

void print_grid(const std::vector<std::vector<int>>& grid) {
    // Iterate through each cell
    for (const auto& row : grid) {
        for (int cell : row) {
            std::cout << (cell ? '#' : ' '); // Print '#' for live (1), ' ' for dead (0)
        }
        std::cout << '\n';
    }
}