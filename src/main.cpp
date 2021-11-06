#include <random>
#include <cassert>
#include <fmt/format.h>
#include <valarray>

std::default_random_engine randomEngine {std::random_device()()};

constexpr float PICK_SUCCESS_PTS = 10;
constexpr float PICK_FAIL_PTS = -1;
constexpr float WALL_HIT_PTS = -5;

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
  int canCount = {0};

  // struct ArgsCreateRandom {};

  World(float fill)
  {
    std::uniform_real_distribution<float> uniformRealDistribution;
    for (int y = 0; y < HEIGHT; ++y) {
      for (int x = 0; x < WIDTH; ++x) {
        auto randomFloat = uniformRealDistribution(randomEngine);
        hasCan[y][x] = randomFloat < fill;
        canCount += hasCan[y][x] ? 1 : 0;
      }
    }
  }

  bool tryPickCan(int x, int y)
  {
    assert(isCoordinateValid(x, y));
    if (!hasCan[y][x]) {
      return false;
    }
    hasCan[y][x] = false;
    canCount -= 1;
    return true;
  }

  Input::State getState(int x, int y)
  {
    bool xValid = (0 <= x && x < WIDTH);
    bool yValid = (0 <= y && y < HEIGHT);
    if (!xValid || !yValid) {
      return Input::State::WALL;
    }
    return hasCan[y][x] ? Input::State::CAN : Input::State::EMPTY;
  }

  Input getInput(int x, int y)
  {
    assert(isCoordinateValid(x, y));
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
        char cellChar  = hasCan[y][x] ? '+' : '.';
        fmt::format_to(std::back_inserter(repr), "{} ", cellChar);
      }
      fmt::format_to(std::back_inserter(repr), "\n");
    }
    return repr;
  }

  bool isCoordinateValid(int x, int y)
  {
    return (0 <= x && x < World::WIDTH) && (0 <= y && y < World::HEIGHT);
  }
};

struct RobotGenome
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
  static constexpr std::array<Action, 4> MoveAction {RobotGenome::Action::MOVE_NORTH, RobotGenome::Action::MOVE_EAST, RobotGenome::Action::MOVE_SOUTH, RobotGenome::Action::MOVE_WEST};
  struct RandomArgs {};

  static constexpr int LENGTH = Input::COMBINATIONS;
  Action rule[LENGTH];

  RobotGenome(RandomArgs _) {
    std::uniform_int_distribution<> uniformIntDistribution(0, static_cast<int>(Action::COUNT) - 1);
    for (auto&& _rule : rule) {
      _rule = static_cast<Action>(uniformIntDistribution(randomEngine));
    }
  }

  RobotGenome(const RobotGenome& parentA, const RobotGenome& parentB)
  {
    // TODO: What will happen if this distribution is different (e.g. binomial)?
    std::uniform_int_distribution<> geneIndexDist(0, static_cast<int>(RobotGenome::LENGTH) - 1);
    int splitIndex = geneIndexDist(randomEngine);
    assert(0 <= splitIndex && splitIndex < RobotGenome::LENGTH);
    assert((std::fill(rule, rule + RobotGenome::LENGTH, Action::COUNT), true));
    std::copy(parentA.rule, parentA.rule + splitIndex, rule);
    std::copy(parentA.rule + splitIndex, parentA.rule + RobotGenome::LENGTH, rule + splitIndex);
    assert(std::none_of(rule, rule + RobotGenome::LENGTH, [](auto&& action) {return action == Action::COUNT;}));
  }

  std::string toString()
  {
    std::string repr;
    for (int i = 0; i < LENGTH; ++i) {
      fmt::format_to(std::back_inserter(repr), "{} -> {}\n", Input(i).toString(), actionToString(rule[i]));
    }
    return repr;
  }


  void mutate(int geneCount)
  {
    assert(geneCount < RobotGenome::LENGTH);
    std::uniform_int_distribution<> indexDist(0, static_cast<int>(RobotGenome::LENGTH) - 1);
    std::uniform_int_distribution<> actionDist(0, static_cast<int>(Action::COUNT) - 1);
    for (int i = 0; i < geneCount; ++i) {

      int mutatedIndex = indexDist(randomEngine);
      rule[mutatedIndex] = static_cast<Action>(actionDist(randomEngine));
    }
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
  auto world = World(World::FILL);
  fmt::print("{}", world.toString());
  fmt::print("Total cans: {}\n", world.canCount);
  fmt::print("Current input: {}\n", world.getInput(0, 0).toString());
  fmt::print("\n");


  fmt::print("Input combinations + integer conversion\n");
  for (int i = 0; i < Input::COMBINATIONS; ++i) {
    fmt::print("{} -> {} -> {}\n", i, Input(i).toString(), static_cast<int>(Input(i)));
  }
  fmt::print("\n");

  fmt::print("Random robot\n");
  auto robot = RobotGenome(RobotGenome::RandomArgs{});
  fmt::print("{}", robot.toString());
  fmt::print("\n");
}

std::vector<RobotGenome> breedNextGeneration(std::vector<RobotGenome>&& currentGeneration, const std::vector<float>& score, int mutationCount)
{
  std::vector<RobotGenome> nextGeneration;
  std::vector<float> weights = score;
  std::discrete_distribution<> sampleByScore{std::begin(weights), std::end(weights)};

  nextGeneration.clear();
  while (nextGeneration.size() < currentGeneration.size()) {

    int idxParentA = sampleByScore(randomEngine);
    int idxParentB = sampleByScore(randomEngine);
    if (idxParentA == idxParentB) {
      continue;
    }
    // fmt::print("child={}: {} + {}\n", nextGeneration.size(), score[idxParentA], score[idxParentB]);
    RobotGenome&& child = RobotGenome(currentGeneration[idxParentA], currentGeneration[idxParentB]);
    child.mutate(mutationCount);

    nextGeneration.emplace_back(child);
  }
  return nextGeneration;
}

float simulate(const RobotGenome& robotGenome, World& world, const int MAX_STEPS)
{
  int rx = world.WIDTH / 2;
  int ry = world.HEIGHT / 2;
  float score = 0;
  for (int s = 0; s < MAX_STEPS && world.canCount > 0; ++s) {
    int dx = 0, dy = 0;
    auto&& input = world.getInput(rx, ry);
    RobotGenome::Action action = robotGenome.rule[static_cast<int>(input)];
    std::uniform_int_distribution<> movesDist(0, RobotGenome::MoveAction.size() - 1);
    if (action == RobotGenome::Action::MOVE_RANDOM) {
        action = RobotGenome::MoveAction[movesDist(randomEngine)];
    }
    switch (action) {
      case RobotGenome::Action::STAY_PUT:
        break;
      case RobotGenome::Action::TRY_PICK:
        score += (world.tryPickCan(rx, ry) ? PICK_SUCCESS_PTS : PICK_FAIL_PTS);
        break;
      case RobotGenome::Action::MOVE_NORTH:
        dy = 1;
        break;
      case RobotGenome::Action::MOVE_EAST:
        dx = 1;
        break;
      case RobotGenome::Action::MOVE_SOUTH:
        dy = -1;
        break;
      case RobotGenome::Action::MOVE_WEST:
        dx = -1;
        break;
      default:
        assert(false);
    }
    if (!world.isCoordinateValid(rx + dx, ry + dy)) {
      score += WALL_HIT_PTS;
      dx = 0;
      dy = 0;
    }
    rx += dx;
    ry += dy;
  }
  return score;
}

// TODO: nothing prohibits us from using multiple parents to generate a single child :)
int main()
{
  constexpr int N = 10000;
  constexpr int mutationCount = 1;
  std::vector<RobotGenome> robots;
  std::vector<float> scores;

  // Generate initial population
  for (int i = 0; i < N; ++i) {
    robots.emplace_back(RobotGenome::RandomArgs{});
    scores.emplace_back(1.0f / static_cast<float>(N));
  }

  fmt::print("generation,score\n");
  for (int gen = 0; gen < 1e6; ++gen) {
    robots = breedNextGeneration(std::move(robots), scores, mutationCount);
    for (int i = 0; i < robots.size(); ++i) {
      auto&& world = World(World::FILL);
      float maxPoints = world.canCount * PICK_SUCCESS_PTS;
      float points = simulate(robots[i], world, World::WIDTH * World::HEIGHT);
      scores[i] = points > 0 ? points / maxPoints : 0;
    }
    float maxScore = *std::max_element(scores.begin(), scores.end());
    fmt::print("{},{}\n", gen, maxScore);
  }
}
