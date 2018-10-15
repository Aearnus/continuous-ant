#include <vector>
#include <set>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <list>
#include <algorithm>

template <typename T>
inline T lerp(T v0, T v1, T t) {
    return (1-t)*v0 + t*v1;
}
template <typename T>
inline T orderable_distance(T x1, T y1, T x2, T y2) {
    return (std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

struct Point {
    double x;
    double y;
    double val;
};

struct World {
    struct Ant {
        double x = 0;
        double y = 0;

        double direction = 0;
        const double speed = 1; // delta r/delta t

        // since this is supposed to approximate the 1 square
        // that an ant can influence in the discrete langston's ant,
        // this function has to have a maximum radius of 0.5
        double influence_falloff(double x1, double y1) {
            return std::max(0.0, 1 - std::sqrt(std::pow(x - x1, 2) + std::pow(y - y1, 2)));
        }

        // this function approximates the rule of Langton's ant:
        // it uses influence_falloff and the value at the calculated
        // location to calculate a new value for that location.
        // this is parametrised by time because this all is time
        // continuous as well as space continuous
        double rule(const double t, double x1, double y1, double val) {
            double influence = influence_falloff(x1, y1);
            if (influence == 0.0) { return val; }

            // this is what the value would be after one whole time step
            double val_prime = 1 - val;
            double out = lerp(val, val_prime, t * influence);

            // ensure that calculation error didn't bite us in the ass
            if (out < 0) { return 0; }
            if (out > 1) { return 1; }
            return out;
        }

        double move(const double t, World& w) {
            x += t * speed * std::cos(direction);
            y += t * speed * std::sin(direction);
            //direction = lerp(direction, -M_PI/2 + w.get_val_closest_to(x, y)*M_PI, t);
            direction += (-M_PI/2 + w.get_val_closest_to(x, y)*M_PI)*t;
        }
    };

    std::vector<std::vector<Point>> world;
    Ant ant;
    double world_resolution; //every entry is n units wide

    double get_val_closest_to(double x1, double y1) {
        // TODO: this has suboptimal algorithmic complexity
        /*
        Point& closest_point = world[0][0];
        for (auto& row : world) {
            for (auto& point : row) {
                if (orderable_distance(point.x, point.y, x1, y1) < orderable_distance(closest_point.x, closest_point.y, x1, y1)) {
                    closest_point = point;
                }
            }
        }
        return closest_point.val;
        */
        long closest_x = std::floor(x1 / world_resolution) + ((world.at(0).size() - 1) / 2);
        long closest_y = std::floor(y1 / world_resolution) + ((world.size() - 1) / 2);
        if (closest_x < 0) {
            closest_x = 0;
        } else if (closest_x >= world.at(0).size()) {
            closest_x = world.at(0).size() - 1;
        }
        if (closest_y < 0) {
            closest_y = 0;
        } else if (closest_y >= world.size()) {
            closest_y = world.size() - 1;
        }

        return world[closest_y][closest_x].val;
    }

    void tick(const double timestep) {
        for (auto& row : world) {
            for (auto& point : row) {
                if (std::abs(point.x - ant.x) < 2 and std::abs(point.y - ant.y) < 2) {
                    point.val = ant.rule(timestep, point.x, point.y, point.val);
                }
            }
        }
        ant.move(timestep, *this);
    }

    void naive_print_world() {
        for (auto& row : world) {
            for (auto& point : row) {
                if (point.val < 0.1) {
                    std::cout << " ";
                } else if (point.val < 0.2) {
                    std::cout << ".";
                } else if (point.val < 0.3) {
                    std::cout << ",";
                } else if (point.val < 0.4) {
                    std::cout << "-";
                } else if (point.val < 0.5) {
                    std::cout << "=";
                } else if (point.val < 0.6) {
                    std::cout << "+";
                } else if (point.val < 0.7) {
                    std::cout << "%";
                } else if (point.val < 0.8) {
                    std::cout << "&";
                } else if (point.val < 0.9) {
                    std::cout << "@";
                } else {
                    std::cout << "#";
                }
            }
            std::cout << std::endl;
        }
    }

    void print_pgm() {
        std::cout << "P2" << std::endl;
        std::cout << world.size() << " " << world.size() << std::endl;
        std::cout << "256" << std::endl;
        for (auto& row : world) {
            for (auto& point : row) {
                std::cout << std::floor(point.val * 256) << " ";
            }
        }
    }

    std::string return_pgm() {
        std::stringstream out;
        out << "P2" << std::endl;
        out << world.size() << " " << world.size() << std::endl;
        out << "256" << std::endl;
        for (auto& row : world) {
            for (auto& point : row) {
                out << std::floor(point.val * 256) << " ";
            }
        }
        return out.str();
    }

    World(long radius, double res) {
        world = std::vector<std::vector<Point>>(radius * 2 + 1);
        world_resolution = res;
        for (long i = 0; i < radius * 2 + 1; i++) {
            world[i] =  std::vector<Point>(radius * 2 + 1);
            for (long j = 0; j < radius * 2 + 1; j++) {
                world[i][j] = Point{ (i - radius - 0.5) * world_resolution, (j - radius - 0.5) * world_resolution, 0 };
            }
        }
    }
};

std::string image_file_name(long i) {
    std::stringstream out;
    out << "out";
    out.fill('0');
    out << std::setw(4) << i;
    out << ".pgm";
    return out.str();
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return 1;
    }
    World world = World(400, 0.1);
    std::list<std::thread> thread_pool;
    for (long i = 0; i < atoll(argv[1]); i++) {
        world.tick(0.01);
        std::string out_string = world.return_pgm();
        thread_pool.push_front(std::thread([out_string, i](){
            std::ofstream img_out(image_file_name(i));
            img_out << out_string;
            std::cout << "rendered frame " << image_file_name(i) << std::endl;
        }));
    }
    std::for_each(thread_pool.begin(), thread_pool.end(), [](std::thread& t) { t.join(); });
    return 0;
}
