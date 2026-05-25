#include "WebViewEditor.h"
#include <BinaryData.h>

HowsTheWeatherWebViewEditor::HowsTheWeatherWebViewEditor(HowsTheWeatherAudioProcessor& p)
    : AudioProcessorEditor(p), audioProcessor(p)
{
    // STEP 1: CREATE RELAYS
    positionRelay      = std::make_unique<juce::WebSliderRelay>("position");
    sizeRelay          = std::make_unique<juce::WebSliderRelay>("size");
    densityRelay       = std::make_unique<juce::WebSliderRelay>("density");
    textureRelay       = std::make_unique<juce::WebSliderRelay>("texture");
    pitchRelay         = std::make_unique<juce::WebSliderRelay>("pitch");
    feedbackRelay      = std::make_unique<juce::WebSliderRelay>("feedback");
    dryWetRelay        = std::make_unique<juce::WebSliderRelay>("drywet");
    spreadRelay        = std::make_unique<juce::WebSliderRelay>("spread");
    reverbRelay        = std::make_unique<juce::WebSliderRelay>("reverb");
    weatherAmountRelay = std::make_unique<juce::WebSliderRelay>("weather_amount");
    weatherLatRelay    = std::make_unique<juce::WebSliderRelay>("weather_lat");
    weatherLonRelay    = std::make_unique<juce::WebSliderRelay>("weather_lon");

    freezeRelay           = std::make_unique<juce::WebToggleButtonRelay>("freeze");
    weatherEnableRelay    = std::make_unique<juce::WebToggleButtonRelay>("weather_enable");
    weatherGuiEnableRelay = std::make_unique<juce::WebToggleButtonRelay>("weather_gui_enable");

    // Preset native functions (JS → C++) — invoked via Juce.getNativeFunction(name)
    auto savePresetFn = [this](const juce::Array<juce::var>& args, auto complete) {
        if (args.size() > 0)
            audioProcessor.getPresetManager().savePreset(args[0].toString());
        complete({});
    };
    auto loadPresetFn = [this](const juce::Array<juce::var>& args, auto complete) {
        if (args.size() > 0)
            audioProcessor.getPresetManager().loadPreset(args[0].toString());
        complete({});
    };
    auto nextPresetFn = [this](const juce::Array<juce::var>&, auto complete) {
        audioProcessor.getPresetManager().loadNextPreset();
        complete({});
    };
    auto prevPresetFn = [this](const juce::Array<juce::var>&, auto complete) {
        audioProcessor.getPresetManager().loadPreviousPreset();
        complete({});
    };
    auto submitBugReportFn = [](const juce::Array<juce::var>& args, auto complete) {
        juce::String body = args.size() > 0 ? args[0].toString() : juce::String();
        juce::String fullMessage = "Plugin Version: 1.4.3\n"
                                   "Time: " + juce::Time::getCurrentTime().toString(true, true) + "\n\n"
                                   + body;
        juce::URL url = juce::URL("https://api.web3forms.com/submit")
            .withParameter("access_key", "1b0d4b20-7ec1-42e9-9644-f1d645d9d08e")
            .withParameter("subject",    "HowsTheWeather Bug Report")
            .withParameter("from_name",  "HowsTheWeather Plugin")
            .withParameter("message",    fullMessage);
        juce::Thread::launch([url]() {
            auto stream = url.createInputStream(
                juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                    .withConnectionTimeoutMs(5000));
            (void) stream; // fire-and-forget
        });
        complete({});
    };

    // STEP 2: CREATE WEBVIEW with all relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withOptionsFrom(*positionRelay)
            .withOptionsFrom(*sizeRelay)
            .withOptionsFrom(*densityRelay)
            .withOptionsFrom(*textureRelay)
            .withOptionsFrom(*pitchRelay)
            .withOptionsFrom(*feedbackRelay)
            .withOptionsFrom(*dryWetRelay)
            .withOptionsFrom(*spreadRelay)
            .withOptionsFrom(*reverbRelay)
            .withOptionsFrom(*weatherAmountRelay)
            .withOptionsFrom(*weatherLatRelay)
            .withOptionsFrom(*weatherLonRelay)
            .withOptionsFrom(*freezeRelay)
            .withOptionsFrom(*weatherEnableRelay)
            .withOptionsFrom(*weatherGuiEnableRelay)
            .withNativeFunction("savePreset", savePresetFn)
            .withNativeFunction("loadPreset", loadPresetFn)
            .withNativeFunction("nextPreset", nextPresetFn)
            .withNativeFunction("prevPreset", prevPresetFn)
            .withNativeFunction("submitBugReport", submitBugReportFn)
    );

    // STEP 3: CREATE PARAMETER ATTACHMENTS (JUCE 8: 3 params, include nullptr)
    auto& params = audioProcessor.parameters;

    positionAttachment      = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("position"), *positionRelay, nullptr);
    sizeAttachment          = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("size"), *sizeRelay, nullptr);
    densityAttachment       = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("density"), *densityRelay, nullptr);
    textureAttachment       = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("texture"), *textureRelay, nullptr);
    pitchAttachment         = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("pitch"), *pitchRelay, nullptr);
    feedbackAttachment      = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("feedback"), *feedbackRelay, nullptr);
    dryWetAttachment        = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("drywet"), *dryWetRelay, nullptr);
    spreadAttachment        = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("spread"), *spreadRelay, nullptr);
    reverbAttachment        = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("reverb"), *reverbRelay, nullptr);
    weatherAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("weather_amount"), *weatherAmountRelay, nullptr);
    weatherLatAttachment    = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("weather_lat"), *weatherLatRelay, nullptr);
    weatherLonAttachment    = std::make_unique<juce::WebSliderParameterAttachment>(*params.getParameter("weather_lon"), *weatherLonRelay, nullptr);

    freezeAttachment           = std::make_unique<juce::WebToggleButtonParameterAttachment>(*params.getParameter("freeze"), *freezeRelay, nullptr);
    weatherEnableAttachment    = std::make_unique<juce::WebToggleButtonParameterAttachment>(*params.getParameter("weather_enable"), *weatherEnableRelay, nullptr);
    weatherGuiEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(*params.getParameter("weather_gui_enable"), *weatherGuiEnableRelay, nullptr);

    // Navigate to UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    addAndMakeVisible(*webView);

    // Timer for pushing weather data to WebView (4 Hz)
    startTimerHz(4);

    setSize(600, 600);
    setResizable(false, false);
}

HowsTheWeatherWebViewEditor::~HowsTheWeatherWebViewEditor()
{
    stopTimer();
}

void HowsTheWeatherWebViewEditor::paint(juce::Graphics&)
{
    // WebView fills entire editor
}

void HowsTheWeatherWebViewEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

void HowsTheWeatherWebViewEditor::timerCallback()
{
    auto& ws = audioProcessor.getWeatherService();

    auto* obj = new juce::DynamicObject();
    obj->setProperty("valid",        ws.isDataValid());
    obj->setProperty("temp",         ws.getTemperature());
    obj->setProperty("wind",         ws.getWindSpeed());
    obj->setProperty("cloud",        ws.getCloudCover());
    obj->setProperty("pressure",     ws.getPressure());
    obj->setProperty("humidity",     ws.getHumidity());
    obj->setProperty("windNorm",     ws.getWindNorm());
    obj->setProperty("cloudNorm",    ws.getCloudNorm());
    obj->setProperty("pressureNorm", ws.getPressureNorm());
    obj->setProperty("tempNorm",     ws.getTemperatureNorm());
    obj->setProperty("humidityNorm", ws.getHumidityNorm());
    obj->setProperty("rain",         ws.getRain());
    obj->setProperty("weatherCode",  ws.getWeatherCode());
    obj->setProperty("windDirection",ws.getWindDirection());

    webView->emitEventIfBrowserIsVisible("weatherUpdate", juce::var(obj));

    // Push preset state to UI
    auto& pm = audioProcessor.getPresetManager();
    auto* presetObj = new juce::DynamicObject();
    presetObj->setProperty("current", pm.getCurrentPresetName());
    juce::Array<juce::var> list;
    for (const auto& name : pm.getAllPresets())
        list.add(juce::var(name));
    presetObj->setProperty("all", list);
    webView->emitEventIfBrowserIsVisible("presetUpdate", juce::var(presetObj));
}

static juce::WebBrowserComponent::Resource makeResource(const char* data, int size, const char* mimeType)
{
    auto* begin = reinterpret_cast<const std::byte*>(data);
    return { { begin, begin + size }, mimeType };
}

std::optional<juce::WebBrowserComponent::Resource>
HowsTheWeatherWebViewEditor::getResource(const juce::String& url)
{
    if (url == "/" || url == "/index.html")
        return makeResource(BinaryData::index_html, BinaryData::index_htmlSize, "text/html");

    if (url == "/js/juce/index.js")
        return makeResource(BinaryData::index_js, BinaryData::index_jsSize, "text/javascript");

    if (url == "/js/juce/check_native_interop.js")
        return makeResource(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize, "text/javascript");

    if (url == "/js/cobe.js")
        return makeResource(BinaryData::cobe_js, BinaryData::cobe_jsSize, "text/javascript");

    return std::nullopt;
}
