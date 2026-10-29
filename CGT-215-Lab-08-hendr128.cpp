#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFPhysics.h>
#include <vector>
#include <SFML/Audio.hpp>

using namespace std;
using namespace sf;
using namespace sfp;

const float KB_SPEED = 0.2;

// Helper to load a texture
void LoadTex(Texture& tex, string filename) {
    if (!tex.loadFromFile(filename)) {
        cout << "Could not load " << filename << endl;
    }
}

// Crossbow moving player input
void MoveCrossbow(PhysicsSprite& crossbow, int elapsedMS) {
    if (Keyboard::isKeyPressed(Keyboard::Right)) {
        Vector2f newPos(crossbow.getCenter());
        newPos.x = newPos.x + (KB_SPEED * elapsedMS);
        crossbow.setCenter(newPos);
    }
    if (Keyboard::isKeyPressed(Keyboard::Left)) {
        Vector2f newPos(crossbow.getCenter());
        newPos.x = newPos.x - (KB_SPEED * elapsedMS);
        crossbow.setCenter(newPos);
    }
}

// Get the size of a given text
Vector2f GetTextSize(Text text) {
    FloatRect r = text.getGlobalBounds();
    return Vector2f(r.width, r.height);
}

int main()
{
    // Create the window
    RenderWindow window(VideoMode(800, 600), "Duck Hunter");
    World world(Vector2f(0, 0));
    int score(0);
    int arrows(5);

    // Load and create the hit sound effect
    SoundBuffer popBuffer;
    if (!popBuffer.loadFromFile("duckquack.ogg")) {
        cout << "coul not load duckquack.ogg" << endl;
        exit(5);
    }
    Sound popSound;
    popSound.setBuffer(popBuffer);

    // Create the crossbow
    PhysicsSprite& crossBow = *new PhysicsSprite();
    Texture cbowTex;
    LoadTex(cbowTex, "images/crossbow.png");
    crossBow.setTexture(cbowTex);
    Vector2f sz = crossBow.getSize();
    crossBow.setCenter(Vector2f(400, 600 - (sz.y / 2)));

    // Only use one arrow (instead of deleting) to prevent race conditions
    // when shooting an arrow immediately after one is deleted
    PhysicsSprite arrow;
    Texture arrowTex;
    LoadTex(arrowTex, "images/arrow.png");
    arrow.setTexture(arrowTex);
    arrow.setCenter(Vector2f(400, 700));
    arrow.setVelocity(Vector2f(0, 0));
    world.AddPhysicsBody(arrow);
    bool drawingArrow(false);

    // Create the top wall
    PhysicsRectangle top;
    top.setSize(Vector2f(800, 10));
    top.setCenter(Vector2f(400, 5));
    top.setStatic(true);
    world.AddPhysicsBody(top);

    // Create the right wall
    PhysicsRectangle right;
    right.setSize(Vector2f(10, 600));
    right.setCenter(Vector2f(795, 300));
    right.setStatic(true);
    world.AddPhysicsBody(right);

    // Load the duck texture and make the PhysicsShapeList
    Texture duckTex;
    LoadTex(duckTex, "images/duck.png");
    PhysicsShapeList<PhysicsSprite> ducks;

    // Collision with the top wall removes an arrow
    top.onCollision = [&drawingArrow, &world, &arrow]
    (PhysicsBodyCollisionResult result) {
        if (drawingArrow && result.object2 == arrow)
        {
            drawingArrow = false;
            arrow.setVelocity(Vector2f(0, 0));
            arrow.setCenter(Vector2f(400, 700));
        }
    };

    // Load the font for the text boxes
    Font fnt;
    if (!fnt.loadFromFile("arial.ttf")) {
        cout << "Could not load font." << endl;
        exit(3);
    }

    // Game time variables
    Clock clock;
    Time lastTime(clock.getElapsedTime());
    Time currentTime(lastTime);
    long duckMS(0);

    // Game loop
    bool gameOver = false;
    while (true) {
        while ((arrows > 0) || drawingArrow) {
            // Update game times
            currentTime = clock.getElapsedTime();
            Time deltaTime = currentTime - lastTime;
            long deltaMS = deltaTime.asMilliseconds();

            // Update
            if (deltaMS > 9) {
                lastTime = currentTime;
                duckMS += deltaMS;
                world.UpdatePhysics(deltaMS);

                MoveCrossbow(crossBow, deltaMS);

                // Either shoot an arrow or skip based on game state
                if (gameOver) {
                    gameOver = false;
                }
                else if (!drawingArrow && Keyboard::isKeyPressed(Keyboard::Space)) {
                    drawingArrow = true;
                    arrows = arrows - 1;
                    arrow.setCenter(crossBow.getCenter());
                    arrow.setVelocity(Vector2f(0, -1));
                }

                // Draw the crossbow
                window.clear();
                window.draw(crossBow);

                // Clear any ducks and draw them
                ducks.DoRemovals();
                for (PhysicsShape& duck : ducks) {
                    window.draw((PhysicsSprite&)duck);
                }

                // Draw the arrow count text
                Text arrowCountText;
                arrowCountText.setString(to_string(arrows));
                arrowCountText.setFont(fnt);
                arrowCountText.setPosition(Vector2f(10, 580 - GetTextSize(arrowCountText).y));
                window.draw(arrowCountText);

                // Draw the score text
                Text scoreText;
                scoreText.setString(to_string(score));
                scoreText.setFont(fnt);
                scoreText.setPosition(Vector2f(775 - GetTextSize(arrowCountText).x, 580 - GetTextSize(arrowCountText).y));
                window.draw(scoreText);

                // Draw the arrow
                if (drawingArrow) {
                    window.draw(arrow);
                }

                window.display();
            } // if (deltaMS > 9) {

            // Duck spawning
            if (duckMS > 2000)
            {
                duckMS = 0;

                // Set the duck texture
                PhysicsSprite& duck = ducks.Create();
                duck.setTexture(duckTex);

                // Create the duck
                int x = -50;
                Vector2f sz = duck.getSize();
                duck.setCenter(Vector2f(x, 20 + (sz.y / 2)));
                duck.setVelocity(Vector2f(0.25, 0));
                world.AddPhysicsBody(duck);

                // Duck collision behavior
                duck.onCollision =
                    [&drawingArrow, &world, &arrow, &duck, &ducks, &score, &popSound, &right]
                    (PhysicsBodyCollisionResult result) {
                    if (drawingArrow && result.object2 == arrow) {
                        popSound.play();
                        world.RemovePhysicsBody(duck);
                        ducks.QueueRemove(duck);
                        score += 10;
                        drawingArrow = false;
                        arrow.setVelocity(Vector2f(0, 0));
                        arrow.setCenter(Vector2f(400, 700));
                    }
                    else if (result.object2 == right)
                    {
                        world.RemovePhysicsBody(duck);
                        ducks.QueueRemove(duck);
                    }
                };
            } // if (duckMS > 2000)
        } // while ((arrows > 0) || drawingArrow)

        // Draw the game over text
        if (!gameOver) {
            gameOver = true;
            Text gameOverText;
            gameOverText.setString("GAME OVER");
            gameOverText.setFont(fnt);
            sz = GetTextSize(gameOverText);
            gameOverText.setPosition(400 - (sz.x / 2), 300 - (sz.y / 2));
            window.draw(gameOverText);
            window.display();
        }

        // Restart game
        if (Keyboard::isKeyPressed(Keyboard::Space)) {
            // Reset game variables
            arrows = 5;
            score = 0;

            // Clear the ducks from the window
            for (auto& duck : ducks) {
                world.RemovePhysicsBody(duck);
                ducks.QueueRemove(duck);
            }
            ducks.DoRemovals();

            // Sleep so the crossbow doesn't shoot immediately when you play again
            while (Keyboard::isKeyPressed(Keyboard::Space)) {
                sleep(milliseconds(50));
            }
        }
    } // while (true)
}