#include "overlay_button.hpp"
#include "../../core/config.hpp"
#include "../../core/gui.hpp"
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/GameManager.hpp>

using namespace cocos2d;
using namespace geode::prelude;

OverlayButton* OverlayButton::instance = nullptr;

OverlayButton* OverlayButton::create(const char* file) {
    auto* ret = new OverlayButton();
    if (ret->init(file)) { ret->autorelease(); return ret; }
    delete ret;
    return nullptr;
}

OverlayButton* OverlayButton::get(const char* file) {
    if (!instance) {
        instance = create(file);
    }
    return instance;
}

bool OverlayButton::init(const char* file) {
    if (!CCMenu::init()) return false;

    auto& config = Config::get();
    m_scale = config.get<float>("ui_icon.scale", 0.5f);
    
    m_sprite = CCSprite::create(file);
    m_sprite->setScale(m_scale);

    float m_minOpacity = std::clamp(config.get<float>("ui_icon.minOpacity", 0.2f), 0.0f, 1.0f);
    m_sprite->setOpacity(m_minOpacity * 255);

    m_sprite->setPosition({
        config.get<float>("ui_icon.x", 80.f),
        config.get<float>("ui_icon.y", 80.f)
    });
    m_target = m_sprite->getPosition();
    addChild(m_sprite);

    // Lower floating button controls the existing Hide Pause Menu hack.
    m_pauseSprite = CCSprite::create(file);
    if (m_pauseSprite) {
        m_pauseSprite->setScale(m_scale * 0.82f);
        m_pauseSprite->setOpacity(static_cast<GLubyte>(m_minOpacity * 255.f));
        auto label = CCLabelBMFont::create("P", "bigFont.fnt");
        if (label) {
            label->setScale(0.55f);
            label->setPosition({
                m_pauseSprite->getContentSize().width * 0.5f,
                m_pauseSprite->getContentSize().height * 0.5f
            });
            m_pauseSprite->addChild(label);
        }
        m_pauseSprite->setPosition({
            m_target.x,
            std::max(radius(), m_target.y - radius() * 2.15f)
        });
        addChild(m_pauseSprite);
    }

    auto autoLabel = CCLabelBMFont::create("A", "bigFont.fnt");
    if (autoLabel) {
        autoLabel->setScale(0.55f);
        autoLabel->setPosition({
            m_sprite->getContentSize().width * 0.5f,
            m_sprite->getContentSize().height * 0.5f
        });
        m_sprite->addChild(autoLabel);
    }

    setZOrder(1000);
    setPosition({0, 0});
    scheduleUpdate();

    bool isVisible = config.get<bool>("ui_icon.visible", true);
    this->setVisible(isVisible);

    return true;
}

void OverlayButton::setSizeScale(float scale) {
    m_scale = std::clamp(scale, 0.1f, 5.0f);
    if (m_sprite) m_sprite->setScale(m_scale);
    if (m_pauseSprite) m_pauseSprite->setScale(m_scale * 0.82f);
    Config::get().set<float>("ui_icon.scale", m_scale);
}

void OverlayButton::setMinOpacity(float opacity) {
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    Config::get().set<float>("ui_icon.minOpacity", opacity);
    if (m_sprite) m_sprite->setOpacity(static_cast<GLubyte>(opacity * 255.0f));
    if (m_pauseSprite) m_pauseSprite->setOpacity(static_cast<GLubyte>(opacity * 255.0f));
}

void OverlayButton::setVisible(bool visible) {
    if (isVisible() == visible) return;
    
    CCMenu::setVisible(visible);

    if (auto dispatcher = CCDirector::sharedDirector()->getTouchDispatcher()) {
        if (visible) {
            dispatcher->addTargetedDelegate(this, -1000, true);
        } else {
            dispatcher->removeDelegate(this);
        }
    }

    Config::get().set<bool>("ui_icon.visible", visible);
}

void OverlayButton::update(float dt) {
    auto win_size = CCDirector::sharedDirector()->getWinSize();
    float r = radius();
    m_target.x = std::clamp(m_target.x, r, win_size.width - r);
    m_target.y = std::clamp(m_target.y, r, win_size.height - r);

    CCPoint pos = m_sprite->getPosition();
    if (ccpDistance(pos, m_target) < 0.5f) return;
    auto next = ccpLerp(pos, m_target, 12.f * dt);
    m_sprite->setPosition(next);
    if (m_pauseSprite) {
        m_pauseSprite->setPosition({
            next.x,
            std::max(r, next.y - r * 2.15f)
        });
    }
}

bool OverlayButton::ccTouchBegan(CCTouch* t, CCEvent*) {
    if (!isVisible()) return false;

    CCPoint tp = convertToNodeSpace(t->getLocation());
    float pauseRadius = m_pauseSprite ? m_pauseSprite->getScaledContentSize().width * 0.5f : 0.f;
    m_pressedPause = m_pauseSprite &&
        ccpDistance(tp, m_pauseSprite->getPosition()) <= pauseRadius;

    if (!m_pressedPause && ccpDistance(tp, m_sprite->getPosition()) > radius())
        return false;

    m_moved = false;
    m_touchStart = std::chrono::steady_clock::now();

    auto* pressed = m_pressedPause ? m_pauseSprite : m_sprite;
    pressed->stopAllActions();
    pressed->runAction(CCScaleTo::create(
        0.08f,
        (m_pressedPause ? m_scale * 0.82f : m_scale) * 0.88f
    ));
    pressed->runAction(CCFadeTo::create(0.08f, 255));
    return true;
}

void OverlayButton::ccTouchMoved(CCTouch* t, CCEvent*) {
    if (m_pressedPause) return;

    CCPoint tp = convertToNodeSpace(t->getLocation());
    if (!m_moved && ccpDistance(tp, m_sprite->getPosition()) < radius())
        return;

    m_moved = true;
    auto win_size = CCDirector::sharedDirector()->getWinSize();
    float r = m_sprite->getContentSize().width * m_scale * 0.60f;
    m_target.x = std::clamp(tp.x, r, win_size.width - r);
    m_target.y = std::clamp(tp.y, r * 2.2f, win_size.height - r);
}

void OverlayButton::ccTouchEnded(CCTouch*, CCEvent*) {
    auto* pressed = m_pressedPause ? m_pauseSprite : m_sprite;
    pressed->stopAllActions();
    pressed->runAction(CCEaseBackOut::create(CCScaleTo::create(
        0.25f, m_pressedPause ? m_scale * 0.82f : m_scale
    )));

    float opacity = std::clamp(
        Config::get().get<float>("ui_icon.minOpacity", 0.2f), 0.0f, 1.0f
    );
    pressed->runAction(CCSequence::create(
        CCDelayTime::create(0.8f),
        CCFadeTo::create(0.3f, opacity * 255.f),
        nullptr
    ));

    auto held = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - m_touchStart
    ).count();

    if (!m_moved) {
        auto& gui = GDH::Gui::get();
        if (m_pressedPause) {
            gui.getWindow("Cosmetic").findHackByName("Hide Pause Menu").toggle();
        } else if (held >= 500) {
            // Long press keeps the existing GDH menu toggle available.
            if (m_callback) m_callback();
        } else {
            gui.getWindow("Level").findHackByName("Autoclicker").toggle();
        }
    }

    m_pressedPause = false;
    m_moved = false;
    savePos();
}

void OverlayButton::savePos() {
    Config::get().set<float>("ui_icon.x", m_target.x);
    Config::get().set<float>("ui_icon.y", m_target.y);
}

void OverlayButton::registerWithTouchDispatcher() {
    CCTouchDispatcher::get()->addTargetedDelegate(this, -1000, true);
}

#ifdef GEODE_IS_MOBILE
#include "hacks_layer.hpp"
extern void ToggleUI();      

static bool inited = false;
class $modify(CocosInitMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (!inited) {
            inited = true;
            auto button = OverlayButton::get();
            button->setID("gdh.toggle.ui"_spr);
            button->setCallback([]() {
                ToggleUI();
            });
            OverlayManager::get()->addChild(button);
        }
        return true;
    }
};

class $modify(RefreshSpriteHook, GameManager) {
    void reloadAll(bool switchingModes, bool toFullscreen, bool borderless, bool fix, bool unused) {
        OverlayManager::get()->removeChildByID("gdh.toggle.ui"_spr);
        OverlayButton::instance = nullptr;
        inited = false;
        GameManager::reloadAll(switchingModes, toFullscreen, borderless, fix, unused);
    }
};
#endif
