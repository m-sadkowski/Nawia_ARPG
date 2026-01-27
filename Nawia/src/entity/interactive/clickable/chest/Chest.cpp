#include "Chest.h"
#include <iostream>
#include <InteractiveClickable.h>

#include "Collider.h"

namespace Nawia::Entity {

    Chest::Chest(const std::string& name, float x, float y, const std::shared_ptr<Texture2D>& texture)
        : InteractiveClickable(name, x, y, texture, 1) // 1 HP
    {
        _type = EntityType::Chest;
        setFaction(Faction::None);
        _use_3d_rendering = false;
        setCollider(std::make_unique<RectangleCollider>(this, 0.9f, 0.4f, -0.5f, -0.5f));

        _inventory = std::make_unique<Item::Backpack>(CHEST_INV_SIZE);
    }
    
    void Chest::onInteract(Entity& instigator) {
        if (_isOpen) {
            std::cout << "Skrzynia jest juz otwarta." << std::endl;
            return;
        }

        std::cout << instigator.getName() << " otwiera skrzynie " << _name << "!" << std::endl;

        // Jeœli masz animacjê otwierania:
        // playAnimation("open_chest", false);

        _isOpen = true;

        // Tutaj logika wypadania przedmiotów:
        // instigator.addItem(this->getLoot());
    }

    void Chest::update(float delta_time) {
        Entity::update(delta_time);
        // Tutaj ewentualna logika sprawdzania czy gracz jest w zasiêgu wzroku
    }

    void Chest::render(float offset_x, float offset_y) {
        // Renderuj grafikê bazow¹
        Entity::render(offset_x, offset_y);

        // Debug: Jeœli chcesz widzieæ, ¿e to obiekt interaktywny
        if (!_isOpen) {
            // Mo¿na dodaæ ikonkê wykrzyknika nad skrzyni¹
        }
    }

    float Chest::getInteractionRange()
    {
        return 2.5f * 2.5f;
    }

} // namespace Nawia::Entity