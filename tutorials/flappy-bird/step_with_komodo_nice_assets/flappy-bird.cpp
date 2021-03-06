#include <random>
#include <antara/gaming/ecs/virtual.input.system.hpp>
#include <antara/gaming/collisions/basic.collision.system.hpp>
#include <antara/gaming/graphics/component.layer.hpp>
#include <antara/gaming/graphics/component.canvas.hpp>
#include <antara/gaming/math/vector.hpp>
#include <antara/gaming/scenes/scene.manager.hpp>
#include <antara/gaming/sfml/graphic.system.hpp>
#include <antara/gaming/sfml/input.system.hpp>
#include <antara/gaming/sfml/resources.manager.hpp>
#include <antara/gaming/world/world.app.hpp>
#include <antara/gaming/graphics/component.sprite.hpp>
#include <antara/gaming/input/virtual.hpp>
#include <antara/gaming/animation2d/component.animation.2d.hpp>
#include <antara/gaming/animation2d/animation.2d.hpp>

// For convenience
using namespace antara::gaming;
using namespace std::string_literals;

// Constants
struct flappy_bird_constants {
    // UI
    const unsigned long long font_size{32ull};

    // Player
    const float player_pos_x{400.0f};
    const float gravity{2000.f};
    const float jump_force{650.f};
    const float rotate_speed{100.f};
    const float max_angle{60.f};
    const float jump_rotation{-40.f};
    const float fall_angle{-5.f};
    const math::vec2f collision_box_size{40.f, 40.f};

    // Pipes
    const float gap_height{265.f};
    const float column_start_distance{700.f};
    const float column_min{0.2f};
    const float column_max{0.8f};
    const int pipe_body_image_width{175};
    const float column_thickness{100.f};
    const float column_scale{column_thickness / pipe_body_image_width};
    const int pipe_cap_image_height{127};
    const float column_distance{400.f};
    const std::size_t column_count{6};
    const float pipe_cap_height{column_scale * pipe_cap_image_height};
    const float scroll_speed{200.f};

    // Background
    const float ground_thickness{100.0f};
    const float grass_thickness{20.0f};
    const int background_image_height{2048};
    const graphics::color ground_color{220, 209, 143};
    const graphics::color grass_color{176, 218, 110};
    const graphics::outline_color grass_outline_color{2.0f, graphics::color{76, 47, 61}};
};

// Random number generator
namespace {
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    float random_float(float lower, float higher) {
        std::uniform_real_distribution<float> dist(lower, higher);
        return dist(gen);
    }
}

// A Flappy Bird column which has two pipes
struct pipe {
    entt::entity body{entt::null};
    entt::entity cap{entt::null};

    // Destroy pipe
    void destroy(entt::registry &registry) {
        registry.destroy(body);
        registry.destroy(cap);
    }
};

// Column is made of two pipes
struct column {
    // Entities representing the Flappy Bird pipes
    pipe top_pipe{entt::null};
    pipe bottom_pipe{entt::null};

    // Is score taken from this column
    bool scored{false};

    // Destroy pipes and this column
    void destroy(entt::registry &registry, entt::entity entity) {
        top_pipe.destroy(registry);
        bottom_pipe.destroy(registry);
        registry.destroy(entity);
    }
};

// Score struct, has current value, max record, and the UI text
struct score {
    int value;
    int max_score;
    entt::entity text;
};

// Logic functions
namespace {
    void tag_game_scene(entt::registry &registry, entt::entity entity, bool dynamic = false) {
        // Tag game scene
        registry.assign<entt::tag<"game_scene"_hs>>(entity);

        // Tag dynamic
        if (dynamic) registry.assign<entt::tag<"dynamic"_hs>>(entity);
    }

    // Create the UI string
    std::string score_ui_text(int score = 0, int best_score = 0) {
        return "Score: "s + std::to_string(score) +
               "\nBest: "s + std::to_string(best_score) +
               "\n\nW / UP / Space / Mouse to FLAP"s;
    }

    // Update score
    void update_score(entt::registry &registry, entt::entity entity, bool reset = false) {
        score &sc = registry.get<score>(entity);

        // If reset is asked, set score to 0
        if (reset) sc.value = 0;
            // Else, increase the score,
            // Compare it with the max score, and update max score if it's greater
        else if (++sc.value > sc.max_score) sc.max_score = sc.value;

        // Update the score entity
        registry.replace<score>(entity, sc);

        // Update the UI text entity with the current values
        auto &text = registry.get<graphics::text>(sc.text);
        text.contents = score_ui_text(sc.value, sc.max_score);
        registry.replace<graphics::text>(sc.text, text);
    }

    // Returns a random gap start position Y
    float get_random_gap_start_pos(const entt::registry &registry) {
        // Retrieve constants
        const auto canvas_height = registry.ctx<graphics::canvas_2d>().canvas.size.y();
        const auto constants = registry.ctx<flappy_bird_constants>();

        float top_limit = canvas_height * constants.column_min;
        float bottom_limit = canvas_height * constants.column_max - constants.gap_height;

        return random_float(top_limit, bottom_limit);
    }
}

// Factory functions
namespace {
    // Factory to create score entity
    entt::entity create_score(entt::registry &registry) {
        // Retrieve constants
        const auto[canvas_width, canvas_height] = registry.ctx<graphics::canvas_2d>().canvas.size;
        const auto constants = registry.ctx<flappy_bird_constants>();

        // Create a fresh entity
        auto entity = registry.create();

        // Create text
        auto text_entity = graphics::blueprint_text(registry, graphics::text{score_ui_text(), constants.font_size},
                                                    transform::position_2d{canvas_width * 0.03f, canvas_height * 0.03f},
                                                    graphics::white);

        registry.assign<graphics::layer<9>>(text_entity);
        tag_game_scene(registry, text_entity);

        // Create score
        registry.assign<score>(entity, 0, 0, text_entity);
        registry.assign<entt::tag<"high_score"_hs>>(entity);
        tag_game_scene(registry, entity);

        return entity;
    }

    // Factory for pipes, requires to know if it's a top one, position x of the column, and the gap starting position Y
    pipe create_pipe(entt::registry &registry, bool is_top, float pos_x, float gap_start_pos_y) {
        // Retrieve constants
        const auto canvas_height = registry.ctx<graphics::canvas_2d>().canvas.size.y();
        const auto constants = registry.ctx<flappy_bird_constants>();

        // PIPE BODY
        // Top pipe is at Y: 0 and bottom pipe is at canvas_height, bottom of the canvas
        transform::position_2d body_pos{pos_x, is_top ? 0.f : canvas_height};

        // Scale X is the column scale,
        // Scale Y is the important part. Since our pipe_body is 1px, scale will be the size.
        // If it's a top pipe, gap_start_pos_y should be bottom of the rectangle
        //  So half size should be gap_start_pos_y since center of the rectangle is at 0.
        // If it's the bottom pipe, top of the rectangle will be at gap_start_pos_y + gap_height
        //  So half size should be canvas_height - (gap_start_pos_y + gap_height)
        // Since these are half-sizes, and the position is at the screen border, we multiply these sizes by two
        math::vec2f body_size{constants.column_scale,
                              is_top ?
                              gap_start_pos_y * 2.0f :
                              (canvas_height - (gap_start_pos_y + constants.gap_height)) * 2.0f};

        auto body = graphics::blueprint_sprite(registry, graphics::sprite{"pipe_body.png"}, body_pos, graphics::white,
                                              transform::properties{ body_size });

        // PIPE CAP
        // Let's prepare the pipe cap
        // Position, X is same as the body. Bottom of the cap is aligned with bottom of the body,
        // or start of the gap, we will use start of the gap here, minus half of the cap height
        transform::position_2d cap_pos{body_pos.x(),
                                       is_top ?
                                       gap_start_pos_y - constants.pipe_cap_height * 0.5f :
                                       gap_start_pos_y + constants.gap_height + constants.pipe_cap_height * 0.5f
        };

        // Construct the cap
        // Size of the cap is defined in constants
        math::vec2f cap_size{constants.column_scale,
                            constants.column_scale * (is_top ? -1.f : 1.f)};

        auto cap = graphics::blueprint_sprite(registry, graphics::sprite{"pipe_cap.png"}, cap_pos, graphics::white,
        transform::properties{cap_size});

        // Set layers, cap should be in front of body
        registry.assign<graphics::layer<4>>(cap);
        registry.assign<graphics::layer<3>>(body);
        tag_game_scene(registry, cap, true);
        tag_game_scene(registry, body, true);

        // Construct a pipe with body and cap and return it
        return {body, cap};
    }

    // Factory to create single column
    void create_column(entt::registry &registry, float pos_x) noexcept {
        // Create a fresh entity for a new column
        auto entity_column = registry.create();

        // Get a random gap start position Y, between pipes
        float gap_start_pos_y = get_random_gap_start_pos(registry);

        // Create pipes, is_top variable is false for bottom one
        auto top_pipe = create_pipe(registry, true, pos_x, gap_start_pos_y);
        auto bottom_pipe = create_pipe(registry, false, pos_x, gap_start_pos_y);

        // Make a column from these two pipes and mark it as "column"
        registry.assign<column>(entity_column, top_pipe, bottom_pipe);
        registry.assign<entt::tag<"column"_hs>>(entity_column);
        tag_game_scene(registry, entity_column, true);
    }

    // Factory for creating a Flappy Bird columns
    void create_columns(entt::registry &registry) noexcept {
        // Retrieve constants
        const auto constants = registry.ctx<flappy_bird_constants>();

        // Spawn columns out of the screen, out of the canvas
        const float column_pos_offset = constants.column_start_distance + constants.column_thickness * 2.0f;

        // Create the columns
        for (std::size_t i = 0; i < constants.column_count; ++i) {
            // Horizontal position (X) increases for every column, keeping the distance
            float pos_x = column_pos_offset + i * constants.column_distance;

            create_column(registry, pos_x);
        }
    }

    // Factory for creating a Flappy Bird background
    void create_background(entt::registry &registry) noexcept {
        // Retrieve constants
        const auto[canvas_width, canvas_height] = registry.ctx<graphics::canvas_2d>().canvas.size;
        const auto constants = registry.ctx<flappy_bird_constants>();

        // Create Sky
        {
            // Sky is whole canvas so position is middle of it
            transform::position_2d pos{canvas_width * 0.5f, canvas_height * 0.5f};

            // And the size is full canvas
            float scale = canvas_height / constants.background_image_height;

            auto sky = graphics::blueprint_sprite(registry, graphics::sprite{"background.png"}, pos, graphics::white,
                                                  transform::properties{{scale, scale}});

            registry.assign<graphics::layer<1>>(sky);
            tag_game_scene(registry, sky);
        }

        // Create Grass
        {
            // Ground expands to whole canvas width so position is middle of it,
            // But position Y is at top of the ground, so it's canvas height minus ground thickness
            transform::position_2d pos{canvas_width * 0.5f, canvas_height - constants.ground_thickness};

            // Size X is full canvas but the height is defined in constants
            // We also make it a bit longer by adding the thickness of the outline to hide the outline at sides
            math::vec2f size{canvas_width + constants.grass_outline_color.thickness * 2.0f, constants.grass_thickness};

            auto grass = geometry::blueprint_rectangle(registry, size, constants.grass_color, pos,
                                                       constants.grass_outline_color);
            registry.assign<graphics::layer<3>>(grass);
            tag_game_scene(registry, grass);
        }

        // Create Ground
        {
            // Ground expands to whole canvas width so position is middle of it,
            // But position Y is at bottom of the screen so it's full canvas_height minus half of the ground thickness
            transform::position_2d pos{canvas_width * 0.5f, canvas_height - constants.ground_thickness * 0.5f};

            // Size X is full canvas but the height is defined in constants
            math::vec2f size{canvas_width, constants.ground_thickness};

            auto ground = geometry::blueprint_rectangle(registry, size, constants.ground_color, pos);
            registry.assign<graphics::layer<3>>(ground);
            tag_game_scene(registry, ground);
        }
    }

    // Factory for creating the player
    entt::entity create_player(entt::registry &registry) {
        // Retrieve constants
        const auto[_, canvas_height] = registry.ctx<graphics::canvas_2d>().canvas.size;
        const auto constants = registry.ctx<flappy_bird_constants>();

        /*auto entity = graphics::blueprint_sprite(registry,
                                                 graphics::sprite{constants.player_image_name.c_str()},
                                                 transform::position_2d{constants.player_pos_x, canvas_height * 0.5f});*/

        auto entity = animation2d::blueprint_animation(registry,
                                                       animation2d::anim_component{.animation_id = "dragon_jump",
                                                               .current_status = animation2d::anim_component::status::paused,
                                                               .speed = animation2d::anim_component::seconds(0.13f),
                                                               .loop = true},
                                                       transform::position_2d{constants.player_pos_x,
                                                                              canvas_height * 0.5f}, graphics::white,
                                                       transform::properties{.scale = 0.22f});
        registry.assign<antara::gaming::graphics::layer<5>>(entity);
        registry.assign<entt::tag<"player"_hs>>(entity);
        tag_game_scene(registry, entity, true);

        return entity;
    }
}

// Column Logic System
class column_logic final : public ecs::logic_update_system<column_logic> {
public:
    explicit column_logic(entt::registry &registry, entt::entity score) noexcept : system(registry),
                                                                                   score_entity_(score) {
        disable();
    }

    // Update, this will be called every tick
    void update() noexcept final {
        auto &registry = entity_registry_;

        // Retrieve constants
        const auto constants = registry.ctx<flappy_bird_constants>();

        // Loop all columns
        for (auto entity : registry.view<column>()) {
            auto &col = registry.get<column>(entity);

            // Move pipes, and retrieve column position x
            float column_pos_x = move_pipe(registry, col.top_pipe);
            move_pipe(registry, col.bottom_pipe);

            // If this column is not scored, and player passed this column
            if (!col.scored && column_pos_x < constants.player_pos_x) {
                // Increase the score
                update_score(registry, score_entity_);

                // Set column as scored
                col.scored = true;
            }

            // If column is out of the screen
            if (column_pos_x < -constants.column_distance) {
                // Remove this column
                col.destroy(registry, entity);

                // Create a new column at far end
                create_column(registry, furthest_pipe_position(registry) + constants.column_distance);
            }
        }
    }

private:
    entt::entity score_entity_;

    // Find the furthest pipe's position X
    float furthest_pipe_position(entt::registry &registry) {
        float furthest = 0.f;

        for (auto entity : registry.view<column>()) {
            auto &col = registry.get<column>(entity);
            float x = entity_registry_.get<transform::position_2d>(col.top_pipe.body).x();
            if (x > furthest) furthest = x;
        }

        return furthest;
    }

    // Move the pipe and return the x position
    float move_pipe(entt::registry &registry, pipe &pipe) {
        // Retrieve constants
        const auto constants = registry.ctx<flappy_bird_constants>();

        // Get current position of the pipe
        auto pos = registry.get<transform::position_2d>(pipe.body);

        // Shift pos X to left by scroll_speed but multiplying with dt because we do this so many times a second,
        // Delta time makes sure that it's applying over time, so in one second it will move scroll_speed pixels
        auto new_pos_x = pos.x() - constants.scroll_speed * timer::time_step::get_fixed_delta_time();

        // Set the new position value
        registry.replace<transform::position_2d>(pipe.body, new_pos_x, pos.y());

        // Set cap position too
        auto cap_pos = registry.get<transform::position_2d>(pipe.cap);
        registry.replace<transform::position_2d>(pipe.cap, new_pos_x, cap_pos.y());

        // Return the info about if this pipe is out of the screen
        return new_pos_x;
    }
};

// Name this system
REFL_AUTO (type(column_logic));

// Player Logic System
class player_logic final : public ecs::logic_update_system<player_logic> {
public:
    player_logic(entt::registry &registry, entt::entity player) noexcept : system(registry), player_(player) {
        disable();
    }

    // Update, this will be called every tick
    void update() noexcept final {
        auto &registry = entity_registry_;

        // Retrieve constants
        const auto constants = registry.ctx<flappy_bird_constants>();

        // Get current position of the player
        auto pos = registry.get<transform::position_2d>(player_);

        // Add gravity to movement speed, multiply with delta time to apply it over time
        movement_speed_.set_y(movement_speed_.y() + constants.gravity * timer::time_step::get_fixed_delta_time());

        // Check if jump key is tapped
        bool jump_key_tapped = input::virtual_input::is_tapped("jump");

        // If jump is tapped, jump by adding jump force to the movement speed Y
        if (jump_key_tapped) {
            movement_speed_.set_y(-constants.jump_force);

            auto& anim = entity_registry_.get<animation2d::anim_component>(player_);
            anim.animation_id = "dragon_jump";
            entity_registry_.replace<animation2d::anim_component>(player_, anim);
        }

        // Add movement speed to position to make the character move, but apply over time with delta time
        pos += movement_speed_ * timer::time_step::get_fixed_delta_time();

        // Do not let player to go out of the screen to top
        if (pos.y() <= 0.f) {
            pos.set_y(0.f);
            movement_speed_.set_y(0.f);
        }

        // Set the new position value
        registry.replace<transform::position_2d>(player_, pos);

        // ROTATION
        // Retrieve props of the player
        auto &props = registry.get<transform::properties>(player_);

        // Increase the rotation a little by applying delta time
        props.rotation = props.rotation + constants.rotate_speed * timer::time_step::get_fixed_delta_time();

        // If jump button is tapped, reset rotation,
        // If rotation is higher than the max angle, set it to max angle
        if (jump_key_tapped)
            props.rotation = constants.jump_rotation;
        else if (props.rotation > constants.max_angle)
            props.rotation = constants.max_angle;

        // Change to falling animation when angle is down enough
        if(props.rotation > constants.fall_angle) {
            auto &anim = entity_registry_.get<animation2d::anim_component>(player_);
            if(anim.animation_id != "dragon_fall") {
                anim.animation_id = "dragon_fall";
                entity_registry_.replace<animation2d::anim_component>(player_, anim);
            }
        }

        // Set the properties
        registry.replace<transform::properties>(player_, props);
    }

private:
    entt::entity player_;
    math::vec2f movement_speed_{0.f, 0.f};
};

// Name this system
REFL_AUTO (type(player_logic));

// Collision Logic System
class collision_logic final : public ecs::logic_update_system<collision_logic> {
public:
    collision_logic(entt::registry &registry, entt::entity player, bool &player_died) noexcept : system(registry),
                                                                                                 player_(player),
                                                                                                 player_died_(
                                                                                                         player_died) {}

    // Update, this will be called every tick
    void update() noexcept final {
        auto &registry = entity_registry_;

        // Do not check anything if player is already dead
        if (player_died_) return;

        // Check collision
        check_player_pipe_collision(registry);
    }

private:
    // Loop all columns to check collisions between player and the pipes
    void check_player_pipe_collision(entt::registry &registry) {
        const auto constants = registry.ctx<flappy_bird_constants>();

        for (auto entity : registry.view<graphics::layer<3>>()) {
            // Check collision between player and a collidable object
            auto entity_props = entity_registry_.try_get<transform::properties>(entity);
            auto player_pos = entity_registry_.try_get<transform::position_2d>(player_);

            // If this entity has a box
            if(entity_props != nullptr && player_pos != nullptr) {
                transform::ts_rect player_box{
                        {*player_pos - constants.collision_box_size*0.5f},
                        constants.collision_box_size
                };

                if (collisions::basic_collision_system::query_rect(entity_props->global_bounds, player_box)) {
                    // Mark player died as true
                    player_died_ = true;

                    auto& anim = entity_registry_.get<animation2d::anim_component>(player_);
                    anim.animation_id = "dragon_hurt";
                    anim.current_status = animation2d::anim_component::status::stopped;
                    anim.current_frame = 0;
                    entity_registry_.replace<animation2d::anim_component>(player_, anim);
                }
            }
        }
    }

    entt::entity player_;
    bool &player_died_;
};

// Name this system
REFL_AUTO (type(collision_logic));

// Game Scene
class game_scene final : public scenes::base_scene {
public:
    game_scene(entt::registry &registry, ecs::system_manager &system_manager) noexcept : base_scene(registry),
                                                                                         system_manager_(
                                                                                                 system_manager) {
        // Set the constants that will be used in the program
        registry.set<flappy_bird_constants>();

        // Create everything
        score_entity_ = create_score(registry);
        create_background(registry);
        init_dynamic_objects(registry);
    }

    // Scene name
    std::string scene_name() noexcept final {
        return "game_scene";
    }

private:
    // Update the game every tick
    void update() noexcept final {
        // Check if player requested to start the game
        check_start_game_request();

        // Check if player died
        check_death();

        // Check if player requested reset after death
        check_reset_request();
    }

    // Check if start game is requested at the pause state
    void check_start_game_request() {
        // If game is not started yet and jump key is tapped
        if (!started_playing_ && input::virtual_input::is_tapped("jump")) {
            // Game starts, player started playing
            started_playing_ = true;
            if (entity_registry_.valid(player_)) {
                auto &anim = entity_registry_.get<animation2d::anim_component>(player_);
                anim.current_status = animation2d::anim_component::status::playing;
            }
            resume_physics();
        }
    }

    // Check if player died
    void check_death() {
        // If player died, game over, and pause physics
        if (player_died_) {
            player_died_ = false;
            game_over_ = true;
            pause_physics();
        }
    }

    // Check if reset is requested at game over state
    void check_reset_request() {
        // If game is over, and jump key is pressed, reset game
        if (game_over_ && input::virtual_input::is_tapped("jump")) reset_game();
    }

    // Initialize dynamic objects, this function is called at start and resets
    void init_dynamic_objects(entt::registry &registry) {
        create_columns(registry);

        // Create player
        player_ = create_player(registry);

        // Create logic systems
        create_logic_systems(player_);

        // Reset state variables
        reset_state_variables();
    }

    // Create logic systems
    void create_logic_systems(entt::entity player) {
        system_manager_.create_system_rt<column_logic>(score_entity_);
        system_manager_.create_system_rt<player_logic>(player);
        system_manager_.create_system_rt<collision_logic>(player, player_died_);
    }

    // Reset state values
    void reset_state_variables() {
        started_playing_ = false;
        player_died_ = false;
        game_over_ = false;
    }

    // Pause physics
    void pause_physics() {
        system_manager_.disable_systems<column_logic, player_logic>();
    }

    // Resume physics
    void resume_physics() {
        system_manager_.enable_systems<column_logic, player_logic>();
    }

    // Destroy dynamic objects
    void destroy_dynamic_objects() {
        // Retrieve the collection of entities from the game scene
        auto view = entity_registry_.view<entt::tag<"dynamic"_hs>>();

        // Iterate the collection and destroy each entities
        entity_registry_.destroy(view.begin(), view.end());

        // Delete systems
        system_manager_.mark_systems<player_logic, collision_logic>();
    }

    // Reset game
    void reset_game() {
        // Destroy all dynamic objects
        destroy_dynamic_objects();

        // Queue reset to reinitialize
        this->need_reset_ = true;

        // Reset current score, but keep the max score
        update_score(entity_registry_, score_entity_, true);
    }

    // Post update
    void post_update() noexcept final {
        // If reset is requested
        if (need_reset_) {
            // Reinitialize all these
            init_dynamic_objects(entity_registry_);
            need_reset_ = false;
        }
    }

    // System manager reference
    ecs::system_manager &system_manager_;

    // States
    entt::entity score_entity_;
    entt::entity player_{entt::null};
    bool started_playing_{false};
    bool player_died_{false};
    bool game_over_{false};
    bool need_reset_{false};
};

// Game world
struct flappy_bird_world : world::app {
    // Game entry point
    flappy_bird_world() noexcept {
        // Load the graphical system
        auto &graphic_system = system_manager_.create_system<sfml::graphic_system>();

        // Load the resources system
        entity_registry_.set<sfml::resources_system>(entity_registry_);

        // Load the input system with the window from the graphical system
        system_manager_.create_system<sfml::input_system>(graphic_system.get_window());

        auto &anim_system = system_manager_.create_system<antara::gaming::animation2d::anim_system>();

        anim_system.add_animation("dragon_hurt", "dragon_hurt.png", 1, 1, 1);
        anim_system.add_animation("dragon_fall", "dragon_fall.png", 1, 1, 2);
        anim_system.add_animation("dragon_jump", "dragon_jump.png", 1, 1, 2);
        // Create virtual input system
        system_manager_.create_system<ecs::virtual_input_system>();

        // Define the buttons for the jump action
        input::virtual_input::create("jump",
                                     {input::key::space, input::key::w, input::key::up},
                                     {input::mouse_button::left, input::mouse_button::right});

        // Load the scenes manager
        auto &scene_manager = system_manager_.create_system<scenes::manager>();

        // Change the current_scene to "game_scene" by pushing it.
        scene_manager.change_scene(std::make_unique<game_scene>(entity_registry_, system_manager_), true);
    }
};

int main() {
    // Declare the world
    flappy_bird_world game;

    // Run the game
    return game.run();
}