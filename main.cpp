#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <vector>
#include <iostream>
#include<math.h>

#define PI 3.14159265358979323846f
#define SUB_STEPS 8

const std::vector<sf::Color> colors = {sf::Color::White, sf::Color::Red, sf::Color::Green, sf::Color::Blue};
enum {CIRCLE, BOX, POLYGON};

struct PObject {
    PObject(float x, float y, float mass = 1.f, bool is_static = false, int8_t group = 0) {
        this->position.x = x; this->position.y = y;
		this->position_old = this->position;
        this->is_static = is_static;
        this->mass = mass;
        this->group = rand()%4;
        this->velocity.x = 0; this->velocity.y = 0;
        this->acceleration.x = 0; this->acceleration.y = 0;
        this->angularVelocity = 0; this->angularAcceleration = 0;
    } // virtual ~PhysicalObject();
    
    sf::Vector2f position, position_old, velocity, acceleration;
    float mass, rotation, angularVelocity, angularAcceleration;
    bool is_static;
    int8_t group;

    virtual int getType() = 0;
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {}
    void applyAcceleration(float x, float y) {acceleration.x += x; acceleration.y += y;}
    void applyAcceleration(sf::Vector2f acceleration) {this->acceleration += acceleration;}
    
    void updateObject(float dt) {
		if (!is_static) {
			updatePosition(dt);
			// updateRotation(dt);
		}
    }
    void updatePosition(float dt) {
        if (is_static) return;
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
};

struct PCircle : public PObject {
    PCircle (float x, float y, float radius, float mass = 1.f, bool is_static = false, int8_t group = 0) :
    PObject(x, y, mass, is_static, group) {this->radius = radius;}
    float radius;
    int getType() {return CIRCLE;}
    void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        sf::CircleShape circle(radius);
        circle.setOrigin(radius, radius);
        circle.setPosition(position.x, position.y);
        circle.setFillColor(colors[group]);
        target.draw(circle, states);
    }
};

struct PBox : public PObject {
    PBox(float x, float y, float width, float height, float mass = 0.f, bool is_static = false, int8_t group = 0) :
    PObject(x, y, mass, is_static, group) {this->width = width; this->height = height;}
    float width, height;
    int getType() {return BOX;}
    void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        sf::RectangleShape box(sf::Vector2f(width, height));
        box.setPosition(position.x, position.y);
        box.setFillColor(colors[group]);
        target.draw(box);
    }
};

// struct PPolygon : public PObject {
//     PPolygon(float x, float y, float radius, int sides, float mass = 0.f, bool is_static = false, int8_t group = 0) :
//     PObject(x, y, mass, is_static, group) {
//         this->radius = radius;
//         this->sides = sides;
//         this->angle = 2*PI/sides;
//         this->vertices.resize(sides);
//         for (int i = 0; i < sides; i++) {
//             vertices[i].x = radius * cos(i*angle);
//             vertices[i].y = radius * sin(i*angle);
//         }
//     }
//     float radius, angle;
//     int sides;
//     std::vector<sf::Vector2f> vertices;
//     void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
//         sf::ConvexShape polygon(sides);
//         polygon.setPosition(position.x, position.y);
//         polygon.setFillColor(colors[group]);
//         polygon.setPoint(0, vertices[0]);
//         for (int i = 1; i < sides; i++) polygon.setPoint(i, vertices[i]);
//         target.draw(polygon);
//     }
// };

struct Spring : sf::Drawable {
    Spring(PObject* a, PObject* b, float spring_constant, float damping_constant) {
        this->a = a; this->b = b;
        this->spring_constant = spring_constant;
        this->damping_constant = damping_constant;
    }
    PObject* a; PObject* b;
    float spring_constant, damping_constant;

    void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) const {
        sf::VertexArray line(sf::Lines, 2);
        line[0].position = a->position; line[1].position = b->position;
        line[0].color = colors[a->group]; line[1].color = colors[b->group];
        target.draw(line, states);
    }
    
    void updateSpring(float dt) {
        sf::Vector2f delta = b->position - a->position;
        float distance = sqrt(delta.x*delta.x + delta.y*delta.y);
        float force = spring_constant*distance;
        sf::Vector2f force_vector = force*delta/distance;
        a->applyAcceleration(-force_vector/a->mass);
        b->applyAcceleration(force_vector/b->mass);
    }
};

struct Scene {
    Scene(sf::Vector2f gravity = sf::Vector2f(0, 0), float air_resistance = 0.f, bool elastic_collisions = true) {
        this->gravity = gravity;
        this->air_resistance = air_resistance;
    }
    std::vector<PObject*> objects;
    std::vector<Spring*> springs;
    sf::Vector2f gravity;
    float air_resistance;
    
    void CollisionHandler() {
        for (int i = 0; i < objects.size(); i++) for (int j = i+1; j < objects.size(); j++) {
                // if (objects[i]->group != objects[j]->group) {
                    if (objects[i]->getType() == CIRCLE && objects[j]->getType() == CIRCLE) CCTest((PCircle*)objects[i], (PCircle*)objects[j]);
                    // else if (objects[i]->getType() == CIRCLE && objects[j]->getType() == BOX) CBTest((PCircle*)objects[i], (PBox*)objects[j]);
                    // else if (objects[i]->getType() == BOX && objects[j]->getType() == CIRCLE) CBTest((PCircle*)objects[j], (PBox*)objects[i]);
                    // else if (objects[i]->getType() == BOX && objects[j]->getType() == BOX) BBTest((PBox*)objects[i], (PBox*)objects[j]);
                // }
        }
    }

    void CCTest(PCircle* a, PCircle* b) {
        if (a->is_static && b->is_static) return;
        float sqdist = pow(a->position.x - b->position.x, 2) + pow(a->position.y - b->position.y, 2);
        if (sqdist < pow(a->radius + b->radius, 2)) elasticCollision(a, b, sqdist);
    }

    void elasticCollision(PCircle* a, PCircle* b, float& sqdist) {
        const sf::Vector2f posdiff = a->position - b->position, veldiff = a->velocity - b->velocity;
        const float mass_sum = a->mass + b->mass, distance = sqrt(sqdist), dot = posdiff.x*veldiff.x + posdiff.y*veldiff.y;
        const sf::Vector2f delta = posdiff/distance*(a->radius + b->radius - distance)*0.5f;
        a->velocity -= 2*b->mass*dot*posdiff/(mass_sum*sqdist); b->velocity += 2*a->mass*dot*posdiff/(mass_sum*sqdist);
        a->position += delta; b->position -= delta;
    }

    void update(float dt) {
        for (int i = 0; i < objects.size(); i++) objects[i]->updateObject(dt);
        for (int i = 0; i < springs.size(); i++) springs[i]->updateSpring(dt);
        for (int i = 0; i < objects.size(); i++) objects[i]->applyAcceleration(gravity - air_resistance*objects[i]->velocity);
        CollisionHandler();
    }

    void draw(sf::RenderWindow& window) {
        for (int i = 0; i < objects.size(); i++) objects[i]->draw(window, sf::RenderStates::Default); // window.draw(*objects[i], sf::RenderStates::Default);
        for (int i = 0; i < springs.size(); i++) window.draw(*springs[i]); //springs[i]->draw(&window);
    }
};

int main() {
    sf::Clock clock;
    sf::ContextSettings settings; settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(1000, 1000), "pure chaos", sf::Style::None, settings);
    // window.setFramerateLimit(60); // window.setMouseCursorVisible(false);
    
    sf::Font font; sf::Text text;
    font.loadFromFile("Lavinia.otf");
    text.setFont(font); text.setCharacterSize(20); text.setPosition(5, 5);
    
    Scene scene(sf::Vector2f(0, 50), 1.f);
    for (int i = 0; i < 15; i++) for (int j = 0; j < 15; j++) {
		scene.objects.push_back(new PCircle(250 + i*30, 250 + j*30, 9, 250));
		scene.objects.back()->velocity = sf::Vector2f(rand()%100 - 50, rand()%100 - 50);
	}
    // scene.objects.push_back(new PCircle(500.f, 10000.f, 10200.f, 10000.f, true));

    for (int i = 0; i < 224; i++) if (i%15 != 14) scene.springs.push_back(new Spring(scene.objects[i], scene.objects[i+1], 10, 0.1f));
    for (int i = 0; i < 210; i++) scene.springs.push_back(new Spring(scene.objects[i], scene.objects[i+15], 10, 0.1f));

    bool spacepressed = false;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)) window.close();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !spacepressed) {
            scene.objects.push_back(new PCircle(rand() % 1000, rand() % 1000, rand() % 100 + 10));
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