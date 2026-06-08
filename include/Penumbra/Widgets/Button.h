#pragma once

#include "Penumbra/Widgets/Box.h"

#include <functional>

namespace Penumbra::Widgets {

// A Button is a Box plus click handling and a state-driven background fill. It has
// no built-in label: a text button is a Button with one Label child (Milestone 5).
class Button : public Box {
public:
    // Extra state colours (the box-model + default background live in Box::Style).
    SDL_Color ColorBackgroundHovered{0, 0, 0, 0};
    SDL_Color ColorBackgroundPressed{0, 0, 0, 0};
    SDL_Color ColorBackgroundDisabled{0, 0, 0, 0};

    std::function<void()> OnClicked;

    // Pours a resolved ButtonStyle into this widget: the BoxStyle slice (box model
    // + default background) into Box::Style, the state colours into our own fields.
    // ColorLabel is intentionally ignored — the resolver applies it to a Label child.
    void ApplyStyle(const ButtonStyle& Style);

    bool UpdateInteractionState(const Platform::InputState&) override;
    void Draw(Render::Renderer&) override;

    InteractionState GetInteractionState() const { return CurrentState; }

private:
    SDL_Color BackgroundForState() const;

    InteractionState CurrentState{InteractionState::Default};
    bool             PressedInside{false}; // a press began on this button and is held
};

} // namespace Penumbra::Widgets
