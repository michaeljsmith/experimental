#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <array>

using std::function;
using std::shared_ptr;
using std::make_shared;
using std::string;
using std::cout;
using std::array;

using Bound = int;
using AxisBounds = array<Bound, 2>;
using Bounds = array<AxisBounds, 2>;

using Position = int;
using Point = array<Position, 2>;

inline bool positionInAxisBounds(Position position, AxisBounds axisBounds) {
  return position >= axisBounds[0] &&
    position < axisBounds[1];
}

inline bool pointInBounds(Point point, Bounds bounds) {
  for (size_t axis = 0; axis < 2; ++axis) {
    if (!positionInAxisBounds(point[axis], bounds[axis])) {
      return false;
    }
  }
  return true;
}

struct Widget {
  Widget(function<void (int, int)> _onClick): onClick(_onClick) {}
  function<void (int, int)> onClick;
};

//inline shared_ptr<Widget> optionsMenu() {
//}

inline shared_ptr<Widget> mainMenu() {
  auto self = make_shared<Widget>([=] (int, int) {
    cout << "clicked\n";
    self->onClick
  });
  return self;
}

int main() {
  auto app = mainMenu();
  app->onClick(5, 5);

  return 0;
}
