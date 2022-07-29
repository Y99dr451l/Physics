#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <vector>
#include <iostream>
#include<math.h>

#define PI 3.14159265358979323846f
#define SUB_STEPS 8

struct PhysicalObject : public sf::Drawable {
    PhysicalObject(float x, float y, float radius, float mass = 0.f, bool is_static = false) {
        this->position.x = x; this->position.y = y;
		this->position_old = this->position;
        this->radius = radius; this->is_static = is_static;
        this->mass = mass + (mass == 0.f)*radius*radius*PI;
        // this->elasticity = elasticity; this->friction = friction;
        this->velocity.x = 0; this->velocity.y = 0;
        this->acceleration.x = 0; this->acceleration.y = 0;
        this->angularVelocity = 0; this->angularAcceleration = 0;
    } //virtual ~PhysicalObject();
    
    float radius, mass; // elasticity, friction;
    sf::Vector2f position, position_old, velocity, acceleration;
    float rotation, angularVelocity, angularAcceleration;
    bool is_static;
    
    void applyAcceleration(float x, float y) {acceleration.x += x; acceleration.y += y;}
    void applyAcceleration(sf::Vector2f acceleration) {this->acceleration += acceleration;}
    
    void updateObject(float dt) {
		if (!is_static) {
			updatePosition(dt);
			// updateRotation(dt);
		}
}
    void updatePosition(float dt) {
		position_old = position;
		velocity += acceleration*dt;
		position += velocity*dt;
		acceleration = sf::Vector2f(0, 0);
    }
    void updateRotation(float dt) {
        angularVelocity += angularAcceleration * dt; rotation += angularVelocity * dt;
        if (rotation > 360) rotation -= 360; if (rotation < 0) rotation += 360;
        angularAcceleration = 0;
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        states.transform *= getTransform(); states.texture = NULL;
        target.draw(sf::CircleShape(radius), states);
    }
    sf::Transform getTransform() const {
        sf::Transform transform;
        return transform.translate(position - sf::Vector2f(radius, radius)).rotate(rotation);
    }
};

struct Spring {
    Spring(PhysicalObject* a, PhysicalObject* b, float spring_constant, float damping_constant) {
        this->a = a; this->b = b;
        this->spring_constant = spring_constant;
        this->damping_constant = damping_constant;
    }
    PhysicalObject* a; PhysicalObject* b;
    float spring_constant, damping_constant;
};

// struct Force {
//     Force(PhysicalObject* a, float x, float y) {
//         this->a = a;
//         this->x = x; this->y = y;
//     }
//     PhysicalObject* a;
//     float x, y;
// };

struct Scene {
    Scene(sf::Vector2f gravity = sf::Vector2f(0, 0), float air_resistance = 0.f) {
        this->gravity = gravity;
        this->air_resistance = air_resistance;
    }
    std::vector<PhysicalObject*> objects;
    std::vector<Spring*> springs;
    // std::vector<Force*> forces;
    sf::Vector2f gravity;
    float air_resistance;
    bool elastic_collisions = true;

    void applyForces() {
        for (int i = 0; i < objects.size(); i++) {
            PhysicalObject* a = objects[i];
            a->acceleration += gravity;
            if (air_resistance != 0.f) {
                a->acceleration.x += air_resistance * a->velocity.x;
                a->acceleration.y += air_resistance * a->velocity.y;
            }
        }
    }

    void applyConstraint() {
        const sf::Vector2f bposition(500, 500); const float bradius = 400;
        for (int i = 0; i < objects.size(); i++) {
            if (objects[i]->is_static) continue;
            sf::Vector2f distance = objects[i]->position - bposition;
            float distance_length = sqrt(distance.x * distance.x + distance.y * distance.y);
            if (distance_length > bradius - objects[i]->radius) {
                sf::Vector2f normal = distance / distance_length;
                objects[i]->position = bposition + normal * (bradius - objects[i]->radius);
            }
        }
    }

    void solveCollisions() {
        for (int i = 0; i < objects.size(); i++) for (int j = i+1; j < objects.size(); j++) {
                if (objects[i]->is_static && objects[j]->is_static) continue;
                float distance = sqrt(pow(objects[i]->position.x - objects[j]->position.x, 2) + pow(objects[i]->position.y - objects[j]->position.y, 2));
                if (distance < objects[i]->radius + objects[j]->radius) {
                    const sf::Vector2f normal = (objects[i]->position - objects[j]->position) / distance;
                    const sf::Vector2f delta = normal * (objects[i]->radius + objects[j]->radius - distance) * 0.5f;
                    objects[i]->position += delta; objects[j]->position -= delta;
                }
        }
    }

    void solveElasticCollisions() {
        for (int i = 0; i < objects.size(); i++) for (int j = i+1; j < objects.size(); j++) {
                if (objects[i]->is_static && objects[j]->is_static) continue;
                float sqdist = pow(objects[i]->position.x - objects[j]->position.x, 2) + pow(objects[i]->position.y - objects[j]->position.y, 2);
                if (sqdist < pow(objects[i]->radius + objects[j]->radius, 2)) {
                    const float mass_sum = objects[i]->mass + objects[j]->mass;
                    const sf::Vector2f posdiff = objects[i]->position - objects[j]->position;
                    const sf::Vector2f veldiff = objects[i]->velocity - objects[j]->velocity;
                    const float dot = posdiff.x*veldiff.x + posdiff.y*veldiff.y;
                    objects[i]->velocity -= 2*objects[j]->mass*dot*posdiff/(mass_sum*sqdist);
                    objects[j]->velocity += 2*objects[i]->mass*dot*posdiff/(mass_sum*sqdist);

                    const float distance = sqrt(sqdist);
                    const sf::Vector2f delta = posdiff/distance*(objects[i]->radius + objects[j]->radius - distance)*0.5f;
                    objects[i]->position += delta; objects[j]->position -= delta;
                }
        }
    }

    void update(float dt) {
        for (int i = 0; i < objects.size(); i++) objects[i]->updateObject(dt);
        for (int i = 0; i < springs.size(); i++) {
            sf::Vector2f distance = springs[i]->b->position - springs[i]->a->position;
            float distance_length = sqrt(distance.x * distance.x + distance.y * distance.y);
            float force = springs[i]->spring_constant * (distance_length - springs[i]->a->radius - springs[i]->b->radius);
            sf::Vector2f force_vector = distance / distance_length * force;
            springs[i]->a->applyAcceleration(force_vector);
            springs[i]->b->applyAcceleration(-force_vector);
        }
        // for (int i = 0; i < forces.size(); i++) {
        //     forces[i].a->applyAcceleration(forces[i].x, forces[i].y);
        // }
        for (int i = 0; i < objects.size(); i++)
            objects[i]->applyAcceleration(gravity + sf::Vector2f(-air_resistance*objects[i]->velocity.x, -air_resistance*objects[i]->velocity.y));
        if (elastic_collisions) solveElasticCollisions(); else solveCollisions();
        applyConstraint();
    }

    void draw(sf::RenderWindow& window) {
        for (int i = 0; i < objects.size(); i++) window.draw(*objects[i]);
    }
};

int main() {
    sf::Clock clock;
    sf::RenderWindow window(sf::VideoMode(1000, 1000), "pure chaos");
    // window.setFramerateLimit(60); // window.setMouseCursorVisible(false);
    
    sf::Font font; sf::Text text;
    font.loadFromFile("Lavinia.otf");
    text.setFont(font); text.setCharacterSize(20); text.setPosition(5, 5);
    
    Scene scene(sf::Vector2f(0, 50), 1.f);
    scene.elastic_collisions = true;
    for (int i = 0; i < 15; i++) for (int j = 0; j < 15; j++) {
		scene.objects.push_back(new PhysicalObject(250 + i*20, 250 + j*20, 9, 250));
		scene.objects.back()->velocity = sf::Vector2f(rand()%1000 - 500, rand()%1000 - 500);
	}
	scene.objects.push_back(new PhysicalObject(500, 500, 100, 1e10, true));

    bool spacepressed = false;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)) window.close();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !spacepressed) {
            scene.objects.push_back(new PhysicalObject(rand() % 1000, rand() % 1000, rand() % 100 + 10));
            spacepressed = true;
        } else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) spacepressed = false;

        float dt = clock.restart().asSeconds();
        scene.update(dt);
        text.setString("FPS: " + std::to_string(1/dt));

        window.clear();
        scene.draw(window);
        window.draw(text);
        window.display();
    }
    return 0;
}