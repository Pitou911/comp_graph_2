if (!control_points.empty()) {
    glColor3f(CONTROL_POINT.r, CONTROL_POINT.g, CONTROL_POINT.b);
    const float radius = 0.02f; // Adjust radius to your scale
    const int segments = 20;    // Number of triangle segments for the circle

    for (auto it = control_points.begin(); it != control_points.end(); ++it) {
        Point p = (is_moving && it == move_iter && !moving_points.empty())
            ? moving_points.back() : *it;
        Point gl_p = screen_to_gl(p);

        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(gl_p.x, gl_p.y); // Center of circle
        for (int i = 0; i <= segments; ++i) {
            float angle = i * 2.0f * M_PI / segments;
            float dx = cosf(angle) * radius;
            float dy = sinf(angle) * radius;
            glVertex2f(gl_p.x + dx, gl_p.y + dy);
        }
        glEnd();
    }
}
