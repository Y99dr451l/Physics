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

void applyConstraint(std::vector<PhysicalObject>& objects) {
    const sf::Vector2f bposition(500, 500); const float bradius = 400;
    for (int i = 0; i < objects.size(); i++) {
        if (objects[i].is_static) continue;
        sf::Vector2f distance = objects[i].position - bposition;
        float distance_length = sqrt(distance.x * distance.x + distance.y * distance.y);
        if (distance_length > bradius - objects[i].radius) {
            sf::Vector2f normal = distance / distance_length;
            objects[i].position = bposition + normal * (bradius - objects[i].radius);
        }
    }
}

void solveCollisions(std::vector<PhysicalObject>& objects) {
    for (int i = 0; i < objects.size(); i++) for (int j = i+1; j < objects.size(); j++) {
            if (objects[i].is_static && objects[j].is_static) continue;
            float distance = sqrt(pow(objects[i].position.x - objects[j].position.x, 2) + pow(objects[i].position.y - objects[j].position.y, 2));
            if (distance < objects[i].radius + objects[j].radius) {
                const sf::Vector2f normal = (objects[i].position - objects[j].position) / distance;
                const sf::Vector2f delta = normal * (objects[i].radius + objects[j].radius - distance) * 0.5f;
                objects[i].position += delta; objects[j].position -= delta;
            }
    }
}

void solveElasticCollisions(std::vector<PhysicalObject>& objects) {
    for (int i = 0; i < objects.size(); i++) for (int j = i+1; j < objects.size(); j++) {
            if (objects[i].is_static && objects[j].is_static) continue;
            float sqdist = pow(objects[i].position.x - objects[j].position.x, 2) + pow(objects[i].position.y - objects[j].position.y, 2);
            if (sqdist < pow(objects[i].radius + objects[j].radius, 2)) {
                const float mass_sum = objects[i].mass + objects[j].mass;
                const sf::Vector2f posdiff = objects[i].position - objects[j].position;
				const sf::Vector2f veldiff = objects[i].velocity - objects[j].velocity;
				const float dot = posdiff.x*veldiff.x + posdiff.y*veldiff.y;
				objects[i].velocity -= 2*objects[j].mass*dot*posdiff/(mass_sum*sqdist);
				objects[j].velocity += 2*objects[i].mass*dot*posdiff/(mass_sum*sqdist);

				const float distance = sqrt(sqdist);
                const sf::Vector2f delta = posdiff/distance*(objects[i].radius + objects[j].radius - distance)*0.5f;
                objects[i].position += delta; objects[j].position -= delta;
            }
    }
}

int main() {
    sf::Clock clock;
    sf::RenderWindow window(sf::VideoMode(1000, 1000), "pure chaos");
    window.setFramerateLimit(60); // window.setMouseCursorVisible(false);

    std::vector<PhysicalObject> objects;
    for (int i = 0; i < 15; i++) for (int j = 0; j < 15; j++) {
		objects.push_back(PhysicalObject(250 + i*20, 250 + j*20, 9, 250));
		objects.back().velocity = sf::Vector2f(rand()%100 - 50, rand()%100 - 50);
	}
	objects.push_back(PhysicalObject(500, 500, 100, 1e10));

    bool spacepressed = false;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)) window.close();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !spacepressed) {
            objects.push_back(PhysicalObject(rand() % 1000, rand() % 1000, rand() % 100 + 10));
            spacepressed = true;
        } else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) spacepressed = false;

        float dt = clock.restart().asSeconds();
        for (int i = 0; i < SUB_STEPS; i++) {
			 for (int j = 0; j < objects.size(); j++) {
                // objects[j].applyAcceleration(0, 50);
                objects[j].updateObject(dt/SUB_STEPS);
            }
            // solveCollisions(objects);
            solveElasticCollisions(objects);
        }
        window.clear();
        for (int i = 0; i < objects.size(); i++) window.draw(objects[i]);
        window.display();
    }
    return 0;
}