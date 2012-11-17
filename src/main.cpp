#include <functional>

using std::function;

template <typename T> using Class = function<void (T&)>;

struct Action {
  function<void ()> perform;
};

struct ClickHandler {
  function<void (int x, int y)> onClick;
};

inline Class<ClickHandler> onClick(Class<Action> action) {
  return [=] (ClickHandler& self) {
    Action _action;
    action(_action);

    self.onClick = [=] (int /*x*/, int /*y*/) {
      _action.perform();
    };
  };
}

int main() {
  return 0;
}
