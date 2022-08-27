#include <iostream>
#include <vector>
#include <cmath>
#include <functional>
#include <algorithm>

#include <SDL2/SDL.h>

class Vec3
{
    public:
        float x;
        float y;
        float z;

        Vec3() : Vec3(0, 0, 0){};
        Vec3(float x, float y, float z)
        {
            this->x = x;
            this->y = y;
            this->z = z;
        };
};

class Vec4
{
    public:
        float x;
        float y;
        float z;
        float a;

        Vec4() : Vec4(0, 0, 0, 0){};
        Vec4(float x, float y, float z, float a)
        {
            this->x = x;
            this->y = y;
            this->z = z;
            this->a = a;
        };
};

class CameraTransform
{
    private:
        static constexpr float posStep = 2;
        static constexpr float rotStep = 2 * M_PI / 180;

        Vec3 camPos;
        float rotX;
        float cosCamRotX;
        float sinCamRotX;
        float rotY;
        float cosCamRotY;
        float sinCamRotY;
        float fov;
        float inverseTanCamFovOver2;
    public:
        int width;
        int height;

        CameraTransform() : CameraTransform(Vec3(0, 30, -60), -30 * M_PI / 180, 0, M_PI_2, 400, 400){};
        CameraTransform(Vec3 camPos, float rotX, float rotY, float fov, float width, float height)
        {
            this->camPos = camPos;
            this->rotX = rotX;
            this->cosCamRotX = cosf(rotX);
            this->sinCamRotX = sinf(rotX);
            this->rotY = rotY;
            this->cosCamRotY = cosf(rotY);
            this->sinCamRotY = sinf(rotY);
            this->fov = fov;
            this->inverseTanCamFovOver2 = 1 / tanf(fov / 2);
            this->width = width;
            this->height = height;
        };
        
        void moveForward()
        {
            this->camPos.x += CameraTransform::posStep * cosf(M_PI_2 + this->rotY);
            this->camPos.z += CameraTransform::posStep * sinf(M_PI_2 + this->rotY);
        };
        void moveBackward()
        {
            this->camPos.x -= CameraTransform::posStep * cosf(M_PI_2 + this->rotY);
            this->camPos.z -= CameraTransform::posStep * sinf(M_PI_2 + this->rotY);
        };
        void moveLeft()
        {
            this->camPos.x += CameraTransform::posStep * cosf(M_PI + this->rotY);
            this->camPos.z += CameraTransform::posStep * sinf(M_PI + this->rotY);
        };
        void moveRight()
        {
            this->camPos.x -= CameraTransform::posStep * cosf(M_PI + this->rotY);
            this->camPos.z -= CameraTransform::posStep * sinf(M_PI + this->rotY);
        };
        void moveUp()
        {
            this->camPos.y += CameraTransform::posStep;
        };
        void moveDown()
        {
            this->camPos.y -= CameraTransform::posStep;
        };
        void rotateUp()
        {
            this->rotX += CameraTransform::rotStep;
            this->cosCamRotX = cosf(rotX);
            this->sinCamRotX = sinf(rotX);
        };
        void rotateDown()
        {
            this->rotX -= CameraTransform::rotStep;
            this->cosCamRotX = cosf(rotX);
            this->sinCamRotX = sinf(rotX);
        };
        void rotateLeft()
        {
            this->rotY += CameraTransform::rotStep;
            this->cosCamRotY = cosf(rotY);
            this->sinCamRotY = sinf(rotY);
        };
        void rotateRight()
        {
            this->rotY -= CameraTransform::rotStep;
            this->cosCamRotY = cosf(rotY);
            this->sinCamRotY = sinf(rotY);
        };

        Vec4 transformVec(const Vec4& v)
        {
            // translate
            Vec3 vector = Vec3(v.x - this->camPos.x, v.y - this->camPos.y, v.z - this->camPos.z);
            // rotate
            vector = Vec3(vector.x * this->cosCamRotY + vector.z * this->sinCamRotY, vector.y, vector.z * this->cosCamRotY - vector.x * this->sinCamRotY);
            vector = Vec3(vector.x, vector.y * this->cosCamRotX - vector.z * this->sinCamRotX, vector.y * this->sinCamRotX + vector.z * this->cosCamRotX);
            if(vector.z <= 0){
                return Vec4(0, 0, -1, 0);
            };
            // perspective
            vector.x /= vector.z * this->inverseTanCamFovOver2;
            vector.y /= vector.z * this->inverseTanCamFovOver2;
            // fit
            vector.x = 0.5 * this->width + vector.x * this->width;
            vector.y = 0.5 * this->height - vector.y * this->width;

            return Vec4(vector.x, vector.y, vector.z, v.a);
        };
        std::vector<Vec4> transformSpace(const std::vector<Vec4>& solutions)
        {
            std::vector<Vec4> transformed;
            for(auto v : solutions)
            {
                Vec4 vec = this->transformVec(v);
                if(vec.z > 0){
                    transformed.push_back(vec);
                };
            };
            std::sort(transformed.begin(), transformed.end(), [](Vec4 a, Vec4 b) -> bool {
                return a.z > b.z;
            });
            return transformed;
        };
};

class FunctionSpace
{
    public:
        int minX;
        int minY;
        int minZ;
        int maxX;
        int maxY;
        int maxZ;

        FunctionSpace() : FunctionSpace(-10, -20, -10, 10, 20, 10){};
        FunctionSpace(int minX, int minY, int minZ, int maxX, int maxY, int maxZ)
        {
            this->minX = minX;
            this->minY = minY;
            this->minZ = minZ;
            this->maxX = maxX;
            this->maxY = maxY;
            this->maxZ = maxZ;
        };
};

class Function
{
    private:
        std::function<float(float x, float z)> evaluate;
        float color(const FunctionSpace& bounds, float y)
        {
            return (y - bounds.minY) / (bounds.maxY - bounds.minY);
        };
    public:
        Function() : Function([](float, float) -> float {
            return 0;
        }){};
        Function(std::function<float(float x, float z)> evaluate)
        {
            this->evaluate = evaluate;
        };

        std::vector<Vec4> generateSolutionSpace(const FunctionSpace& bounds)
        {
            std::vector<Vec4> solutions;
            for(float x = bounds.minX;x<=bounds.maxX;x+=0.05)
            {
                for(float z = bounds.minZ;z<=bounds.maxZ;z+=0.05)
                {
                    float y = this->evaluate(x, z);
                    if(y >= bounds.minY && y <= bounds.maxY){
                        solutions.push_back(Vec4(x, y, z, this->color(bounds, y)));
                    };
                };
            };
            return solutions;
        };
};

class Bounds
{
    public:
        static inline const Vec3 bound1 = Vec3(250, 0, 0);
        static inline const Vec3 bound2 = Vec3(250, 150, 50);
        static inline const Vec3 bound3 = Vec3(250, 250, 50);
        static inline const Vec3 bound4 = Vec3(50, 250, 50);
        static inline const Vec3 bound5 = Vec3(50, 100, 250);
        static inline const Vec3 bound6 = Vec3(150, 50, 250);
};

Vec3 interpolateColor(float a)
{
    Vec3 lowerBound;
    Vec3 upperBound;

    if(a < 0.2){
        lowerBound = Bounds::bound1;
        upperBound = Bounds::bound2;
    } else if(a < 0.4){
        lowerBound = Bounds::bound2;
        upperBound = Bounds::bound3;
        a -= 0.2;
    } else if(a < 0.6){
        lowerBound = Bounds::bound3;
        upperBound = Bounds::bound4;
        a -= 0.4;
    } else if(a < 0.8){
        lowerBound = Bounds::bound4;
        upperBound = Bounds::bound5;
        a -= 0.6;
    } else {
        lowerBound = Bounds::bound5;
        upperBound = Bounds::bound6;
        a -= 0.8;
    };

    a /= 0.2;

    return Vec3(
        lowerBound.x * (1 - a) + upperBound.x * a,
        lowerBound.y * (1 - a) + upperBound.y * a,
        lowerBound.z * (1 - a) + upperBound.z * a
    );
};

class Modeler
{
    private:
        std::vector<Vec4> solutions;
    public:
        CameraTransform camTransform;
        FunctionSpace bounds;

        Modeler(){};
        Modeler(const FunctionSpace& bounds, Function& function)
        {
            this->camTransform = CameraTransform();

            this->bounds = bounds;

            this->solutions = function.generateSolutionSpace(this->bounds);
        };

        std::vector<Vec4> getSolutionsInRenderSpace()
        {
            return this->camTransform.transformSpace(this->solutions);
        };
};

class Manager
{
    private:
        Modeler modeler;

        SDL_Window* window;
        SDL_Renderer* renderer;

        bool running;
    public:
        Manager(const Modeler& modeler)
        {
            this->modeler = modeler;

            this->running = SDL_CreateWindowAndRenderer(this->modeler.camTransform.width, this->modeler.camTransform.height, 0, &this->window, &this->renderer) == 0;
        };

        void redraw()
        {
            SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
            SDL_RenderClear(this->renderer);
            for(auto v : this->modeler.getSolutionsInRenderSpace())
            {
                Vec3 color = interpolateColor(v.a);
                SDL_SetRenderDrawColor(this->renderer, color.x, color.y, color.z, 255);
                SDL_RenderDrawPoint(renderer, v.x, v.y);
            };
            SDL_RenderPresent(this->renderer);
        };

        void runUntilQuit()
        {
            SDL_Event e;

            this->redraw();

            while(running)
            {
                while(SDL_PollEvent(&e))
                {
                    if(e.type == SDL_QUIT){
                        this->running = false;
                    } else {
                        switch(e.key.keysym.sym)
                        {
                            case SDLK_UP:
                                modeler.camTransform.moveForward();
                                this->redraw();
                                break;
                            case SDLK_DOWN:
                                modeler.camTransform.moveBackward();
                                this->redraw();
                                break;
                            case SDLK_LEFT:
                                modeler.camTransform.moveLeft();
                                this->redraw();
                                break;
                            case SDLK_RIGHT:
                                modeler.camTransform.moveRight();
                                this->redraw();
                                break;
                            case SDLK_a:
                                modeler.camTransform.rotateLeft();
                                this->redraw();
                                break;
                            case SDLK_d:
                                modeler.camTransform.rotateRight();
                                this->redraw();
                                break;
                            case SDLK_w:
                                modeler.camTransform.rotateUp();
                                this->redraw();
                                break;
                            case SDLK_s:
                                modeler.camTransform.rotateDown();
                                this->redraw();
                                break;
                        };
                    };
                };
            };
        };
};

int main()
{
    FunctionSpace bounds = FunctionSpace();
    Function function = Function([](float x, float z) -> float {
        return z * sinf(100 * x * M_PI / 180) + x * sinf(100 * z * M_PI / 180);
    });

    Modeler modeler = Modeler(bounds, function);

    Manager manager = Manager(modeler);
    manager.runUntilQuit();
    
    return 0;
};