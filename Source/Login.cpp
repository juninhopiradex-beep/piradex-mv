// =============================================================================
//  LEVEL-X by Piradex — implementação do ecrã de login (TU própria)
// =============================================================================
#include <juce_gui_basics/juce_gui_basics.h>
#include "License.h"
#include "Login.h"

namespace lvlx
{
class LoginOverlay : public juce::Component
{
public:
    std::function<void()> onUnlock;

    LoginOverlay()
    {
        pw.setPasswordCharacter ((juce_wchar) 0x2022);
        pw.setJustification (juce::Justification::centred);
        pw.setTextToShowWhenEmpty ("Senha", juce::Colour (0xff6b7b8c));
        pw.onReturnKey = [this] { attempt(); };
        addAndMakeVisible (pw);

        keep.setButtonText ("Lembrar neste computador");
        keep.setToggleState (true, juce::dontSendNotification);
        addAndMakeVisible (keep);

        go.setButtonText ("Entrar");
        go.onClick = [this] { attempt(); };
        addAndMakeVisible (go);

        msg.setJustificationType (juce::Justification::centred);
        msg.setColour (juce::Label::textColourId, juce::Colour (0xffff6b6b));
        addAndMakeVisible (msg);
    }

    void attempt()
    {
        const auto r = lic::License::get().tryPassword (pw.getText(), keep.getToggleState());
        if (r == lic::License::Result::Ok) { msg.setText ({}, juce::dontSendNotification); if (onUnlock) onUnlock(); }
        else { msg.setText (r == lic::License::Result::Expired ? "Senha expirada." : "Senha incorreta.", juce::dontSendNotification); pw.clear(); }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xf20b1016));
        auto panel = getLocalBounds().withSizeKeepingCentre (440, 320).toFloat();
        g.setColour (juce::Colour (0xff141c26)); g.fillRoundedRectangle (panel, 14.0f);
        g.setColour (juce::Colour (0xff2f9fe6)); g.drawRoundedRectangle (panel, 14.0f, 1.5f);
        auto top = panel.reduced (24.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (30.0f, juce::Font::bold));
        g.drawText ("LEVEL-X", top.removeFromTop (54.0f), juce::Justification::centred);
        g.setColour (juce::Colour (0xff9fb3c8)); g.setFont (13.0f);
        g.drawText ("by Piradex  -  introduza a sua senha", top.removeFromTop (22.0f), juce::Justification::centred);
    }

    void resized() override
    {
        auto panel = getLocalBounds().withSizeKeepingCentre (440, 320).reduced (40);
        panel.removeFromTop (96);
        pw.setBounds (panel.removeFromTop (42)); panel.removeFromTop (12);
        keep.setBounds (panel.removeFromTop (26)); panel.removeFromTop (10);
        go.setBounds (panel.removeFromTop (42)); panel.removeFromTop (8);
        msg.setBounds (panel.removeFromTop (24));
    }

private:
    juce::TextEditor pw;
    juce::ToggleButton keep;
    juce::TextButton go;
    juce::Label msg;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoginOverlay)
};

std::unique_ptr<juce::Component> makeLoginOverlay (std::function<void()> onUnlock)
{
    auto c = std::make_unique<LoginOverlay>();
    c->onUnlock = std::move (onUnlock);
    return c;
}
} // namespace lvlx
