#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <vector>
#include <iostream>
#include<math.h>

#define SUB_STEPS 8


struct PhysicalObject : public sf::Drawable {
    PhysicalObject(float x, float y, float radius, bool is_static = false) {
        this->position.x = x; this->position.y = y;
        this->radius = radius; this->is_static = is_static;
        // this->mass = mass; this->elasticity = elasticity; this->friction = friction;
        this->velocity.x = 0; this->velocity.y = 0;
        this->acceleration.x = 0; this->acceleration.y = 0;
        this->angularVelocity = 0; this->angularAcceleration = 0;
    } //virtual ~PhysicalObject();
    
    float radius; //width, height; // mass, elasticity, friction;
    sf::Vector2f position, velocity, acceleration;
    float rotation, angularVelocity, angularAcceleration;
    bool is_static;
    
    void applyAcceleration(float x, float y) {acceleration.x += x; acceleration.y += y;}
    void applyAcceleration(sf::Vector2f acceleration) {this->acceleration += acceleration;}
    void applyImpulse(float x, float y) {velocity.x += x; velocity.y += y;}
    void applyImpulse(sf::Vector2f impulse) {velocity += impulse;}
    void applyAngularAcceleration(float acceleration) {angularAcceleration += acceleration;}
    void applyAngularForce(float force) {angularVelocity += force;}
    void applyAngularImpulse(float impulse) {angularVelocity += impulse;}

    void updateObject(float dt) {if (!is_static) {updatePosition(dt); updateRotation(dt);}}
    void updatePosition(float dt) {
        velocity += acceleration * dt; position += velocity * dt;
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
    const sf::Vector2f center(500, 500); const float radius = 400;
    for (int i = 0; i < objects.size(); i++) {
        if (objects[i].is_static) continue;
        sf::Vector2f distance = objects[i].position - center;
        float distance_length = sqrt(distance.x * distance.x + distance.y * distance.y);
        if (distance_length > radius - objects[i].radius) {
            sf::Vector2f normal = distance / distance_length;
            objects[i].position = center + normal * (radius - objects[i].radius);
        }
    }
}

void solveCollisions(std::vector<PhysicalObject>& objects) {
    for (int i = 0; i < objects.size(); i++) for (int j = i+1; j < objects.size(); j++) {
            if (objects[i].is_static && objects[j].is_static) continue;
            float distance = sqrt(pow(objects[i].position.x - objects[j].position.x, 2) + pow(objects[i].position.y - objects[j].position.y, 2));
            if (distance < objects[i].radius + objects[j].radius) {
                sf::Vector2f normal = (objects[i].position - objects[j].position) / distance;
                sf::Vector2f delta = normal * (objects[i].radius + objects[j].radius - distance) * 0.5f;
                objects[i].position += delta; objects[j].position -= delta;
            }
    }
}

int main() {
    sf::Clock clock;
    sf::RenderWindow window(sf::VideoMode(1000, 1000), "pure chaos");
    //window.setFramerateLimit(FPS); // window.setMouseCursorVisible(false);
    std::vector<PhysicalObject> objects;
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) objects.push_back(PhysicalObject(250 + i*20, 250 + j*20, 9));
    sf::CircleShape boundary(400, 500); boundary.setPosition(100, 100); boundary.setFillColor(sf::Color(100, 100, 100));
    bool spacepressed = false;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)) window.close();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !spacepressed) {
            objects.push_back(PhysicalObject(rand() % 1000, rand() % 1000, rand() % 100 + 10, false));
            spacepressed = true;
        } else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) spacepressed = false;

       float dt = clock.restart().asSeconds();
        for (int i = 0; i < SUB_STEPS; i++) {
            for (int j = 0; j < objects.size(); j++) {
                objects[j].applyAcceleration(0, 100);
                objects[j].updateObject(dt/SUB_STEPS);
            }
            applyConstraint(objects); solveCollisions(objects);
        }
        window.clear();
        window.draw(boundary);
        for (int i = 0; i < objects.size(); i++) window.draw(objects[i]);
        window.display();
    }
    return 0;
}

// const sf::Vector2f v1 = object1->velocity; const sf::Vector2f v2 = object2->velocity;
// const float v1n = v1.x*n.x + v1.y*n.y; const float v2n = v2.x*n.x + v2.y*n.y;
// const float m1 = object1->mass; const float m2 = object2->mass;
// const float m1m2 = m1 + m2;
// const float v1n_new = (m1*v1n + m2*v2n)/m1m2; const float v2n_new = (m2*v2n + m1*v1n)/m1m2;
// object1->velocity = v1n_new*n; object2->velocity = v2n_new*n;