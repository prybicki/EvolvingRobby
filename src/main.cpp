#include <random>
#include <cassert>
#include <fmt/format.h>

std::default_random_engine randomEngine {std::random_device()()};

struct Input {
  enum struct State : int8_t {
    EMPTY,
    WALL,
    CAN,
    COUNT,
  };
  static constexpr int LENGTH = 5;
  static constexpr int COMBINATIONS = 243; // State::Count ^ Input::Count
  static constexpr char DIRECTIONS[] = {'+', '^', '>', 'v', '<'};
  State state[LENGTH];

  Input(State current, State north, State east, State south, State west)
  : state {current, north, east, south, west} { }

  Input(int code) {
    assert(code < Input::COMBINATIONS);
    for (int i = 0; i < Input::LENGTH; ++i) {
      state[Input::LENGTH - (1 + i)] = static_cast<State>(code % static_cast<int>(State::COUNT));
      code /= static_cast<int>(State::COUNT);
    }
  }

  operator int()
  {
    int code = 0;
    for (int i = 0; i < Input::LENGTH; ++i) {
      code *= static_cast<int>(State::COUNT);
      code += static_cast<int>(state[i]);
    }
    assert(code < Input::COMBINATIONS);
    return code;
  }

  std::string toString()
  {
    std::string repr;
    for (int i = 0; i < LENGTH; ++i) {
      fmt::format_to(std::back_inserter(repr), "({}{}) ", DIRECTIONS[i], stateToString(state[i]));
    }
    return repr;
  }

private:
  std::string stateToString(State state)
  {
    switch(state)
    {
      case State::EMPTY: return "Empty";
      case State::WALL: return "Wall";
      case State::CAN: return "Can";
      default: throw std::invalid_argument(fmt::format("invalid state {}", static_cast<int>(state)));
    }
  }
};

struct World
{
  static constexpr int WIDTH = 11;
  static constexpr int HEIGHT = 11;
  static constexpr float FILL = 0.2;
  bool hasCan[HEIGHT][WIDTH] = {false};
  std::pair<int, int> xyRobot = {5, 5};
  int canCount = {0};

  Input::State getState(int x, int y)
  {
    bool xValid = (0 <= x && x < WIDTH);
    bool yValid = (0 <= y && y < HEIGHT);
    if (!xValid || !yValid) {
      return Input::State::WALL;
    }
    return hasCan[y][x] ? Input::State::CAN : Input::State::EMPTY;
  }

  Input getInput()
  {
    auto [x, y] = xyRobot;
    return {
      getState(x,   y  ),
      getState(x,   y+1),
      getState(x+1, y  ),
      getState(x,   y-1),
      getState(x-1,   y)
    };
  }

  std::string toString()
  {
    std::string repr;
    for (int y = HEIGHT-1; y >= 0; --y) {
      for (int x = 0; x < WIDTH; ++x) {
        bool robotHere = (x == xyRobot.first) && (y == xyRobot.second);
        char robotChar = hasCan[y][x] ? '@' : '#';
        char cellChar  = hasCan[y][x] ? '+' : '.';
        char stateChar = robotHere ? robotChar : cellChar;
        fmt::format_to(std::back_inserter(repr), "{} ", stateChar);
      }
      fmt::format_to(std::back_inserter(repr), "\n");
    }
    return repr;
  }

  static World createRandom(float fill)
  {
    World world = {};
    std::uniform_real_distribution<float> uniformRealDistribution;
    for (int y = 0; y < HEIGHT; ++y) {
      for (int x = 0; x < WIDTH; ++x) {
        auto randomFloat = uniformRealDistribution(randomEngine);
        world.hasCan[y][x] = randomFloat < fill;
        world.canCount += world.hasCan[y][x] ? 1 : 0;
      }
    }
    return world;
  }
private:
  World() {};
};


struct Robot
{
  enum struct Action : int8_t {
    STAY_PUT,
    TRY_PICK,
    MOVE_RANDOM,
    MOVE_NORTH,
    MOVE_EAST,
    MOVE_SOUTH,
    MOVE_WEST,
    COUNT,
  };
  struct CreateRandom {};

  static constexpr int LENGTH = Input::COMBINATIONS;
  Action rule[LENGTH];
  float score = {0};

  Robot(CreateRandom _) {
    std::uniform_int_distribution<> uniformIntDistribution(0, static_cast<int>(Action::COUNT) - 1);
    for (auto&& _rule : rule) {
      _rule = static_cast<Action>(uniformIntDistribution(randomEngine));
    }
  }

  std::string toString()
  {
    std::string repr;
    for (int i = 0; i < LENGTH; ++i) {
      fmt::format_to(std::back_inserter(repr), "{} -> {}\n", Input(i).toString(), actionToString(rule[i]));
    }
    return repr;
  }

private:
  std::string actionToString(Action action)
  {
    switch (action) {
      case Action::STAY_PUT: return "Stay";
      case Action::TRY_PICK: return "Try Pick";
      case Action::MOVE_RANDOM: return "Move Random";
      case Action::MOVE_NORTH: return "Move North";
      case Action::MOVE_EAST: return "Move East";
      case Action::MOVE_SOUTH: return "Move South";
      case Action::MOVE_WEST: return "Move West";
      default: throw std::invalid_argument(fmt::format("invalid action {}", static_cast<int>(action)));
    }
  }
};

void doSmokeTest()
{
  fmt::print("Example world\n");
  auto world = World::createRandom(World::FILL);
  fmt::print("{}", world.toString());
  fmt::print("Total cans: {}\n", world.canCount);
  fmt::print("Current input: {}\n", world.getInput().toString());
  fmt::print("\n");


  fmt::print("Input combinations + integer conversion\n");
  for (int i = 0; i < Input::COMBINATIONS; ++i) {
    fmt::print("{} -> {} -> {}\n", i, Input(i).toString(), static_cast<int>(Input(i)));
  }
  fmt::print("\n");

  fmt::print("Random robot\n");
  auto robot = Robot(Robot::CreateRandom{});
  fmt::print("{}", robot.toString());
  fmt::print("\n");
}

int main()
{
  constexpr int N = 1000;
  std::vector<Robot> generation, nextGeneration;
  for (int i = 0; i < N; ++i) {
    generation.emplace_back(Robot::CreateRandom{});
  }

  // TODO: Implement weighted sampling
  // TODO: Generation struct (?)

}