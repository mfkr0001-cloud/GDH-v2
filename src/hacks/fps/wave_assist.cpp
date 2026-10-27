#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include <algorithm>

using namespace geode::prelude;

// Buffers very short wave taps so they survive until the next game update.
// It does not change display refresh rate or globally scale physics.
GUI_HACK_CREATE("FPS", "Wave Response", "Buffers short wave taps for more consistent input on 60 Hz screens", false);

class $modify(GDHWaveResponsePlayerObject, PlayerObject) {
    struct Fields {
        bool waveHeld = false;
        bool releasePending = false;
        float heldTime = 0.f;
        PlayerButton pendingButton = PlayerButton::Jump;
    };

    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("FPS").findHackByName("Wave Response");
        hack.addHookPtr(self.getHook("PlayerObject::pushButton").unwrap());
        hack.addHookPtr(self.getHook("PlayerObject::releaseButton").unwrap());
        hack.addHookPtr(self.getHook("PlayerObject::update").unwrap());
    }

    bool pushButton(PlayerButton button) {
        auto& hack = GDH::Gui::get().getWindow("FPS").findHackByName("Wave Response");
        const bool targetWaveInput = hack.getEnabled() && m_gameLayer && m_isDart && button == PlayerButton::Jump;

        // If another press arrives before the buffered release is processed, close
        // the previous tap first so the next press remains a distinct input.
        if (targetWaveInput && m_fields->releasePending) {
            PlayerObject::releaseButton(m_fields->pendingButton);
            m_fields->releasePending = false;
            m_fields->waveHeld = false;
            m_fields->heldTime = 0.f;
        }

        bool result = PlayerObject::pushButton(button);
        if (targetWaveInput && result) {
            m_fields->waveHeld = true;
            m_fields->releasePending = false;
            m_fields->heldTime = 0.f;
        }
        return result;
    }

    bool releaseButton(PlayerButton button) {
        auto& hack = GDH::Gui::get().getWindow("FPS").findHackByName("Wave Response");
        const bool targetWaveInput = hack.getEnabled() && m_gameLayer && m_isDart && button == PlayerButton::Jump;
        if (targetWaveInput && m_fields->waveHeld) {
            float bufferMs = Config::get().get<float>("fps.wave_response::buffer_ms", 8.f);
            bufferMs = std::clamp(bufferMs, 0.f, 20.f);
            if (bufferMs > 0.f && m_fields->heldTime < bufferMs / 1000.f) {
                m_fields->releasePending = true;
                m_fields->pendingButton = button;
                return true;
            }
        }

        bool result = PlayerObject::releaseButton(button);
        if (targetWaveInput && result) {
            m_fields->waveHeld = false;
            m_fields->releasePending = false;
            m_fields->heldTime = 0.f;
        }
        return result;
    }

    void update(float dt) {
        if (m_gameLayer && m_isDart && m_fields->waveHeld) {
            m_fields->heldTime += std::max(0.f, dt);
            if (m_fields->releasePending) {
                auto& hack = GDH::Gui::get().getWindow("FPS").findHackByName("Wave Response");
                float bufferMs = Config::get().get<float>("fps.wave_response::buffer_ms", 8.f);
                bufferMs = std::clamp(bufferMs, 0.f, 20.f);
                if (!hack.getEnabled() || m_fields->heldTime >= bufferMs / 1000.f) {
                    PlayerObject::releaseButton(m_fields->pendingButton);
                    m_fields->waveHeld = false;
                    m_fields->releasePending = false;
                    m_fields->heldTime = 0.f;
                }
            }
        }
        PlayerObject::update(dt);
    }
};
