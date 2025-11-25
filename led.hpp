/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/chassis.hpp"
#include "redfish_util.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"

#include <sdbusplus/asio/property.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{
/**
 * @brief Retrieves identify led group properties over dbus
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
// TODO (Gunnar): Remove IndicatorLED after enough time has passed
inline void
    getIndicatorLedState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get led groups");
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "xyz.openbmc_project.Led.Group", "Asserted",
        [asyncResp](const boost::system::error_code& ec, const bool blinking) {
            // Some systems may not have enclosure_identify_blink object so
            // proceed to get enclosure_identify state.
            if (ec == boost::system::errc::invalid_argument)
            {
                BMCWEB_LOG_DEBUG(
                    "Get identity blinking LED failed, mismatch in property type");
                messages::internalError(asyncResp->res);
                return;
            }

            // Blinking ON, no need to check enclosure_identify assert.
            if (!ec && blinking)
            {
                asyncResp->res.jsonValue["IndicatorLED"] =
                    chassis::IndicatorLED::Blinking;
                return;
            }

            sdbusplus::asio::getProperty<bool>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.LED.GroupManager",
                "/xyz/openbmc_project/led/groups/enclosure_identify",
                "xyz.openbmc_project.Led.Group", "Asserted",
                [asyncResp](const boost::system::error_code& ec2,
                            const bool ledOn) {
                    if (ec2 == boost::system::errc::invalid_argument)
                    {
                        BMCWEB_LOG_DEBUG(
                            "Get enclosure identity led failed, mismatch in property type");
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    if (ec2)
                    {
                        return;
                    }

                    if (ledOn)
                    {
                        asyncResp->res.jsonValue["IndicatorLED"] =
                            chassis::IndicatorLED::Lit;
                    }
                    else
                    {
                        asyncResp->res.jsonValue["IndicatorLED"] =
                            chassis::IndicatorLED::Off;
                    }
                });
        });
}

/**
 * @brief Sets identify led group properties
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 * @param[in] ledState  LED state passed from request
 *
 * @return None.
 */
// TODO (Gunnar): Remove IndicatorLED after enough time has passed
inline void
    setIndicatorLedState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& ledState)
{
    BMCWEB_LOG_DEBUG("Set led groups");
    bool ledOn = false;
    bool ledBlinkng = false;

    if (ledState == "Lit")
    {
        ledOn = true;
    }
    else if (ledState == "Blinking")
    {
        ledBlinkng = true;
    }
    else if (ledState != "Off")
    {
        messages::propertyValueNotInList(asyncResp->res, ledState,
                                         "IndicatorLED");
        return;
    }

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "xyz.openbmc_project.Led.Group", "Asserted", ledBlinkng,
        [asyncResp, ledOn,
         ledBlinkng](const boost::system::error_code& ec) mutable {
            if (ec)
            {
                // Some systems may not have enclosure_identify_blink object so
                // Lets set enclosure_identify state to true if Blinking is
                // true.
                if (ledBlinkng)
                {
                    ledOn = true;
                }
            }
            setDbusProperty(
                asyncResp, "IndicatorLED",
                "xyz.openbmc_project.LED.GroupManager",
                sdbusplus::message::object_path(
                    "/xyz/openbmc_project/led/groups/enclosure_identify"),
                "xyz.openbmc_project.Led.Group", "Asserted", ledBlinkng);
        });
}

/**
 * @brief Retrieves identify system led group properties over dbus
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getSystemLocationIndicatorActive(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get LocationIndicatorActive");
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "xyz.openbmc_project.Led.Group", "Asserted",
        [asyncResp](const boost::system::error_code& ec, const bool blinking) {
            // Some systems may not have enclosure_identify_blink object so
            // proceed to get enclosure_identify state.
            if (ec == boost::system::errc::invalid_argument)
            {
                BMCWEB_LOG_DEBUG(
                    "Get identity blinking LED failed, mismatch in property type");
                messages::internalError(asyncResp->res);
                return;
            }

            // Blinking ON, no need to check enclosure_identify assert.
            if (!ec && blinking)
            {
                asyncResp->res.jsonValue["LocationIndicatorActive"] = true;
                return;
            }

            sdbusplus::asio::getProperty<bool>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.LED.GroupManager",
                "/xyz/openbmc_project/led/groups/enclosure_identify",
                "xyz.openbmc_project.Led.Group", "Asserted",
                [asyncResp](const boost::system::error_code& ec2,
                            const bool ledOn) {
                    if (ec2 == boost::system::errc::invalid_argument)
                    {
                        BMCWEB_LOG_DEBUG(
                            "Get enclosure identity led failed, mismatch in property type");
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    if (ec2)
                    {
                        return;
                    }

                    asyncResp->res.jsonValue["LocationIndicatorActive"] = ledOn;
                });
        });
}

/**
 * @brief Sets identify system led group properties
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 * @param[in] ledState  LED state passed from request
 *
 * @return None.
 */
inline void setSystemLocationIndicatorActive(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const bool ledState)
{
    BMCWEB_LOG_DEBUG("Set LocationIndicatorActive");

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups/enclosure_identify_blink",
        "xyz.openbmc_project.Led.Group", "Asserted", ledState,
        [asyncResp, ledState](const boost::system::error_code& ec) {
            if (ec)
            {
                // Some systems may not have enclosure_identify_blink object so
                // lets set enclosure_identify state also if
                // enclosure_identify_blink failed
                setDbusProperty(
                    asyncResp, "LocationIndicatorActive",
                    "xyz.openbmc_project.LED.GroupManager",
                    sdbusplus::message::object_path(
                        "/xyz/openbmc_project/led/groups/enclosure_identify"),
                    "xyz.openbmc_project.Led.Group", "Asserted", ledState);
            }
        });
}

namespace drive_led_control
{

struct DiskModuleDefinition
{
    std::string_view name;
    std::string_view inventoryPath;
};

inline constexpr std::array<DiskModuleDefinition, 4> diskModules = {{
    {"m2_disk_module0",
     "/xyz/openbmc_project/inventory/system/chassis/m2_disk_module0"},
    {"m2_disk_module1",
     "/xyz/openbmc_project/inventory/system/chassis/m2_disk_module1"},
    {"ed_disk_module",
     "/xyz/openbmc_project/inventory/system/chassis/ed_disk_module"},
    {"edr_disk_module",
     "/xyz/openbmc_project/inventory/system/chassis/edr_disk_module"},
}};

inline constexpr std::array<std::string_view, 1> m2DiskModule0LedGroups = {
    "m2_drive0_led"};
inline constexpr std::array<std::string_view, 1> m2DiskModule1LedGroups = {
    "m2_drive1_led"};
inline constexpr std::array<std::string_view, 8> edDiskModuleLedGroups = {
    "drive0_led", "drive1_led", "drive2_led", "drive3_led",
    "drive4_led", "drive5_led", "drive6_led", "drive7_led"};
inline constexpr std::array<std::string_view, 6> edrDiskModuleLedGroups = {
    "drive0_led", "drive1_led", "drive2_led",
    "drive3_led", "drive4_led", "drive5_led"};

inline constexpr std::string_view driveLedUri =
    "/redfish/v1/Chassis/DriveLed";
inline constexpr const char* inventoryService =
    "xyz.openbmc_project.Inventory.Manager";
inline constexpr const char* ledGroupManagerService =
    "xyz.openbmc_project.LED.GroupManager";

using ModulePresenceMap = std::map<std::string, bool>;

inline bool isModulePresent(const ModulePresenceMap& presence,
                            std::string_view moduleName)
{
    auto it = presence.find(std::string(moduleName));
    if (it == presence.end())
    {
        return false;
    }
    return it->second;
}

inline void queryModulePresence(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::function<void(ModulePresenceMap)> callback)
{
    if (diskModules.empty())
    {
        callback({});
        return;
    }

    auto presence = std::make_shared<ModulePresenceMap>();
    for (const auto& module : diskModules)
    {
        (*presence)[std::string(module.name)] = false;
    }

    auto pendingRequests =
        std::make_shared<std::size_t>(diskModules.size());
    auto callbackHolder =
        std::make_shared<std::function<void(ModulePresenceMap)>>(
            std::move(callback));

    for (const auto& module : diskModules)
    {
        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, inventoryService,
            std::string(module.inventoryPath),
            "xyz.openbmc_project.Inventory.Item", "Present",
            [asyncResp, module, presence, pendingRequests,
             callbackHolder](const boost::system::error_code& ec,
                             const bool present) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG("Presence read failed for {}: {}",
                                     module.name, ec);
                }
                else
                {
                    (*presence)[std::string(module.name)] = present;
                }

                if (*pendingRequests == 0)
                {
                    return;
                }

                (*pendingRequests)--;
                if (*pendingRequests == 0)
                {
                    (*callbackHolder)(std::move(*presence));
                }
            });
    }
}

struct DriveLedBinding
{
    std::string moduleName;
    std::string ledGroupName;
};

inline std::vector<DriveLedBinding>
    buildDriveLedBindings(const ModulePresenceMap& presence)
{
    std::vector<DriveLedBinding> bindings;
    bindings.reserve(16);

    auto appendBindings = [&bindings](std::string_view moduleName,
                                      std::span<const std::string_view> leds) {
        for (std::string_view ledName : leds)
        {
            bindings.push_back(
                {std::string(moduleName), std::string(ledName)});
        }
    };

    if (isModulePresent(presence, "m2_disk_module0"))
    {
        appendBindings("m2_disk_module0", m2DiskModule0LedGroups);
    }
    if (isModulePresent(presence, "m2_disk_module1"))
    {
        appendBindings("m2_disk_module1", m2DiskModule1LedGroups);
    }

    const bool edrPresent = isModulePresent(presence, "edr_disk_module");
    if (edrPresent)
    {
        appendBindings("edr_disk_module", edrDiskModuleLedGroups);
    }
    else if (isModulePresent(presence, "ed_disk_module"))
    {
        appendBindings("ed_disk_module", edDiskModuleLedGroups);
    }

    return bindings;
}

inline void populateModulesArray(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const ModulePresenceMap& presence)
{
    nlohmann::json::array_t modules;
    modules.reserve(presence.size());
    for (const auto& [name, present] : presence)
    {
        modules.emplace_back(
            nlohmann::json{{"Module", name}, {"Present", present}});
    }

    asyncResp->res.jsonValue["Modules"] = std::move(modules);
}

inline void populateDriveLedStates(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::vector<DriveLedBinding>&& bindings)
{
    if (bindings.empty())
    {
        asyncResp->res.jsonValue["DriveLedGroups"] =
            nlohmann::json::array();
        return;
    }

    auto ledStates =
        std::make_shared<nlohmann::json::array_t>(bindings.size());
    auto pendingRequests =
        std::make_shared<std::size_t>(bindings.size());

    for (std::size_t index = 0; index < bindings.size(); ++index)
    {
        const DriveLedBinding binding = bindings[index];
        const std::string objectPath =
            "/xyz/openbmc_project/led/groups/" + binding.ledGroupName;

        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, ledGroupManagerService,
            objectPath, "xyz.openbmc_project.Led.Group", "Asserted",
            [asyncResp, ledStates, pendingRequests, binding,
             index](const boost::system::error_code& ec,
                    const bool asserted) {
                nlohmann::json entry;
                entry["LedGroup"] = binding.ledGroupName;
                entry["Module"] = binding.moduleName;
                entry["Asserted"] = (!ec) && asserted;

                (*ledStates)[index] = std::move(entry);

                if (--(*pendingRequests) == 0)
                {
                    asyncResp->res.jsonValue["DriveLedGroups"] =
                        std::move(*ledStates);
                }
            });
    }
}

inline void setDriveLedResourceMetadata(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#DriveLed.v1_0_0.DriveLed";
    asyncResp->res.jsonValue["@odata.id"] =
        std::string(driveLedUri);
    asyncResp->res.jsonValue["Name"] = "Disk Module LED Control";
    asyncResp->res.jsonValue["Description"] =
        "Controls disk module LED groups exposed by the LED manager.";
}

} // namespace drive_led_control

inline void handleDriveLedGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    drive_led_control::setDriveLedResourceMetadata(asyncResp);
    drive_led_control::queryModulePresence(
        asyncResp,
        [asyncResp](drive_led_control::ModulePresenceMap presence) {
            drive_led_control::populateModulesArray(asyncResp, presence);
            auto bindings =
                drive_led_control::buildDriveLedBindings(presence);
            drive_led_control::populateDriveLedStates(
                asyncResp, std::move(bindings));
        });
}

inline void handleDriveLedPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::optional<std::string> ledName;
    std::optional<bool> asserted;

    if (!json_util::readJsonPatch(req, asyncResp->res, "LedName",
                                  ledName, "Asserted", asserted))
    {
        return;
    }

    if (!ledName)
    {
        messages::propertyMissing(asyncResp->res, "LedName");
        return;
    }

    if (!asserted)
    {
        messages::propertyMissing(asyncResp->res, "Asserted");
        return;
    }

    std::string requestedLed = std::move(*ledName);
    const bool requestedState = *asserted;

    drive_led_control::queryModulePresence(
        asyncResp,
        [asyncResp, requestedLed = std::move(requestedLed),
         requestedState](drive_led_control::ModulePresenceMap presence) {
            auto bindings =
                drive_led_control::buildDriveLedBindings(presence);

            const auto it = std::find_if(
                bindings.begin(), bindings.end(),
                [&requestedLed](
                    const drive_led_control::DriveLedBinding& binding) {
                    return binding.ledGroupName == requestedLed;
                });

            if (it == bindings.end())
            {
                messages::propertyValueNotInList(
                    asyncResp->res, requestedLed, "LedName");
                return;
            }

            const std::string objectPath =
                "/xyz/openbmc_project/led/groups/" + requestedLed;

            setDbusProperty(
                asyncResp, "LedName",
                drive_led_control::ledGroupManagerService,
                sdbusplus::message::object_path(objectPath),
                "xyz.openbmc_project.Led.Group", "Asserted",
                requestedState);
        });
}

inline void requestRoutesDriveLedControl(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/DriveLed")
        .privileges(redfish::privileges::getChassis)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleDriveLedGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/DriveLed")
        .privileges(redfish::privileges::patchChassis)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleDriveLedPatch, std::ref(app)));
}
} // namespace redfish
