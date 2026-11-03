#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(LitePlayLayer, PlayLayer) {
    // Noclip: не умираем от препятствий (античит-шип оставляем как есть)
    void destroyPlayer(PlayerObject* player, GameObject* obj) override {
        if (Mod::get()->getSettingValue<bool>("noclip") && obj != m_anticheatSpike) {
            return;
        }
        PlayLayer::destroyPlayer(player, obj);
    }

    // Layout: прячем декорации, остаются только блоки и шипы
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        if (!Mod::get()->getSettingValue<bool>("layout")) return;

        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (obj->m_objectType == GameObjectType::Decoration) {
                obj->setVisible(false);
            }
        }
    }
};
