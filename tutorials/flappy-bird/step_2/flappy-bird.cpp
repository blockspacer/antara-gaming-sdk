#include <antara/gaming/world/world.app.hpp>
#include <antara/gaming/sfml/graphic.system.hpp>
#include <antara/gaming/sfml/input.system.hpp>
#include <antara/gaming/scenes/scene.manager.hpp>
#include <antara/gaming/math/vector.hpp>
#include <antara/gaming/graphics/component.canvas.hpp>
#include <antara/gaming/graphics/component.layer.hpp>
#include <random>
#include <iostream>

//! For convenience
using namespace antara::gaming;

namespace {
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    float random_float(float lower, float higher) {
        std::uniform_real_distribution<float> dist(lower, higher);
        return dist(gen);
    }
}

struct flappy_bird_constants {
    // Pipes
    const float gap_height{200.f};
    const float column_min{0.2f};
    const float column_max{0.8f};
    const float column_thickness{100.f};
    const float column_distance{400.f};
    const std::size_t column_count{30};
    const graphics::color pipe_color{92, 181, 61};
    const graphics::outline_color pipe_outline_color{2.0f, graphics::color{76, 47, 61}};
};

// A Flappy Bird column which has two pipes
struct column {
    //! Entities representing the Flappy Bird pipes
    entt::entity top_pipe{entt::null};
    entt::entity bottom_pipe{entt::null};
};

//! Contains all the function that will be used for logic  and factory
namespace {

    // Factory for pipes, requires to know if it's a top one, position x of the column, and the gap starting position Y
    entt::entity create_pipe(entt::registry &registry, bool is_top, float pos_x, float gap_start_pos_y) {
        // Retrieve constants
        const auto canvas_height = registry.ctx<graphics::canvas_2d>().canvas.size.y();
        const auto constants = registry.ctx<flappy_bird_constants>();

        // Top pipe is at Y: 0 and bottom pipe is at canvas_height, bottom of the canvas
        transform::position_2d pos{pos_x, is_top ? 0.f : canvas_height};

        // Size X is the column thickness,
        // Size Y is the important part.
        // If it's a top pipe, gap_start_pos_y should be bottom of the rectangle
        //  So half size should be gap_start_pos_y since center of the rectangle is at 0.
        // If it's the bottom pipe, top of the rectangle will be at gap_start_pos_y + gap_height
        //  So half size should be canvas_height - (gap_start_pos_y + gap_height)
        // Since these are half-sizes, and the position is at the screen border, we multiply these sizes by two
        math::vec2f size{constants.column_thickness,
            is_top ?
                gap_start_pos_y * 2.0f :
                (canvas_height - (gap_start_pos_y + constants.gap_height)) * 2.0f};

        auto pipe = geometry::blueprint_rectangle(registry, size, constants.pipe_color, pos, constants.pipe_outline_color);

        registry.assign<graphics::layer<6>>(pipe);

        return pipe;
    }

    // Returns a random gap start position Y
    float get_random_gap_start_pos(entt::registry &registry) {
        //! Retrieve constants
        const auto canvas_height = registry.ctx<graphics::canvas_2d>().canvas.size.y();
        const auto constants = registry.ctx<flappy_bird_constants>();

        float top_limit = canvas_height * constants.column_min;
        float bottom_limit = canvas_height * constants.column_max - constants.gap_height;

        return random_float(top_limit, bottom_limit);
    }

    //! Factory for creating a Flappy Bird columns
    void create_columns(entt::registry &registry) noexcept {
        //! Retrieve constants
        const auto canvas_width = registry.ctx<graphics::canvas_2d>().canvas.size.x();
        const auto constants = registry.ctx<flappy_bird_constants>();

        // Create the columns
        const float column_pos_offset = 0; // canvas_width will be used, we set it to 0 for now to see the pipes
        for(std::size_t i = 0; i < constants.column_count; ++i) {
            // Create a fresh entity for a new column
            auto entity_column = registry.create();

            // Each column has two pipes
            // Horizontal position (X) increases for every column, keeping the distance
            float pos_x = column_pos_offset + i * constants.column_distance;

            // Get a random gap start position Y, between pipes
            float gap_start_pos_y = get_random_gap_start_pos(registry);

            // Create pipes, is_top variable is false for bottom one
            auto top_pipe = create_pipe(registry, true, pos_x, gap_start_pos_y);
            auto bottom_pipe = create_pipe(registry, false, pos_x, gap_start_pos_y);

            // Make a column from these two pipes and mark it as "column"
            registry.assign<column>(entity_column, top_pipe, bottom_pipe);
            registry.assign<entt::tag<"column"_hs>>(entity_column);
        }
    }
}

class game_scene final : public scenes::base_scene {
public:
    game_scene(entt::registry &entity_registry) noexcept : base_scene(entity_registry) {
        //! Set the constants that will be used in the program
        entity_registry_.set<flappy_bird_constants>();

        //! Create the columns
        create_columns(entity_registry_);
    }

    //! Update the game every tick
    void update() noexcept final {}

    //! Scene name
    std::string scene_name() noexcept final {
        return "game_scene";
    }

    ~game_scene() noexcept final {
        //! Retrieve the collection of entities from the game scene
        auto view = entity_registry_.view<entt::tag<"game_scene"_hs>>();

        //! Iterate the collection and destroy each entities
        entity_registry_.destroy(view.begin(), view.end());

        //! Unset the tic tac toe constants
        entity_registry_.unset<flappy_bird_constants>();
    }
};

//! Game world
struct flappy_bird_world : world::app {
    //! Game entry point
    flappy_bird_world() noexcept {
        //! Load our graphical system
        auto &graphic_system = system_manager_.create_system<sfml::graphic_system>();

        //! Load our input system with the window from the graphical system
        system_manager_.create_system<sfml::input_system>(graphic_system.get_window());

        //! Load the scenes manager
        auto &scene_manager = system_manager_.create_system<scenes::manager>();

        //! Change the current_scene to "game_scene" by pushing it.
        scene_manager.change_scene(std::make_unique<game_scene>(entity_registry_), true);
    }
};

int main() {
    // Use current time as seed for random generator
    std::srand(std::time(nullptr));

    //! Declare our world
    flappy_bird_world game;

    //! Run the game
    return game.run();
}