#include "application/GameLoop.hpp"

#include <algorithm>
#include <chrono>

#include "application/GameSession.hpp"
#include "presentation/SdlApp.hpp"

namespace doomlike::application {

int GameLoop::run(const std::filesystem::path& mapPath)
{
    presentation::SdlApp app;
    if (!app.initialize(DOOMLIKE_PROJECT_NAME, 1280, 720)) {
        return 1;
    }

    GameSession session;
    session.loadMap(mapPath);
    session.startOfflineDemo();

    using clock = std::chrono::steady_clock;
    auto lastTick = clock::now();

    while (app.isRunning()) {
        const auto now = clock::now();
        const float deltaSeconds = std::min(std::chrono::duration<float>(now - lastTick).count(), 0.05F);
        lastTick = now;

        const InputState input = app.pollInput();
        if (input.quitRequested) {
            break;
        }

        session.update(deltaSeconds, input);
        app.render(session.world(), session.localPlayerId());
    }

    return 0;
}

}  // namespace doomlike::application

