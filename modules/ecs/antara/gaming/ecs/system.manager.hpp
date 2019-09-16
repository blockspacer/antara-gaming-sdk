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

#pragma once

#include <exception>
#include <memory>
#include <system_error>
#include <tl/expected.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <entt/signal/dispatcher.hpp>
#include "antara/gaming/ecs/base.system.hpp"
#include "antara/gaming/ecs/system.hpp"
#include "antara/gaming/ecs/system.type.hpp"
#include "antara/gaming/timer/time.step.hpp"
#include "antara/gaming/event/fatal.error.hpp"

namespace antara::gaming::ecs
{
    class system_manager
    {
    public:
        //! Public typedefs
        using system_ptr = std::unique_ptr<base_system>;
        using system_array = std::vector<system_ptr>;
        using system_registry = std::array<system_array, system_type::size>;

        //! Constructors

        /**
         * \note Basic constructor
         * \param dispatcher The dispatcher is provided to the system when it is created.
         * \param registry The entity_registry is provided to the system when it is created.
         */
        explicit system_manager(entt::registry &registry, entt::dispatcher &dispatcher) noexcept;

        /**
         * \return number of systems successfully updated
         * \note This is the function that update your systems.
         * \note Based on the logic of the different kinds of shiva systems,
         * this function take care of updating your systems in the right order.
         * \warning If you have not loaded any system into the system_manager the function return 0.
         * \warning If you decide to mark a system, it's automatically deleted at the next loop tick through this function.
         */
        std::size_t update() noexcept;

        std::size_t update_systems(system_type system_type_to_update) noexcept;

        /**
         * \note This function allow you to get a system through a template parameter.
         * \tparam TSystem represents the system to get.
         * \return A reference to the system obtained.
         */
        template<typename TSystem>
        const TSystem &get_system() const noexcept;

        /**
         * \overload get_system
         */
        template<typename TSystem>
        TSystem &get_system() noexcept;

        /**
         * \note This function allow you to get multiple system through multiple templates parameters.
         * \tparam TSystems represents a list of systems to get
         * \return Tuple of systems obtained.
         * \details This function recursively calls the get_system function
         * \see get_system
         */
        template<typename ...TSystems>
        std::tuple<std::add_lvalue_reference_t<TSystems>...> get_systems() noexcept;

        /**
         * @overload get_systems
         */
        template<typename ...TSystems>
        std::tuple<std::add_lvalue_reference_t<std::add_const_t<TSystems>>...> get_systems() const noexcept;

        /**
         * \note This function allow you to verify if a system is already registered in the system_manager.
         * \tparam TSystem Represents the system that needs to be verified
         * \return true if the system has been loaded, false otherwise
         */
        template<typename TSystem>
        [[nodiscard]] bool has_system() const noexcept;

        /**
         * \note This function allow you to verify if a list of systems is already registered in the system_manager.
         * \tparam TSystems represents a list of system that needs to be verified
         * \return true if the list of systems has been loaded, false otherwise
         * \details This function recursively calls the has_system function
         * \see has_system
         */
        template<typename ... TSystems>
        [[nodiscard]] bool has_systems() const noexcept;

        /**
        * \note This function marks a system that will be destroyed at the next tick of the game loop.
        * \tparam TSystem Represents the system that needs to be marked
        * \return true if the system has been marked, false otherwise
        */
        template<typename TSystem>
        bool mark_system() noexcept;

        /**
         * \note This function marks a list of systems, marked systems will be destroyed at the next tick of the game loop.
         * \tparam TSystems Represents a list of systems that needs to be marked
         * \return true if  the list of systems has been marked, false otherwise
         * \details This function recursively calls the mark_system function
         * \see mark_system
         */
        template<typename ... TSystems>
        bool mark_systems() noexcept;

        /**
        * \note This function enable a system
        * \tparam TSystem Represents the system that needs to be enabled.
        * \return true if the system has been enabled, false otherwise
        */
        template<typename TSystem>
        bool enable_system() noexcept;

        /**
         * \note This function enable a list of systems
         * \tparam TSystems Represents a list of systems that needs to be enabled
         * \return true if the list of systems has been enabled, false otherwise
         * \details This function recursively calls the enable_system function
         * \see enable_system
         */
        template<typename ... TSystems>
        bool enable_systems() noexcept;

        /**
         * \note This function disable a system
         * \tparam TSystem Represents the system that needs to be disabled
         * \return true if the the system has been disabled, false otherwise
         * \attention If you deactivate a system, it will not be destroyed but simply ignore during the game loop
         */
        template<typename TSystem>
        bool disable_system() noexcept;

        /**
         * \note This function disable a list of systems
         * \tparam TSystems  Represents a list of systems that needs to be disabled
         * \return true if the list of systems has been disabled, false otherwise
         * \details This function recursively calls the disable_system function
         */
        template<typename ... TSystems>
        bool disable_systems() noexcept;

        /**
         * \return number of systems
         */
        [[nodiscard]] std::size_t nb_systems() const noexcept;

        /**
         * \param sys_type represent the type of systems.
         * \return number of systems of a specific type.
         */
        [[nodiscard]] std::size_t nb_systems(system_type sys_type) const noexcept;

        /**
         * \note This function allow you to create a system with the given argument
         * \note This function is a factory
         * \tparam TSystem represents the type of system to create
         * \tparam TSystemArgs represents the arguments needed to construct the system to create
         * \return Returns a reference to the created system
         */
        template<typename TSystem, typename ... TSystemArgs>
        TSystem &create_system(TSystemArgs &&...args) noexcept;

        /**
        * \tparam TSystems represents a list of systems to be loaded
        * \return Tuple of systems loaded
        * \see create_system
        */
        template<typename ...TSystems, typename ...TArgs>
        auto load_systems(TArgs &&...args) noexcept;

    private:
        //! Private member functions
        base_system &add_system_(system_ptr &&system, system_type sys_type) noexcept;

        void sweep_systems_() noexcept;

        template<typename TSystem>
        tl::expected<std::reference_wrapper<TSystem>, std::error_code> get_system_() noexcept;

        template<typename TSystem>
        [[nodiscard]] tl::expected<std::reference_wrapper<const TSystem>, std::error_code> get_system_() const noexcept;

        //! Private data members
        using clock = std::chrono::steady_clock;
        clock::time_point start_{clock::now()};
        timer::time_step timestep_;
        [[maybe_unused]] entt::registry &entity_registry_;
        [[maybe_unused]] entt::dispatcher &dispatcher_;
        system_registry systems_{{}};
        bool need_to_sweep_systems_{false};
    };
}

//! Implementation
namespace antara::gaming::ecs
{
    template<typename TSystem>
    const TSystem &system_manager::get_system() const noexcept
    {
        const auto ret = get_system_<TSystem>().or_else([this](const std::error_code &ec) {
            this->dispatcher_.trigger<event::fatal_error>(ec);
        });
        return (*ret).get();
    }

    template<typename TSystem>
    TSystem &system_manager::get_system() noexcept
    {
        auto ret = get_system_<TSystem>().or_else([this](const std::error_code &ec) {
            this->dispatcher_.trigger<event::fatal_error>(ec);
        });
        return (*ret).get();
    }

    template<typename... TSystems>
    std::tuple<std::add_lvalue_reference_t<TSystems>...> system_manager::get_systems() noexcept
    {
        return {get_system<TSystems>()...};
    }

    template<typename... TSystems>
    std::tuple<std::add_lvalue_reference_t<std::add_const_t<TSystems>>...> system_manager::get_systems() const noexcept
    {
        return {get_system<TSystems>()...};
    }

    template<typename TSystem, typename... TSystemArgs>
    TSystem &system_manager::create_system(TSystemArgs &&... args) noexcept
    {
        if (has_system<TSystem>()) {
            return get_system<TSystem>();
        }
        auto creator = [this](auto &&... args_) {
            return std::make_unique<TSystem>(this->entity_registry_,
                                             this->dispatcher_,
                                             std::forward<decltype(args_)>(args_)...);
        };
        system_ptr sys = creator(std::forward<TSystemArgs>(args)...);
        return static_cast<TSystem &>(add_system_(std::move(sys), TSystem::get_system_type()));
    }

    template<typename... TSystems, typename... TArgs>
    auto system_manager::load_systems(TArgs &&... args) noexcept
    {
        (create_system<TSystems>(std::forward<TArgs>(args)...), ...);
        return get_systems<TSystems ...>();
    }

    template<typename TSystem>
    bool system_manager::has_system() const noexcept
    {
        constexpr const auto sys_type = TSystem::get_system_type();
        return ranges::any_of(systems_[sys_type], [](auto &&ptr) {
            return ptr->get_name() == TSystem::get_class_name();
        });
    }

    template<typename... TSystems>
    bool system_manager::has_systems() const noexcept
    {
        return (has_system<TSystems>() && ...);
    }

    template<typename TSystem>
    bool system_manager::mark_system() noexcept
    {
        if (has_system<TSystem>()) {
            get_system<TSystem>().mark();
            need_to_sweep_systems_ = true;
            return true;
        }
        need_to_sweep_systems_ = false;
        return false;
    }

    template<typename... TSystems>
    bool system_manager::mark_systems() noexcept
    {
        return (mark_system<TSystems>() && ...);
    }

    template<typename TSystem>
    bool system_manager::enable_system() noexcept
    {
        if (has_system<TSystem>()) {
            get_system<TSystem>().enable();
            return true;
        }
        return false;
    }

    template<typename... TSystems>
    bool system_manager::enable_systems() noexcept
    {
        return (enable_system<TSystems>() && ...);
    }

    template<typename TSystem>
    bool system_manager::disable_system() noexcept
    {
        if (has_system<TSystem>()) {
            get_system<TSystem>().disable();
            return true;
        }
        return false;
    }

    template<typename... TSystems>
    bool system_manager::disable_systems() noexcept
    {
        return (disable_system<TSystems>() && ...);
    }

    template<typename TSystem>
    tl::expected<std::reference_wrapper<TSystem>, std::error_code> system_manager::get_system_() noexcept
    {
        if (!nb_systems(TSystem::get_system_type())) {
            return tl::make_unexpected(std::make_error_code(std::errc::result_out_of_range)); //LCOV_EXCL_LINE
        }

        constexpr const auto sys_type = TSystem::get_system_type();
        auto it = ranges::find_if(systems_[sys_type], [](auto &&ptr) {
            return ptr->get_name() == TSystem::get_class_name();
        });

        if (it != systems_[sys_type].end()) {
            auto &system = static_cast<TSystem &>(*(*it));
            return std::reference_wrapper<TSystem>(system);
        }
        return tl::make_unexpected(std::make_error_code(std::errc::result_out_of_range)); //LCOV_EXCL_LINE
    }

    template<typename TSystem>
    tl::expected<std::reference_wrapper<const TSystem>, std::error_code> system_manager::get_system_() const noexcept
    {
        if (!nb_systems(TSystem::get_system_type())) {
            return tl::make_unexpected(std::make_error_code(std::errc::result_out_of_range)); //LCOV_EXCL_LINE
        }

        constexpr const auto sys_type = TSystem::get_system_type();
        auto it = ranges::find_if(systems_[sys_type], [](auto &&ptr) {
            return ptr->get_name() == TSystem::get_class_name();
        });
        if (it != systems_[sys_type].end()) {
            const auto &system = static_cast<const TSystem &>(*(*it));
            return std::reference_wrapper<const TSystem>(system);
        }
        return tl::make_unexpected(std::make_error_code(std::errc::result_out_of_range)); //LCOV_EXCL_LINE
    }
}