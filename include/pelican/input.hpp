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
    Point getPos();
};

} // namespace pl
