#pragma once
#include <Geode/Geode.hpp>
#include <chrono>

class OverlayButton : public cocos2d::CCMenu {
    geode::Function<void()> m_callback;
    cocos2d::CCSprite* m_sprite = nullptr;
    cocos2d::CCSprite* m_pauseSprite = nullptr;
    cocos2d::CCPoint m_target;
    float m_scale = 0.60f;
    bool m_moved = false;
    bool m_pressedPause = false;
    std::chrono::steady_clock::time_point m_touchStart;

    bool init(const char* file);
    void update(float dt) override;
    
    void savePos();
    float radius() const { return m_sprite->getScaledContentSize().width * .5f; }

    bool ccTouchBegan(cocos2d::CCTouch*, cocos2d::CCEvent*) override;
    void ccTouchMoved(cocos2d::CCTouch*, cocos2d::CCEvent*) override;
    void ccTouchEnded(cocos2d::CCTouch*, cocos2d::CCEvent*) override;
    void ccTouchCancelled(cocos2d::CCTouch* t, cocos2d::CCEvent* e) override { ccTouchEnded(t, e); }
    void registerWithTouchDispatcher() override;

public:
    static OverlayButton* instance;
    static OverlayButton* create(const char* file);
    static OverlayButton* get(const char* file = "GDH_buttonUI.png"_spr);
    void setCallback(geode::Function<void()>&& cb) { m_callback = std::move(cb); }
    void setVisible(bool visible) override;
    
    void setSizeScale(float scale);
    void setMinOpacity(float opacity);
    float getSizeScale() const { return m_scale; }
};
