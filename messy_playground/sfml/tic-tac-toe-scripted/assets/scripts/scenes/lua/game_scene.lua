local entities, board = {}, {}
local grid_entity = 0
local canvas_2d = entt.entity_registry:ctx_canvas_2d().canvas.size
local canvas_width, canvas_height = canvas_2d:x(), canvas_2d:y()
local nb_cells_sqrt = 3
local cell_width, cell_height = math.floor(canvas_width / nb_cells_sqrt), math.floor(canvas_height / nb_cells_sqrt)
local grid_thickness = 20
local cell_state = enum({ "empty", "player_x", "player_o" })
local game_state = enum({ "running", "player_x_won", "player_o_won", "tie" })
local player = enum({ "x", "o" }, 2)
local current_player, current_game_state = player.x.id, game_state.running.id

local function create_grid()
    local grid_entity = entt.entity_registry:create()
    local lines = new_array(8 * 4, vertex)
    local half_thickness = grid_thickness * 0.5
    local counter = 1

    for i = 0, nb_cells_sqrt, 1 do
        local offset_x, offset_y = 0.0, 0.0
        if i == 0 then
            offset_x, offset_y = offset_x + half_thickness, offset_y + half_thickness
        elseif i == nb_cells_sqrt then
            offset_x, offset_y = offset_x - half_thickness, offset_y - half_thickness
        end
        lines[counter].pos:set_xy(offset_x + i * cell_width - half_thickness, 0.0)
        lines[counter + 1].pos:set_xy(offset_x + i * cell_width + half_thickness, 0)
        lines[counter + 2].pos:set_xy(offset_x + i * cell_width + half_thickness, canvas_height)
        lines[counter + 3].pos:set_xy(offset_x + i * cell_width - half_thickness, canvas_height)
        lines[counter + 4].pos:set_xy(offset_x + 0, offset_y + i * cell_height - half_thickness)
        lines[counter + 5].pos:set_xy(offset_x + canvas_width, offset_y + i * cell_height - half_thickness)
        lines[counter + 6].pos:set_xy(offset_x + canvas_width, offset_y + i * cell_height + half_thickness)
        lines[counter + 7].pos:set_xy(offset_x + 0, offset_y + i * cell_height + half_thickness)
        counter = counter + 4 * 2
    end

    entt.entity_registry:add_layer_0_component(grid_entity)
    local cmp_vertex_array = vertex_array.new(lines, antara.geometry_type.quads)
    entt.entity_registry:add_by_copy_vertex_array_component(grid_entity, cmp_vertex_array)
    return grid_entity
end

local function enter()
    current_game_state = game_state.running.id
    current_player = player.x.id
    grid_entity = create_grid()
    entities[#entities + 1] = grid_entity
    board = new_array(nb_cells_sqrt * nb_cells_sqrt, cell_state.empty.id)
end

local function leave()
    for key, value in pairs(entities) do entt.entity_registry:destroy(value) entities[key] = nil end
end

local function create_x(row, column)
    local entity_x = entt.entity_registry:create()
    local half_box_side, half_thickness = math.min(cell_width, cell_height) * 0.25, grid_thickness * 0.5
    local center_x, center_y = cell_width * 0.5 + column * cell_width, cell_height * 0.5 + row * cell_height
    local lines = new_array(2 * 4, vertex)
    for idx, value in ipairs(lines) do value.pixel_color = antara.color_magenta end

    lines[1].pos:set_xy(center_x - half_box_side - half_thickness, center_y - half_box_side)
    lines[2].pos:set_xy(center_x - half_box_side + half_thickness, center_y - half_box_side)
    lines[3].pos:set_xy(center_x + half_box_side + half_thickness, center_y + half_box_side)
    lines[4].pos:set_xy(center_x + half_box_side - half_thickness, center_y + half_box_side)
    lines[5].pos:set_xy(center_x + half_box_side - half_thickness, center_y - half_box_side)
    lines[6].pos:set_xy(center_x + half_box_side + half_thickness, center_y - half_box_side)
    lines[7].pos:set_xy(center_x - half_box_side + half_thickness, center_y + half_box_side)
    lines[8].pos:set_xy(center_x - half_box_side - half_thickness, center_y + half_box_side)
    entt.entity_registry:add_layer_1_component(entity_x)
    entt.entity_registry:add_by_copy_vertex_array_component(entity_x, vertex_array.new(lines, antara.geometry_type.quads))
    return entity_x
end

local function create_o(row, column)
    local entity_o = entt.entity_registry:create()
    local half_box_side = math.min(cell_width, cell_height) * 0.25
    local center_x, center_y = cell_width * 0.5 + column * cell_width, cell_height * 0.5 + row * cell_height

    entt.entity_registry:add_by_copy_fill_color_component(entity_o, fill_color.new(antara.color_transparent))
    entt.entity_registry:add_by_copy_outline_color_component(entity_o, outline_color.new(grid_thickness, antara.color_cyan))
    entt.entity_registry:add_by_copy_circle_component(entity_o, circle.new(half_box_side))
    entt.entity_registry:add_by_copy_position_2d_component(entity_o, position_2d.new(center_x, center_y))
    entt.entity_registry:add_layer_1_component(entity_o)
    return entity_o
end

local function is_current_player_winning_the_game()
    local row_count, column_count, diag1_count, diag2_count = 0, 0, 0, 0
    for i = 1, nb_cells_sqrt, 1 do
        for j = 1, nb_cells_sqrt, 1 do
            if board[(i - 1) * nb_cells_sqrt + (j - 1) + 1] == current_player then
                row_count = row_count + 1
            end
            if board[(j - 1) * nb_cells_sqrt + (i - 1) + 1] == current_player then
                column_count = column_count + 1
            end
        end
        if row_count >= nb_cells_sqrt or column_count >= nb_cells_sqrt then
            return true
        end

        row_count, column_count = 0, 0
        if board[(i - 1) * nb_cells_sqrt + (i - 1) + 1] == current_player then
            diag1_count = diag1_count + 1
        end

        if board[(i - 1) * nb_cells_sqrt + nb_cells_sqrt - (i - 1)] == current_player then
            diag2_count = diag2_count + 1
        end
    end
    return diag1_count >= nb_cells_sqrt or diag2_count >= nb_cells_sqrt
end

local function is_tie()
    local nb_empty = 0
    for idx, value in ipairs(board) do
        if value ~= cell_state.empty.id then nb_empty = nb_empty + 1 end
    end
    return nb_empty == nb_cells_sqrt * nb_cells_sqrt
end

local function check_condition()
    if is_current_player_winning_the_game() then
        current_game_state = current_player
        local vx_array = entt.entity_registry:get_vertex_array_component(grid_entity)
        if current_player == player.x.id then
            for idx, value in ipairs(vx_array.vertices) do value.pixel_color = antara.color_magenta end
        else
            for idx, value in ipairs(vx_array.vertices) do value.pixel_color = antara.color_cyan end
        end
        entt.entity_registry:replace_by_copy_vertex_array_component(grid_entity, vx_array)
    elseif is_tie() then
        current_game_state = game_state.tie.id
        local vx_array = entt.entity_registry:get_vertex_array_component(grid_entity)
        for idx, value in ipairs(vx_array.vertices) do value.pixel_color = antara.color_yellow end
        entt.entity_registry:replace_by_copy_vertex_array_component(grid_entity, vx_array)
    end
end

local function play_one_turn(row, column)
    local index = math.floor(row * nb_cells_sqrt + column) + 1
    if index <= #board and board[index] == cell_state.empty.id then
        board[index] = current_player
        if current_player == player.x.id then
            entities[#entities + 1] = create_x(row, column)
            check_condition()
            current_player = player.o.id
        elseif current_player == player.o.id then
            entities[#entities + 1] = create_o(row, column)
            check_condition()
            current_player = player.x.id
        end
    end
end

local function reset()
    leave()
    enter()
end

local function on_mouse_button_pressed(evt)
    if current_game_state == game_state.running.id then
        play_one_turn(math.floor(evt.y / cell_height), math.floor(evt.x / cell_width))
    else
        reset()
    end
end

return { enter = enter, leave = leave, on_mouse_button_pressed = on_mouse_button_pressed, scene_active = true }