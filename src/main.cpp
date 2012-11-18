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

namespace objects {
  struct Action {
    Action(function<void ()> fn):
      _perform(make_shared<function<void ()>>(fn)) {
    }

    void perform() {(*_perform)();}
    shared_ptr<function<void ()>> _perform;
  };
}

namespace classes {
  using Action = function<objects::Action ()>;
}

namespace objects {
  template <typename T> struct Expression {
    function<T ()> evaluate;
  };
}

namespace classes {
  template <typename T> using Expression =
    function<shared_ptr<objects::Expression<T>> ()>;

  template <typename T> Expression<T> literal(T value) {
    return [=] () -> shared_ptr<objects::Expression<T>> {
      auto self = make_shared<objects::Expression<T>>();
      self->evaluate = [=] () {
        return value;
      };

      return self;
    };
  }
}

namespace objects {
  struct ClickHandler {
    function<void (int x, int y)> onClick;
  };
}

namespace classes {
  using ClickHandler = function<shared_ptr<objects::ClickHandler> (Expression<Bounds>)>;
}

namespace classes {
  inline ClickHandler onClick(Action action) {
    return [=] (Expression<Bounds> bounds) -> shared_ptr<objects::ClickHandler> {
      auto _action = action();
      auto _bounds = bounds();

      auto self = make_shared<objects::ClickHandler>();

      self->onClick = [=] (int x, int y) mutable {
        auto _currentBounds = _bounds->evaluate();
        if (pointInBounds({{x, y}}, _currentBounds)) {
          _action.perform();
        }
      };

      return self;
    };
  }
}

namespace objects {
  struct Renderable {
    function<void ()> render;
  };
}

namespace classes {
  using Renderable = function<shared_ptr<objects::Renderable> (Expression<Bounds>)>;
}

namespace classes {
  inline Renderable quad() {
    return [=] (Expression<Bounds> bounds) -> shared_ptr<objects::Renderable> {
      auto _bounds = bounds();

      auto self = make_shared<objects::Renderable>();
      self->render = [=] () {
        auto _currentBounds = _bounds->evaluate();
        cout << "Rendering quad at <" <<
          _currentBounds[0][0] << ", " <<
          _currentBounds[0][1] << ", " <<
          _currentBounds[1][0] << ", " <<
          _currentBounds[1][1] << ">";
      };

      return self;
    };
  }
}

namespace classes {
  inline Action print(string text) {
    return [=] () {
      return objects::Action(
          [=] () {
            cout << text;
          }); 
    };
  }
}

int main() {
  using namespace classes;

  auto app = onClick(print("hello\n"));
  auto _app = app(literal<Bounds>({{{{0, 100}}, {{0, 100}}}}));

  _app->onClick(10, 10);

  return 0;
}
