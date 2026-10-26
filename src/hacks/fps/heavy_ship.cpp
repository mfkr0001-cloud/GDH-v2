#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

GUI_HACK_CREATE("FPS", "Heavy Ship", "Adds inertia to ship vertical movement for easier straight fly", false);

class $modify(HeavyShipPlayerObject, PlayerObject) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("FPS").findHackByName("Heavy Ship");
        hack.addHookPtr(self.getHook("PlayerObject::update").unwrap());
        hack.setCheating(true);
    }

    void update(float dt) {
        const bool wasShip = m_isShip;
        const float velocityBeforeUpdate = m_yVelocity;

        PlayerObject::update(dt);

        if (!wasShip || !m_isShip || dt <= 0.f || dt > 0.1f)
            return;

        auto& config = Config::get();
        if (!config.get<bool>("fps.heavy_ship", false))
            return;

        const float strength = std::clamp(config.get<float>("fps.heavy_ship::strength", 55.f), 0.f, 90.f);
        if (strength <= 0.f)
            return;

        // Retain part of the previous vertical velocity. Higher strength means
        // more inertia. Exponential scaling keeps the feel roughly consistent
        // across different update rates.
        const float retained = std::pow(strength / 100.f, dt * 60.f);
        const float targetVelocity = m_yVelocity;
        const float effectiveVelocity = velocityBeforeUpdate * retained + targetVelocity * (1.f - retained);

        // PlayerObject::update already moved the ship using targetVelocity.
        // Correct that frame's vertical displacement as well as the velocity,
        // otherwise the next update can overwrite the change before it is seen.
        m_position.y += (effectiveVelocity - targetVelocity) * dt;
        m_yVelocity = effectiveVelocity;
        setPosition(m_position);
    }
};
