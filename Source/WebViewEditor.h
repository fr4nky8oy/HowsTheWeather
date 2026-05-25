#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

/**
 * WebView-based Plugin Editor for HowsTheWeather
 *
 * CRITICAL: Member declaration order prevents release build crashes.
 * Destruction order (reverse of declaration):
 * 1. Attachments destroyed FIRST (stop using relays and WebView)
 * 2. WebView destroyed SECOND (safe, attachments are gone)
 * 3. Relays destroyed LAST (safe, nothing using them)
 *
 * See docs/critical-patterns.md #9 for full explanation.
 */

class HowsTheWeatherWebViewEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    HowsTheWeatherWebViewEditor(HowsTheWeatherAudioProcessor& p);
    ~HowsTheWeatherWebViewEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    std::optional<juce::WebBrowserComponent::Resource> getResource(
        const juce::String& url
    );

    HowsTheWeatherAudioProcessor& audioProcessor;

    // ========================================================================
    // CRITICAL MEMBER DECLARATION ORDER: Relays -> WebView -> Attachments
    // ========================================================================

    // 1. RELAYS FIRST (created first, destroyed last)
    // Float params (12) -> WebSliderRelay
    std::unique_ptr<juce::WebSliderRelay> positionRelay;
    std::unique_ptr<juce::WebSliderRelay> sizeRelay;
    std::unique_ptr<juce::WebSliderRelay> densityRelay;
    std::unique_ptr<juce::WebSliderRelay> textureRelay;
    std::unique_ptr<juce::WebSliderRelay> pitchRelay;
    std::unique_ptr<juce::WebSliderRelay> feedbackRelay;
    std::unique_ptr<juce::WebSliderRelay> dryWetRelay;
    std::unique_ptr<juce::WebSliderRelay> spreadRelay;
    std::unique_ptr<juce::WebSliderRelay> reverbRelay;
    std::unique_ptr<juce::WebSliderRelay> weatherAmountRelay;
    std::unique_ptr<juce::WebSliderRelay> weatherLatRelay;
    std::unique_ptr<juce::WebSliderRelay> weatherLonRelay;

    // Bool params (3) -> WebToggleButtonRelay
    std::unique_ptr<juce::WebToggleButtonRelay> freezeRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> weatherEnableRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> weatherGuiEnableRelay;

    // 2. WEBVIEW SECOND
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. PARAMETER ATTACHMENTS LAST (created last, destroyed first)
    std::unique_ptr<juce::WebSliderParameterAttachment> positionAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> densityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> textureAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> feedbackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dryWetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverbAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> weatherAmountAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> weatherLatAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> weatherLonAttachment;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> freezeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> weatherEnableAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> weatherGuiEnableAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HowsTheWeatherWebViewEditor)
};
