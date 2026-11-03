#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cmath>

using namespace geode::prelude;

static void applyLayout(bool on) {
    auto pl = PlayLayer::get();
    if (!pl) return;
    for (auto obj : CCArrayExt<GameObject*>(pl->m_objects)) {
        if (obj->m_objectType == GameObjectType::Decoration) {
            obj->setVisible(!on);
        }
    }
}

static ccColor3B hueToRGB(float h) {
    float x = 1.f - std::fabs(std::fmod(h / 60.f, 2.f) - 1.f);
    float r = 0, g = 0, b = 0;
    int i = static_cast<int>(h / 60.f) % 6;
    switch (i) {
        case 0: r = 1; g = x; break;
        case 1: r = x; g = 1; break;
        case 2: g = 1; b = x; break;
        case 3: g = x; b = 1; break;
        case 4: r = x; b = 1; break;
        default: r = 1; b = x; break;
    }
    return ccc3(static_cast<GLubyte>(r * 255), static_cast<GLubyte>(g * 255), static_cast<GLubyte>(b * 255));
}

class $modify(LitePlayLayer, PlayLayer) {
    struct Fields {
        bool chaos = false;
        float t = 0.f;
        CCLayerColor* overlay = nullptr;
    };

    // Noclip: не умираем от препятствий (античит-шип оставляем как есть)
    void destroyPlayer(PlayerObject* player, GameObject* obj) override {
        if (Mod::get()->getSettingValue<bool>("noclip") && obj != m_anticheatSpike) {
            return;
        }
        PlayLayer::destroyPlayer(player, obj);
    }

    // Layout: при входе в уровень применяем сохранённое состояние
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        if (Mod::get()->getSettingValue<bool>("layout")) {
            applyLayout(true);
        }
    }

    // Хаос: цвета и тряска
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto f = m_fields.self();
        if (!f->chaos) return;

        f->t += dt;
        if (f->overlay) {
            f->overlay->setColor(hueToRGB(std::fmod(f->t * 180.f, 360.f)));
        }
        m_objectLayer->setRotation(std::sin(f->t * 2.5f) * 10.f);
        m_objectLayer->setSkewX(std::sin(f->t * 4.f) * 10.f);
    }

    void setChaos(bool on) {
        auto f = m_fields.self();
        f->chaos = on;

        if (on) {
            if (!f->overlay) {
                f->overlay = CCLayerColor::create(ccc4(255, 0, 0, 110));
                ccBlendFunc blend = {GL_SRC_ALPHA, GL_ONE};
                f->overlay->setBlendFunc(blend);
                this->addChild(f->overlay, 1000);
            }
            f->overlay->setVisible(true);
        } else {
            if (f->overlay) f->overlay->setVisible(false);
            m_objectLayer->setRotation(0.f);
            m_objectLayer->setSkewX(0.f);
        }
    }
};

class $modify(LitePauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto menu = CCMenu::create();
        menu->setPosition(ccp(0, 0));
        this->addChild(menu, 100);

        auto addRow = [&](char const* text, bool value, SEL_MenuHandler cb, float y) {
            auto toggle = CCMenuItemToggler::createWithStandardSprites(this, cb, 0.6f);
            toggle->toggle(value);
            toggle->setPosition(ccp(30.f, y));
            menu->addChild(toggle);

            auto label = CCLabelBMFont::create(text, "bigFont.fnt");
            label->setScale(0.45f);
            label->setAnchorPoint(ccp(0.f, 0.5f));
            label->setPosition(ccp(52.f, y));
            this->addChild(label, 100);
        };

        bool chaos = false;
        if (auto pl = static_cast<LitePlayLayer*>(PlayLayer::get())) {
            chaos = pl->m_fields->chaos;
        }

        addRow("Noclip", Mod::get()->getSettingValue<bool>("noclip"),
               menu_selector(LitePauseLayer::onNoclip), 70.f);
        addRow("Layout", Mod::get()->getSettingValue<bool>("layout"),
               menu_selector(LitePauseLayer::onLayout), 40.f);
        addRow("NANI?!", chaos,
               menu_selector(LitePauseLayer::onChaos), 100.f);
    }

    void onNoclip(CCObject*) {
        auto mod = Mod::get();
        mod->setSettingValue<bool>("noclip", !mod->getSettingValue<bool>("noclip"));
    }

    void onLayout(CCObject*) {
        auto mod = Mod::get();
        bool now = !mod->getSettingValue<bool>("layout");
        mod->setSettingValue<bool>("layout", now);
        applyLayout(now);
    }

    void onChaos(CCObject*) {
        if (auto pl = static_cast<LitePlayLayer*>(PlayLayer::get())) {
            pl->setChaos(!pl->m_fields->chaos);
        }
    }
};
