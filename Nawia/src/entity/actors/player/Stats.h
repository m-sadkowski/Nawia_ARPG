#pragma once

namespace Nawia::Entity { 

    struct Stats {
        int max_hp = 0;
        int damage = 0;
        int power = 0;
        float attack_speed = 0.0f;
        float movement_speed = 0.0f;
        int tenacity = 0;  
        int armor = 0;
        
        Stats operator+(const Stats& other) const {
            Stats result;
            result.max_hp = this->max_hp + other.max_hp;
            result.damage = this->damage + other.damage;
            result.attack_speed = this->attack_speed + other.attack_speed;
            result.movement_speed = this->movement_speed + other.movement_speed;
            result.power = this->power + other.power;
            result.tenacity = this->tenacity + other.tenacity;
            return result;
        }

        Stats& operator+=(const Stats& other) {
            this->max_hp += other.max_hp;
            this->damage += other.damage;
            this->attack_speed += other.attack_speed;
            this->movement_speed += other.movement_speed;
            this->power = this->power + other.power;
            this->tenacity += other.tenacity;
            return *this;
        }
    };

} 
