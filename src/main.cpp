#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cmath>

using namespace geode::prelude;

static bool opt(char const* key) {
    return Mod::get()->getSettingValue<bool>(key);
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
        float t = 0.f;
        CCLayerColor* overlay = nullptr;
        bool chaosOn = false;
        bool layoutOn = false;
        bool hidOn = false;
    };

    // Noclip
    void destroyPlayer(PlayerObject* player, GameObject* obj) override {
        if (opt("noclip") && obj != m_anticheatSpike) {
            return;
        }
        PlayLayer::destroyPlayer(player, obj);
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto f = m_fields.self();
        auto mod = Mod::get();
        auto win = CCDirector::get()->getWinSize();

        // Speedhack
        float sp = opt("speed-on") ? static_cast<float>(mod->getSettingValue<double>("speed")) : 1.f;
        CCDirector::get()->getScheduler()->setTimeScale(sp);

        // Mirror / Flip
        bool mx = opt("mirror");
        bool fy = opt("flip");
        this->setScaleX(mx ? -1.f : 1.f);
        this->setPositionX(mx ? win.width : 0.f);
        this->setScaleY(fy ? -1.f : 1.f);
        this->setPositionY(fy ? win.height : 0.f);

        // Hide player
        if (opt("hide-player")) {
            m_player1->setVisible(false);
            f->hidOn = true;
        } else if (f->hidOn) {
            m_player1->setVisible(true);
            f->hidOn = false;
        }

        // Layout (применяется при изменении)
        bool layout = opt("layout");
        if (layout != f->layoutOn) {
            f->layoutOn = layout;
            for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
                if (obj->m_objectType == GameObjectType::Decoration) {
                    obj->setVisible(!layout);
                }
            }
        }

        // NANI?! хаос
        if (opt("chaos")) {
            if (!f->overlay) {
                f->overlay = CCLayerColor::create(ccc4(255, 0, 0, 110));
                ccBlendFunc blend = {GL_SRC_ALPHA, GL_ONE};
                f->overlay->setBlendFunc(blend);
                this->addChild(f->overlay, 1000);
            }
            f->overlay->setVisible(true);
            f->t += dt;
            f->overlay->setColor(hueToRGB(std::fmod(f->t * 180.f, 360.f)));
            m_objectLayer->setRotation(std::sin(f->t * 2.5f) * 10.f);
            m_objectLayer->setSkewX(std::sin(f->t * 4.f) * 10.f);
            f->chaosOn = true;
        } else if (f->chaosOn) {
            if (f->overlay) f->overlay->setVisible(false);
            m_objectLayer->setRotation(0.f);
            m_objectLayer->setSkewX(0.f);
            f->chaosOn = false;
        }
    }

    void onQuit() {
        CCDirector::get()->getScheduler()->setTimeScale(1.f);
        PlayLayer::onQuit();
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

        addRow("Noclip", opt("noclip"), menu_selector(LitePauseLayer::onNoclip), 70.f);
        addRow("Layout", opt("layout"), menu_selector(LitePauseLayer::onLayout), 40.f);
        addRow("NANI?!", opt("chaos"), menu_selector(LitePauseLayer::onChaos), 100.f);

        auto spr = ButtonSprite::create("Cheats");
        spr->setScale(0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, nullptr, this, menu_selector(LitePauseLayer::onCheats));
        btn->setPosition(ccp(70.f, 140.f));
        menu->addChild(btn);
    }

    void onNoclip(CCObject*) {
        Mod::get()->setSettingValue<bool>("noclip", !opt("noclip"));
    }

    void onLayout(CCObject*) {
        Mod::get()->setSettingValue<bool>("layout", !opt("layout"));
    }

    void onChaos(CCObject*) {
        Mod::get()->setSettingValue<bool>("chaos", !opt("chaos"));
    }

    void onCheats(CCObject*) {
        geode::openSettingsPopup(Mod::get());
    }
};
