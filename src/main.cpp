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

namespace objects {
  struct Action {
    function<void ()> perform;
  };
}

namespace classes {
  using Action = function<shared_ptr<objects::Action> ()>;
}

namespace objects {
  struct ClickHandler {
    function<void (int x, int y)> onClick;
  };
}

namespace classes {
  using ClickHandler = function<shared_ptr<objects::ClickHandler> ()>;
}

namespace classes {
  inline ClickHandler onClick(Action action) {
    return [=] () -> shared_ptr<objects::ClickHandler> {
      auto _action = action();

      auto self = make_shared<objects::ClickHandler>();

      self->onClick = [=] (int /*x*/, int /*y*/) {
        _action->perform();
      };

      return self;
    };
  }
}

namespace classes {
  inline Action print(string text) {
    return [=] () -> shared_ptr<objects::Action> {
      auto self = make_shared<objects::Action>();
      self->perform = [=] () {
        cout << text;
      };

      return self;
    };
  }
}

int main() {
  using namespace classes;

  auto app = onClick(print("hello\n"));
  auto _app = app();

  _app->onClick(10, 10);

  return 0;
}
