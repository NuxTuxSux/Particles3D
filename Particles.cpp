#include <iostream>
#include <vector>
#include <ctime>
#include <SDL2/SDL.h>
#include <unistd.h>
#include <math.h>


using namespace std;

#define N_KINDS 16

#define N_POINTS 2602
#define MAX_V0 6.9f
#define FORCE_FACTOR 0.8f
// #define ONLY_LOCAL_FORCES 0
#define GLOBAL_FORCES 0
#define MAX_DIST 490

#define FULLSCREEN 0
#define WINDOW_WIDTH 1480
#define WINDOW_HEIGHT 1024
#define WINDOW_DEPTH 800
#define EYE_Z -600
#define CIRCLES 4
#define R  3

// #define PALETTE_DEPTH
#define DELAY 0//20000


inline void drawCircle(SDL_Renderer *renderer, int x, int y, int radius)
{
    int w, h;
    for (w = x-radius; w <= x+radius; w++)
    {
        for (h = y-radius; h <= y+radius; h++)
        {
            if (((w-x)*(w-x) + (h-y)*(h-y)) <= (radius * radius))
            {
                SDL_RenderDrawPoint(renderer, w, h);
            }
        }
    }
}

const float MAX_SQ_DIST = MAX_DIST * MAX_DIST;
vector<vector<float>> randomForce(int n) {
    srand(time(0));
    vector<vector<float>> F = vector<vector<float>>();
    vector<float> f;
    for (int i = 0; i < n; i++){
        f = vector<float>();
        for (int j = 0; j < n; j++) {
            f.insert(f.begin(),
                // (std::rand() % 2) ? float(std::rand()) / INT_MAX - .5 : 0
                (std::rand() % 2) ? 0 : (float(std::rand() % 5) - 2)/2
            );
            // f.insert(f.begin(), (2*((i+j) % 2) - 1) * float(std::rand()) / INT_MAX);
        }
        F.insert(F.begin(), f);
    }
    return F;
}


struct Color {      // add alpha channel
    uint8_t r, g, b;
};


const Color colors[] = {
    Color{255, 0, 0},
    Color{0, 255, 0},
    Color{0, 0, 255},
    Color{255, 255, 0}
};

struct Color kindColor(int kind) {
    /*return Color{
        uint8_t(255 - (kind % 3) * 255/26),
        uint8_t(255 - (kind % 9) * 255/26),
        uint8_t(255 - (kind % 27) * 255/26)
    };*/
    return colors[kind];
};



struct Vec3D {
    float x, y, z;
};

struct Point3D {
    Vec3D pos, vel;
    uint8_t kind;
    Color color;
};

struct Point2D {
    int x, y;
    Color color;
};

bool farther(Point3D p, Point3D q) {
    return p.pos.z > q.pos.z;
}


// forse coordinate intere?
Point3D randPoint(Vec3D from, Vec3D to) {
    int width = int(to.x - from.x);
    int height = int(to.y - from.y);
    int depth = int(to.z - from.z);
    uint8_t kind = static_cast<uint8_t>(std::rand() % N_KINDS);
    return Point3D{
        Vec3D{
            from.x + float(std::rand() % width),
            from.y + float(std::rand() % height),
            from.z + float(std::rand() % depth)
        },
        Vec3D{
            -MAX_V0/2 + float(std::rand()) / INT_MAX * MAX_V0,
            -MAX_V0/2 + float(std::rand()) / INT_MAX * MAX_V0,
            -MAX_V0/2 + float(std::rand()) / INT_MAX * MAX_V0
        },
        kind,
        // Color{static_cast<uint8_t>(std::rand() % 256), static_cast<uint8_t>(std::rand() % 256), static_cast<uint8_t>(std::rand() % 256)}
        kindColor(kind)
    };
}

vector<vector<float>> forceConstant = randomForce(N_KINDS);

class PointsSystem {
    public:
    // std::vector<Point3D> points;
    Point3D points[N_POINTS];
    Vec3D from, to;          // box's dimensions
    // PointsSystem(Point3D[] points, Vec3D from, Vec3D to): points(points), from(from), to(to) {}
    PointsSystem(Vec3D from, Vec3D to): from(from), to(to) {
        // points = std::vector<Point3D>();
        for (int i = 0; i < N_POINTS; i++)
            // points.insert(points.begin(), randPoint(from, to));
            points[i] = randPoint(from, to);
    }
    void step() {
        float fx, fy, fz;
        for (int i = 0; i < N_POINTS; i++) {
            Point3D p = points[i];
            fx = 0;
            fy = 0;
            fz = 0;
            // for (Point3D &q: points) {
            for (int j = 0; j < N_POINTS; j++) {
                const Point3D q = points[j];
                const float dx = q.pos.x - p.pos.x;
                const float dy = q.pos.y - p.pos.y;
                const float dz = q.pos.z - p.pos.z;
                if (dx != 0 || dy != 0 || dz != 0) {
                    // calculate force
                    const float dsq = dx*dx + dy*dy + dz*dz;
                    #if GLOBAL_FORCES
                        const float F = forceConstant[p.kind][q.kind]/sqrt(dsq);
                        fx += F * dx;
                        fy += F * dy;
                        fz += F * dz;
                    #else
                        if (dsq < MAX_SQ_DIST) {
                            // const float d = sqrt(dsq);
                            const float F = forceConstant[p.kind][q.kind]/sqrt(dsq);
                            fx += F * dx;
                            fy += F * dy;
                            fz += F * dz;
                        }
                    #endif
                }
            }
            p.vel.x = (p.vel.x + fx) * 0.5;
            p.vel.y = (p.vel.y + fy) * 0.5;
            p.vel.z = (p.vel.z + fz) * 0.5;
            p.pos.x += p.vel.x;
            p.pos.y += p.vel.y;
            p.pos.z += p.vel.z;
            // p.pos.x = 10. *(static_cast<int>(p.pos.x) / 10);
            // p.pos.y = 10. *(static_cast<int>(p.pos.y) / 10);
            // p.pos.z = 10. *(static_cast<int>(p.pos.z) / 10);
            if (p.pos.x < from.x) p.pos.x = from.x;
            if (p.pos.y < from.y) p.pos.y = from.y;
            if (p.pos.z < from.z) p.pos.z = from.z;
            if (p.pos.x > to.x) p.pos.x = to.x;
            if (p.pos.y > to.y) p.pos.y = to.y;
            if (p.pos.z > to.z) p.pos.z = to.z;
            points[i] = p;
        }
        // sort(points.begin(), points.end(), farther);
    }
};



class Box3D {
    // initialize SDL and window first
    private:
    Point2D f00, f01, f10, f11, b00, b01, b10, b11;

    public:
    Vec3D from, to, eye;
    float width, height;

    SDL_Renderer *renderer2D;
    SDL_Window *window;

    // std::vector<Point3D> points;
    Box3D(Vec3D from, Vec3D to, Vec3D eye): from(from), to(to), eye(eye) {
        width = to.x - from.x;
        height = to.y - from.y;
        // video init
        SDL_Init(SDL_INIT_VIDEO);
        // window/renderer creation
        SDL_CreateWindowAndRenderer(int(width), int(height), 0, &window, &renderer2D);
        SDL_SetRenderDrawBlendMode(renderer2D, SDL_BLENDMODE_ADD);                          //QUA
        if (FULLSCREEN)
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        // define borders and clear screen (not much clean code)
        f00 = Point2D{0,0,Color{0,0,0}};
        f01 = Point2D{0,2*int(to.y)-1,Color{0,0,0}};
        f10 = Point2D{2*int(to.x)-1,0,Color{0,0,0}};
        f11 = Point2D{2*int(to.x)-1,2*int(to.y)-1,Color{0,0,0}};
        b00 = project(Point3D{Vec3D{-to.x,-to.y,to.z},Vec3D{0,0,0},0,Color{0,0,0}});
        b01 = project(Point3D{Vec3D{-to.x,to.y,to.z},Vec3D{0,0,0},0,Color{0,0,0}});
        b10 = project(Point3D{Vec3D{to.x,-to.y,to.z},Vec3D{0,0,0},0,Color{0,0,0}});
        b11 = project(Point3D{Vec3D{to.x,to.y,to.z},Vec3D{0,0,0},0,Color{0,0,0}});
        clear();
    }

    void clear() {
        SDL_SetRenderDrawColor(renderer2D, 15, 15, 15, 255);
        SDL_RenderClear(renderer2D);
        SDL_SetRenderDrawColor(renderer2D, 150, 100, 10, 30);
        
        SDL_RenderDrawLine(renderer2D, f00.x, f00.y, f01.x, f01.y);
        SDL_RenderDrawLine(renderer2D, f00.x, f00.y, f10.x, f10.y);
        SDL_RenderDrawLine(renderer2D, f11.x, f11.y, f01.x, f01.y);
        SDL_RenderDrawLine(renderer2D, f11.x, f11.y, f10.x, f10.y);
        
        SDL_RenderDrawLine(renderer2D, b00.x, b00.y, b01.x, b01.y);
        SDL_RenderDrawLine(renderer2D, b00.x, b00.y, b10.x, b10.y);
        SDL_RenderDrawLine(renderer2D, b11.x, b11.y, b01.x, b01.y);
        SDL_RenderDrawLine(renderer2D, b11.x, b11.y, b10.x, b10.y);

        SDL_RenderDrawLine(renderer2D, f00.x, f00.y, b00.x, b00.y);
        SDL_RenderDrawLine(renderer2D, f01.x, f01.y, b01.x, b01.y);
        SDL_RenderDrawLine(renderer2D, f10.x, f10.y, b10.x, b10.y);
        SDL_RenderDrawLine(renderer2D, f11.x, f11.y, b11.x, b11.y);
    }


    Point2D project(Point3D p) {
        const float lambda = eye.z/(eye.z-p.pos.z);
        return Point2D{
            int(eye.x + lambda * (p.pos.x - eye.x)) + int(width/2),
            int(eye.y + lambda * (p.pos.y - eye.y)) + int(height/2),
            p.color
        };
    }

    void drawPoint(Point3D p) {
        const Point2D proj = project(p);
        // SDL_SetRenderDrawColor(renderer2D, proj.color.r, proj.color.g, proj.color.b, 255-Uint8(2*p.pos.z/WINDOW_DEPTH*255));        // farest points got half alpha
        SDL_SetRenderDrawColor(renderer2D, int((WINDOW_DEPTH - p.pos.z/2/WINDOW_DEPTH)*proj.color.r), int((WINDOW_DEPTH - p.pos.z/2/WINDOW_DEPTH)*proj.color.g), int((WINDOW_DEPTH - p.pos.z/2/WINDOW_DEPTH)*proj.color.b), 255);        // farest points got half alpha
        #if CIRCLES
            drawCircle(renderer2D, proj.x, proj.y, int((R/(EYE_Z-p.pos.z))*EYE_Z));             // aproximated radius reduction
        #else
            SDL_RenderDrawPoint(renderer2D, proj.x, proj.y);             // aproximated radius reduction
        #endif
    }

    void drawPoints(Point3D ps[]) {
        for (int i = 0; i < N_POINTS; i++)
            drawPoint(ps[i]);
        SDL_RenderPresent(renderer2D);
    }

    ~Box3D() {
        SDL_DestroyRenderer(renderer2D);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

};






int main() {

    SDL_Event event;
    // inizializzo il seme random
    // std::srand(std::time(0));
    // std::srand(N_KINDS + N_POINTS + ONLY_LOCAL_FORCES + WINDOW_WIDTH + WINDOW_HEIGHT + WINDOW_DEPTH + EYE_Z);
    
    Box3D box = Box3D(
        Vec3D{
            -WINDOW_WIDTH/2,
            -WINDOW_HEIGHT/2,
            0
        },
        Vec3D{
            WINDOW_WIDTH/2,
            WINDOW_HEIGHT/2,
            WINDOW_DEPTH
        },
        Vec3D {0, 0, EYE_Z}
    );



    PointsSystem psystem = PointsSystem(box.from, box.to);

    for (auto r: forceConstant) {
        for (float x: r) {
            cout << x << "\t";
        }
        cout << endl;
    }

    while (1) {
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
           break;
        // while (SDL_PollEvent(&event)) {
            // switch (event.type) {
                // case SDL_QUIT:
                    // break;
                /*case SDL_KEYDOWN:
                    SDL_SaveBMP(SDL_GetWindowSurface(box.window), "prova.bmp");*/
            // }
        // }
        box.clear();
        box.drawPoints(psystem.points);
        psystem.step();
        usleep(DELAY);
        
    }



    return EXIT_SUCCESS;
}
