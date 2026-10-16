#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/file.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace geode::prelude;
namespace fs = std::filesystem;

static fs::path levelsDir() {
    auto dir = Mod::get()->getSaveDir() / "levels";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}

// достаём <s>...</s> после <k>key</k> из .gmd (работает и с <dict>, и с <d>)
static std::string gmdValue(std::string const& src, std::string const& key) {
    auto tag = "<k>" + key + "</k>";
    auto p = src.find(tag);
    if (p == std::string::npos) return "";
    p = src.find("<s>", p);
    if (p == std::string::npos) return "";
    p += 3;
    auto e = src.find("</s>", p);
    if (e == std::string::npos) return "";
    return src.substr(p, e - p);
}

static void playGmd(fs::path const& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return void(FLAlertLayer::create("Ошибка", "Не удалось открыть файл.", "OK")->show());
    std::stringstream ss;
    ss << in.rdbuf();
    auto data = ss.str();

    auto name = gmdValue(data, "k2");
    auto lvlStr = gmdValue(data, "k4");
    if (lvlStr.empty()) {
        return void(FLAlertLayer::create("Ошибка", "В файле нет данных уровня (k4).", "OK")->show());
    }

    auto level = GJGameLevel::create();
    level->m_levelName = name.empty() ? path.stem().string() : name;
    level->m_levelString = lvlStr;
    level->m_levelType = GJLevelType::Editor;
    level->m_levelID = 0;

    auto scene = PlayLayer::scene(level, false, false);
    CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, scene));
}

class LevelsPopup : public Popup<> {
protected:
    bool setup() override {
        this->setTitle("Levels Folder");

        auto size = m_mainLayer->getContentSize();
        auto dir = levelsDir();

        std::vector<fs::path> files;
        std::error_code ec;
        for (auto& e : fs::directory_iterator(dir, ec)) {
            if (e.is_regular_file() && e.path().extension() == ".gmd") files.push_back(e.path());
        }
        std::sort(files.begin(), files.end());

        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        m_mainLayer->addChild(menu);

        if (files.empty()) {
            auto lbl = CCLabelBMFont::create("Папка пуста.\nПоложи .gmd файлы\nв открытую папку.", "bigFont.fnt");
            lbl->setScale(0.45f);
            lbl->setAlignment(kCCTextAlignmentCenter);
            lbl->setPosition({size.width / 2, size.height / 2 + 10});
            m_mainLayer->addChild(lbl);
        }

        // список (до 6 уровней на странице — для простоты)
        float y = size.height - 55.f;
        int shown = 0;
        for (auto& f : files) {
            if (shown++ >= 6) break;
            auto spr = ButtonSprite::create(f.stem().string().c_str(), 220, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.6f);
            auto btn = CCMenuItemExt::createSpriteExtra(spr, [f](CCMenuItemSpriteExtra*) {
                playGmd(f);
            });
            btn->setPosition({size.width / 2, y});
            menu->addChild(btn);
            y -= 34.f;
        }
        if (files.size() > 6) {
            auto lbl = CCLabelBMFont::create(fmt::format("+{} не показано", files.size() - 6).c_str(), "chatFont.fnt");
            lbl->setScale(0.6f);
            lbl->setPosition({size.width / 2, 40.f});
            m_mainLayer->addChild(lbl);
        }

        auto openSpr = ButtonSprite::create("Открыть папку", 150, true, "goldFont.fnt", "GJ_button_02.png", 28.f, 0.6f);
        auto openBtn = CCMenuItemExt::createSpriteExtra(openSpr, [dir](CCMenuItemSpriteExtra*) {
            file::openFolder(dir);
        });
        openBtn->setPosition({size.width / 2, 20.f});
        menu->addChild(openBtn);

        return true;
    }

public:
    static LevelsPopup* create() {
        auto ret = new LevelsPopup();
        if (ret->initAnchored(300.f, 280.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(LFMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto spr = CircleButtonSprite::createWithSpriteFrameName(
            "folderIcon_001.png", 1.f, CircleBaseColor::Green, CircleBaseSize::Medium);
        auto btn = CCMenuItemExt::createSpriteExtra(spr, [](CCMenuItemSpriteExtra*) {
            LevelsPopup::create()->show();
        });
        btn->setID("levels-folder-button"_spr);

        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        }
        return true;
    }
};
