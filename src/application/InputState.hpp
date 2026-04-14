#pragma once

namespace doomlike::application {

struct InputState {
    bool moveForward {false};
    bool moveBackward {false};
    bool strafeLeft {false};
    bool strafeRight {false};
    bool turnLeft {false};
    bool turnRight {false};
    bool shootPressed {false};
    bool restartPressed {false};
    bool quitRequested {false};
};

}  // namespace doomlike::application

