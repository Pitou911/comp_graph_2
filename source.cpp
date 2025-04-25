#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>

constexpr float WIDTH = 900.0f;
constexpr float HEIGHT = 600.0f;
constexpr float POINT_THRESHOLD = 10.0f;
constexpr float CURVE_STEP = 0.0001f;

// Struct for RGB color
struct Color {
    float r, g, b;
    constexpr Color(float r, float g, float b) : r(r), g(g), b(b) {}
};

constexpr Color YELLOW(1.0f, 1.0f, 0.0f);
constexpr Color RED(1.0f, 0.0f, 0.0f);
constexpr Color CONTROL_POINT(0.839f, 0.0f, 0.156f);
constexpr Color CONTROL_LINE(0.188f, 0.360f, 0.992f);
constexpr Color CURVE(0.0f, 1.0f, 0.3f);

// Struct for 2D point
struct Point {
    float x, y;
    Point() = default;
    Point(float x, float y) : x(x), y(y) {}

    Point operator*(float t) const { return { t * x, t * y }; }
    Point operator+(const Point& p) const { return { x + p.x, y + p.y }; }
};

class BezierCurve {
private:
    std::vector<Point> control_points;
    std::vector<Point> curve_points;
    std::vector<Point> moving_points;
    std::vector<Point>::iterator move_iter;
    bool is_moving = false;
    bool is_deleting = false;

    // Convert screen to OpenGL coordinates
    static Point screen_to_gl(Point p) {
        return {
            (p.x - WIDTH / 2) / WIDTH * 2,
            (HEIGHT / 2 - p.y) / HEIGHT * 2
        };
    }

    // Check if points are close enough
    static bool is_close(Point p1, Point p2) {
        return std::sqrt(std::pow(p1.x - p2.x, 2) +
            std::pow(p1.y - p2.y, 2)) <= POINT_THRESHOLD;
    }

    // De Casteljau's algorithm
    void compute_point(float t) {
        std::vector<Point> temp = control_points;
        for (size_t k = 1; k < control_points.size(); ++k) {
            for (size_t i = 0; i < control_points.size() - k; ++i) {
                temp[i] = temp[i] * (1 - t) + temp[i + 1] * t;
            }
        }
        if (!temp.empty()) {
            curve_points.push_back(temp[0]);
        }
    }

public:
    void compute_curve() {
        curve_points.clear();
        if (control_points.size() >= 2) {
            for (float t = 0; t <= 1.0f; t += CURVE_STEP) {
                compute_point(t);
            }
        }
    }

    void draw_controls() {
        // Draw control lines
        if (control_points.size() >= 2) {
            glLineWidth(1.0f);
            glBegin(GL_LINE_STRIP);
            glColor3f(CONTROL_LINE.r, CONTROL_LINE.g, CONTROL_LINE.b);
            for (const auto& p : control_points) {
                Point gl_p = screen_to_gl(p);
                glVertex2f(gl_p.x, gl_p.y);
            }
            glEnd();
        }

        // Draw control points
        if (!control_points.empty()) {
            glPointSize(15.0f);
            glBegin(GL_POINTS);
            glColor3f(CONTROL_POINT.r, CONTROL_POINT.g, CONTROL_POINT.b);
            for (auto it = control_points.begin(); it != control_points.end(); ++it) {
                Point p = (is_moving && it == move_iter && !moving_points.empty())
                    ? moving_points.back() : *it;
                Point gl_p = screen_to_gl(p);
                glVertex2f(gl_p.x, gl_p.y);
            }
            glEnd();
        }
    }

    void draw_curve() {
        if (!curve_points.empty()) {
            glPointSize(5.0f);
            glBegin(GL_POINTS);
            glColor3f(CURVE.r, CURVE.g, CURVE.b);
            for (const auto& p : curve_points) {
                Point gl_p = screen_to_gl(p);
                glVertex2f(gl_p.x, gl_p.y);
            }
            glEnd();
        }
    }

    void handle_mouse_press(float x, float y, int button) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && !is_moving) {
            // Check if we're clicking near an existing point
            for (auto it = control_points.begin(); it != control_points.end(); ++it) {
                if (is_close(*it, { x, y })) {
                    is_moving = true;
                    move_iter = it;
                    moving_points.emplace_back(x, y);
                    return;
                }
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            // Right click - try to delete a point
            for (auto it = control_points.begin(); it != control_points.end(); ++it) {
                if (is_close(*it, { x, y })) {
                    control_points.erase(it);
                    compute_curve();
                    return;
                }
            }
        }
    }

    void handle_mouse_release(float x, float y, int button) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (is_moving) {
                move_iter->x = x;
                move_iter->y = y;
                is_moving = false;
                moving_points.clear();
            }
            else {
                // Add new control point
                control_points.emplace_back(x, y);
            }
            compute_curve();
        }
    }

    void handle_key(int key, int action) {
        if (key == GLFW_KEY_DELETE) {
            is_deleting = (action == GLFW_PRESS);
        }
        else if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
            control_points.clear();
            moving_points.clear();
            curve_points.clear();
            is_moving = false;
            is_deleting = false;
        }
        else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            control_points.clear();
            moving_points.clear();
            curve_points.clear();
            is_moving = false;
            is_deleting = false;
        }
    }

    bool is_moving_state() const { return is_moving; }
};

int main() {
    if (!glfwInit()) {
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Bezier Curve", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwSetWindowPos(window, 600, 200);
    glfwMakeContextCurrent(window);

    BezierCurve curve;

    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int mods) {
        static BezierCurve* curve_ptr = nullptr;
        if (!curve_ptr) curve_ptr = static_cast<BezierCurve*>(glfwGetWindowUserPointer(win));

        double x, y;
        glfwGetCursorPos(win, &x, &y);

        if (action == GLFW_PRESS) {
            curve_ptr->handle_mouse_press(static_cast<float>(x), static_cast<float>(y), button);
        }
        else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT) {
            curve_ptr->handle_mouse_release(static_cast<float>(x), static_cast<float>(y), button);
        }
        });

    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) {
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            static BezierCurve* curve_ptr = nullptr;
            if (!curve_ptr) curve_ptr = static_cast<BezierCurve*>(glfwGetWindowUserPointer(win));
            if (curve_ptr->is_moving_state()) {
                curve_ptr->handle_mouse_press(static_cast<float>(x), static_cast<float>(y), GLFW_MOUSE_BUTTON_LEFT);
            }
        }
        });

    glfwSetKeyCallback(window, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
        static BezierCurve* curve_ptr = nullptr;
        if (!curve_ptr) curve_ptr = static_cast<BezierCurve*>(glfwGetWindowUserPointer(win));
        curve_ptr->handle_key(key, action);
        });

    glfwSetWindowUserPointer(window, &curve);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        curve.draw_controls();
        curve.draw_curve();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
