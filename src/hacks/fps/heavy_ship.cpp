#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

GUI_HACK_CREATE("FPS", "Heavy Ship", "Adds adjustable inertia to ship vertical movement", false);

class $modify(GDHHeavyShipPlayerObject, PlayerObject) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("FPS").findHackByName("Heavy Ship");
        hack.addHookPtr(self.getHook("PlayerObject::update").unwrap());
        hack.setCheating(true);
    }

    void update(float dt) {
        const bool wasShip = m_isShip;
        const double oldVelocity = m_yVelocity;

        PlayerObject::update(dt);

        if (!wasShip || !m_isShip || !m_gameLayer || !std::isfinite(dt) || dt <= 0.0f || dt > 0.1f)
            return;

        auto& gui = GDH::Gui::get();
        if (!gui.getWindow("FPS").findHackByName("Heavy Ship").getEnabled())
            return;

        float strength = Config::get().get<float>("fps.heavy_ship::strength", 4.f);
        if (!std::isfinite(strength) || strength <= 0.f)
            return;

        // 0 means no inertia. Any positive finite value is accepted; the response factor
        // asymptotically approaches zero rather than overflowing for large values.
        const double response = 1.0 / (1.0 + static_cast<double>(strength));
        const double updatedVelocity = m_yVelocity;
        const double filteredVelocity = oldVelocity + (updatedVelocity - oldVelocity) * response;
        if (!std::isfinite(filteredVelocity))
            return;

        // Correct the displacement already performed by the original update so
        // position and velocity remain consistent for this frame.
        const double correctedY = m_position.y + (filteredVelocity - updatedVelocity) * static_cast<double>(dt);
        if (!std::isfinite(correctedY))
            return;

        m_yVelocity = filteredVelocity;
        m_position.y = correctedY;
        setPosition(m_position);
    }
};
