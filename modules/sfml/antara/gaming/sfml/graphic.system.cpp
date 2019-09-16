/******************************************************************************
 * Copyright © 2013-2019 The Komodo Platform Developers.                      *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Komodo Platform software, including this file may be copied, modified,     *
 * propagated or distributed except according to the terms contained in the   *
 * LICENSE file                                                               *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#include "antara/gaming/ecs/component.position.hpp"
#include "antara/gaming/ecs/component.layer.hpp"
#include "antara/gaming/sfml/graphic.system.hpp"
#include "antara/gaming/sfml/component.drawable.hpp"

namespace antara::gaming::sfml
{
    graphic_system::graphic_system(entt::registry &registry, entt::dispatcher &dispatcher) noexcept : system(registry,
                                                                                                             dispatcher)
    {
    }

    void graphic_system::update() noexcept
    {
        window_.clear();
        draw_each_layers();
        window_.display();
    }

    sf::RenderWindow &graphic_system::get_window() noexcept
    {
        return window_;
    }

    template<size_t Layer, typename DrawableType>
    void graphic_system::draw() noexcept
    {
        this->entity_registry_.view<DrawableType, ecs::layer<Layer>>().each(
                [this](auto entity,
                       auto &&drawable,
                       [[maybe_unused]] auto &&) {
                    if constexpr (std::is_base_of_v<sf::Transformable, decltype(DrawableType::drawable)>) {
                        if (auto cmp_position = this->entity_registry_.try_get<ecs::component_position>(entity);
                                cmp_position != nullptr) {
                            drawable.drawable.setPosition(cmp_position->pos_x, cmp_position->pos_y);
                        }
                    }
                    this->window_.draw(drawable.drawable);
                });
    }

    template<size_t Layer, typename... DrawableType>
    void graphic_system::draw(doom::meta::list<DrawableType...>) noexcept
    {
        (draw<Layer, DrawableType>(), ...);
    }

    template<size_t... Is>
    void graphic_system::draw_each_layers(std::index_sequence<Is...>) noexcept
    {
        (draw<Is>(drawable_list{}), ...);
    }

    void graphic_system::draw_each_layers() noexcept
    {
        draw_each_layers(std::make_index_sequence<ecs::max_layer>{});
    }
}