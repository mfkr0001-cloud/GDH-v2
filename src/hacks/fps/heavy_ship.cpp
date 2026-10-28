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

        // Use a time-based smoothing coefficient so the response is consistent
        // across frame rates. Avoid correcting position with setPosition() here:
        // that extra per-frame position write can cause visible micro-stutter
        // during long straight ship sections.
        const double safeStrength = static_cast<double>(strength);
        const double tau = 1.0 + safeStrength;
        const double response = 1.0 - std::exp(-static_cast<double>(dt) * 60.0 / tau);
        if (!std::isfinite(response))
            return;

        const double updatedVelocity = m_yVelocity;
        const double filteredVelocity =
            oldVelocity + (updatedVelocity - oldVelocity) * std::clamp(response, 0.0, 1.0);
        if (!std::isfinite(filteredVelocity))
            return;

        // Keep the smoothed velocity for the next physics update; do not rewrite
        // the already-integrated position, which can make the ship appear to hitch.
        m_yVelocity = filteredVelocity;
    }
};
