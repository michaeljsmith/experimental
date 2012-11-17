#include <functional>
#include <memory>
#include <string>
#include <iostream>

using std::function;
using std::shared_ptr;
using std::make_shared;
using std::string;
using std::cout;

template <typename T> using Class = function<shared_ptr<T> ()>;

struct Action {
  function<void ()> perform;
};

struct ClickHandler {
  function<void (int x, int y)> onClick;
};

inline Class<ClickHandler> onClick(Class<Action> action) {
  return [=] () -> shared_ptr<ClickHandler> {
    auto _action = action();

    shared_ptr<ClickHandler> self = make_shared<ClickHandler>();

    self->onClick = [=] (int /*x*/, int /*y*/) {
      _action->perform();
    };

    return self;
  };
}

inline Class<Action> print(string text) {
  return [=] () -> shared_ptr<Action> {
    auto self = make_shared<Action>();
    self->perform = [=] () {
      cout << text;
    };

    return self;
  };
}

int main() {
  auto app = onClick(print("hello\n"));
  auto _app = app();

  _app->onClick(10, 10);

  return 0;
}
