#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include <cmath>

using namespace geode::prelude;

GUI_HACK_CREATE("FPS", "Heavy Ship", "Makes ship vertical response heavier for straight fly", false);

class $modify(HeavyShipPlayerObject, PlayerObject) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("FPS").findHackByName("Heavy Ship");
        hack.addHookPtr(self.getHook("PlayerObject::update").unwrap());
    }

    void update(float dt) {
        const bool wasShip = m_isShip;
        const float previousVelocity = m_yVelocity;

        PlayerObject::update(dt);

        if (!wasShip || !m_isShip || dt <= 0.f || dt > 0.1f)
            return;

        auto& config = Config::get();
        const float strength = std::clamp(config.get<float>("fps.heavy_ship::strength", 55.f), 0.f, 90.f);
        if (strength <= 0.f)
            return;

        // Keep the same approximate response per second at 60/90/120 Hz.
        const float responseAt60Hz = 1.f - strength / 100.f;
        const float response = 1.f - std::pow(1.f - responseAt60Hz, dt * 60.f);
        const float updatedVelocity = m_yVelocity;
        m_yVelocity = previousVelocity + (updatedVelocity - previousVelocity) * response;
    }
};
