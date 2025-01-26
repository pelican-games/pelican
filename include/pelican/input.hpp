namespace pl {

// don't use as global variable
class Key {
    int keycode;
  public:
    Key(int keycode);
    ~Key();
    bool down() const;
    bool up() const;
    bool pressed() const;
};

class Cursor {
  public:
    struct Point {
        int x, y;
    };
    void setPos(Point point);
    Point getPos() const;

    bool left_down() const;
    bool left_up() const;
    bool left_pressed() const;
    bool right_down() const;
    bool right_up() const;
    bool right_pressed() const;
};

} // namespace pl
