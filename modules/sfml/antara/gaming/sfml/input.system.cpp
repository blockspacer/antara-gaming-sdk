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

#include <SFML/Window/Event.hpp>
#include <entt/entity/helper.hpp>
#include "antara/gaming/config/config.game.hpp"
#include "antara/gaming/event/quit.game.hpp"
#include "antara/gaming/event/mouse.button.pressed.hpp"
#include "antara/gaming/event/mouse.button.released.hpp"
#include "antara/gaming/event/mouse.moved.hpp"
#include "antara/gaming/event/key.released.hpp"
#include "antara/gaming/event/key.pressed.hpp"
#include "antara/gaming/sfml/input.system.hpp"

namespace antara::gaming::sfml
{
    input_system::input_system(entt::registry &registry, sf::RenderWindow &window) noexcept
            : system(registry), window_(window)
    {

    }

    void input_system::update() noexcept
    {
        sf::Event evt{};
        while (window_.pollEvent(evt)) {
            switch (evt.type) {
                case sf::Event::Closed:
                    this->dispatcher_.trigger<event::quit_game>(0);
                    break;
                case sf::Event::Resized: {
                    auto &window_component = this->entity_registry_.ctx<config::game_cfg>().win_cfg;
                    window_component.width = evt.size.width;
                    window_component.height = evt.size.height;
                    this->dispatcher_.trigger<entt::tag<"window_resized"_hs>>();
                }
                    break;
                case sf::Event::LostFocus:
                    break;
                case sf::Event::GainedFocus:
                    break;
                case sf::Event::TextEntered:
                    break;
                case sf::Event::KeyPressed:
                    this->dispatcher_.trigger<event::key_pressed>(static_cast<input::key>(evt.key.code), evt.key.alt,
                                                                  evt.key.control, evt.key.shift, evt.key.system);
                    break;
                case sf::Event::KeyReleased:
                    this->dispatcher_.trigger<event::key_released>(static_cast<input::key>(evt.key.code), evt.key.alt,
                                                                   evt.key.control, evt.key.shift, evt.key.system);
                    break;
                case sf::Event::MouseWheelMoved:
                    break;
                case sf::Event::MouseWheelScrolled:
                    break;
                case sf::Event::MouseButtonPressed:
                    this->dispatcher_.trigger<event::mouse_button_pressed>(
                            static_cast<input::mouse_button>(evt.mouseButton.button), evt.mouseButton.x,
                            evt.mouseButton.y);
                    break;
                case sf::Event::MouseButtonReleased:
                    this->dispatcher_.trigger<event::mouse_button_released>(
                            static_cast<input::mouse_button>(evt.mouseButton.button), evt.mouseButton.x,
                            evt.mouseButton.y);
                    break;
                case sf::Event::MouseMoved:
                    this->dispatcher_.trigger<event::mouse_moved>(evt.mouseMove.x, evt.mouseMove.y);
                    break;
                case sf::Event::MouseEntered:
                    break;
                case sf::Event::MouseLeft:
                    break;
                case sf::Event::JoystickButtonPressed:
                    break;
                case sf::Event::JoystickButtonReleased:
                    break;
                case sf::Event::JoystickMoved:
                    break;
                case sf::Event::JoystickConnected:
                    break;
                case sf::Event::JoystickDisconnected:
                    break;
                case sf::Event::TouchBegan:
                    break;
                case sf::Event::TouchMoved:
                    break;
                case sf::Event::TouchEnded:
                    break;
                case sf::Event::SensorChanged:
                    break;
                case sf::Event::Count:
                    break;
            }
        }
    }
}

