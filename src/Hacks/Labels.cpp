#include "Labels.h"
#include "../ConstData.h"
#include "../GUI/GUI.h"
#include "../JsonHacks/JsonHacks.h"

Labels::Label Labels::setupLabel(std::string labelSettingName,
								 const std::function<void(cocos2d::CCLabelBMFont*)>& function,
								 cocos2d::CCLayer* playLayer)
{
	int opacity = Settings::get<int>("labels/" + labelSettingName + "/opacity", 150);

	auto label = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
	label->setZOrder(1000);
	label->setScale(0.4f);
	label->setOpacity(opacity);

	Label l;
	l.pointer = label;
	l.settingName = labelSettingName;
	l.function = function;

	labels.push_back(l);
	playLayer->addChild(l.pointer);

	return l;
}

bool __fastcall Labels::playLayerInitHook(cocos2d::CCLayer* self, void*, void* level, bool idk1, bool idk2)
{
	labels.clear();
	bool res = playLayerInit(self, level, idk1, idk2);

	setupLabel(
		"Framerate",
		[&](cocos2d::CCLabelBMFont* pointer) {
			pointer->setString(std::to_string((int)ImGui::GetIO().Framerate).c_str());
		},
		self);

	setupLabel(
		"CPS",
		[&](cocos2d::CCLabelBMFont* pointer) {
			float currentTime = GetTickCount();
			for (int i = 0; i < clicks.size(); i++)
			{
				if ((currentTime - clicks[i]) / 1000.0f >= 1)
					clicks.erase(clicks.begin() + i);
			}
			pointer->setString(
				cocos2d::CCString::createWithFormat("%i/%i CPS", clicks.size(), totalClicks)->getCString());

			if (click)
			{
				float clickColor[3];
				clickColor[0] = Settings::get<float>("labels/CPS/color/r", 1) * 255.f;
				clickColor[1] = Settings::get<float>("labels/CPS/color/g", 0.2f) * 255.f;
				clickColor[2] = Settings::get<float>("labels/CPS/color/b", 0.2f) * 255.f;

				pointer->setColor({(GLubyte)clickColor[0], (GLubyte)clickColor[1], (GLubyte)clickColor[2]});
			}
			else
			{
				pointer->setColor({255, 255, 255});
			}
		},
		self);

	setupLabel(
		"Time",
		[&](cocos2d::CCLabelBMFont* pointer) {
			auto t = std::time(nullptr);
			auto tm = *std::localtime(&t);
			std::ostringstream s;

			bool h12 = Settings::get<bool>("labels/Time/12h", false);

			if (!h12)
				s << std::put_time(&tm, "%H:%M:%S");
			else
				s << std::put_time(&tm, "%I:%M:%S %p");

			pointer->setString(s.str().c_str());
		},
		self);

	setupLabel(
		"Noclip Accuracy",
		[&](cocos2d::CCLabelBMFont* pointer) {
			float p = (float)(frames - deaths) / (float)frames;
			float acc = p * 100.f;
			float limit = Settings::get<float>("labels/Noclip Accuracy/limit", 0.f);

			if (!dead)
			{
				if (acc <= limit)
				{
					JsonHacks::toggleHack(JsonHacks::player, 0, true);
					playLayerDestroyPlayer(Common::getBGL(), nullptr, nullptr);
					JsonHacks::toggleHack(JsonHacks::player, 0, true);

					dead = true;
				}
				pointer->setString(frames == 0
									   ? "Accuracy: 100%"
									   : cocos2d::CCString::createWithFormat("Accuracy: %.2f%%", acc)->getCString());
			}
		},
		self);

	calculatePositions();

	labelsCreated = true;

	return res;
}

void __fastcall Labels::playLayerDestroyPlayerHook(cocos2d::CCLayer* self, void*, void* player, void* object)
{
	if (totalDelta > 0.1f)
		deaths++;
	playLayerDestroyPlayer(self, player, object);
}

int __fastcall Labels::playLayerResetLevelHook(void* self, void*)
{
	dead = false;
	totalClicks = 0;
	frames = 0;
	deaths = 0;
	totalDelta = 0;
	clicks.clear();
	return playLayerResetLevel(self);
}

bool __fastcall Labels::playerObjectPushButtonHook(void* self, void*, int btn)
{
	if (!clickRegistered)
	{
		clicks.push_back(GetTickCount());
		clickRegistered = true;
	}
	click = true;
	totalClicks++;
	return playerObjectPushButton(self, btn);
}

bool __fastcall Labels::playerObjectReleaseButtonHook(void* self, void*, int btn)
{
	click = false;
	return playerObjectReleaseButton(self, btn);
}

void __fastcall Labels::playLayerPostUpdateHook(cocos2d::CCLayer* self, void*, float dt)
{
	if (labelsCreated)
	{
		for (Label& l : labels)
			l.process();
	}

	clickRegistered = false;

	frames++;

	if (*((double*)Common::getBGL() + 1411) > 0)
		totalDelta += dt;

	playLayerPostUpdate(self, dt);
}

void Labels::calculatePositions()
{
	auto size = cocos2d::CCDirector::sharedDirector()->getWinSize();

	tl.clear();
	tr.clear();
	bl.clear();
	br.clear();

	for (Label& l : labels)
	{
		int position = Settings::get<int>("labels/" + l.settingName + "/position", 0);

		switch (position)
		{
		case 0:
			l.pointer->setAnchorPoint({0.f, 0.5f});
			tl.push_back(l);
			break;
		case 1:
			l.pointer->setAnchorPoint({1.f, 0.5f});
			tr.push_back(l);
			break;
		case 2:
			l.pointer->setAnchorPoint({0.f, 0.5f});
			bl.push_back(l);
			break;
		case 3:
			l.pointer->setAnchorPoint({1.f, 0.5f});
			br.push_back(l);
			break;
		}
	}

	size_t counter = 0;
	for (auto label : tl)
	{
		if (Settings::get<bool>("labels/" + label.settingName + "/enabled", false))
		{
			label.pointer->setPositionX(5.f);
			label.pointer->setPositionY(size.height - 10 - (15 * counter));
			counter++;
		}
		else
			label.pointer->setPositionX(-500.f);
	}

	counter = 0;
	for (auto label : tr)
	{
		if (Settings::get<bool>("labels/" + label.settingName + "/enabled", false))
		{
			label.pointer->setPositionX(size.width - 5.f);
			label.pointer->setPositionY(size.height - 10 - (15 * counter));
			counter++;
		}
		else
			label.pointer->setPositionX(-500.f);
	}

	counter = 0;
	for (auto label : bl)
	{
		if (Settings::get<bool>("labels/" + label.settingName + "/enabled", false))
		{
			label.pointer->setPositionX(5.f);
			label.pointer->setPositionY(10 + (15 * counter));
			counter++;
		}
		else
			label.pointer->setPositionX(-500.f);
	}

	counter = 0;
	for (auto label : br)
	{
		if (Settings::get<bool>("labels/" + label.settingName + "/enabled", false))
		{
			label.pointer->setPositionX(size.width - 5.f);
			label.pointer->setPositionY(10 + (15 * counter));
			counter++;
		}
		else
			label.pointer->setPositionX(-500.f);
	}
}

void Labels::initHooks()
{
	MH_CreateHook(reinterpret_cast<void*>(utils::gd_base + 0x2DA660), playLayerInitHook,
				  reinterpret_cast<void**>(&playLayerInit));

	MH_CreateHook(reinterpret_cast<void*>(utils::gd_base + 0x2E5310), playLayerPostUpdateHook,
				  reinterpret_cast<void**>(&playLayerPostUpdate));

	MH_CreateHook(reinterpret_cast<void*>(utils::gd_base + 0x2D0060), playerObjectPushButtonHook,
				  reinterpret_cast<void**>(&playerObjectPushButton));

	MH_CreateHook(reinterpret_cast<void*>(utils::gd_base + 0x2D02A0), playerObjectReleaseButtonHook,
				  reinterpret_cast<void**>(&playerObjectReleaseButton));

	MH_CreateHook(reinterpret_cast<void*>(utils::gd_base + 0x2E8200), playLayerResetLevelHook,
				  reinterpret_cast<void**>(&playLayerResetLevel));

	MH_CreateHook(reinterpret_cast<void*>(utils::gd_base + 0x2E4840), playLayerDestroyPlayerHook,
				  reinterpret_cast<void**>(&playLayerDestroyPlayer));
}

void Labels::settingsForLabel(std::string labelSettingName, std::function<void()> extraSettings)
{
	if (GUI::checkbox(labelSettingName.c_str(), Settings::get<bool*>("labels/" + labelSettingName + "/enabled", false)))
		calculatePositions();

	GUI::arrowButton("Settings##" + labelSettingName);

	GUI::modalPopup("Settings##" + labelSettingName, [&] {
		int position = Settings::get<int>("labels/" + labelSettingName + "/position", 0); // 0 tl 1 tr 2 bl 3 br
		if (GUI::combo("Position##" + labelSettingName, &position, positions, 4))
		{
			Settings::set<int>("labels/" + labelSettingName + "/position", position);
			calculatePositions();
		}

		int opacity = Settings::get<int>("labels/" + labelSettingName + "/opacity", 150);

		if (GUI::inputInt("Opacity##" + labelSettingName, &opacity, 10, 255))
			Settings::set<int>("labels/" + labelSettingName + "/opacity", opacity);

		extraSettings();
	});
}

void Labels::renderWindow()
{
	settingsForLabel("Framerate", [] {});
	settingsForLabel("CPS", [] {
		float clickColor[3];
		clickColor[0] = Settings::get<float>("labels/CPS/color/r", 1.f);
		clickColor[1] = Settings::get<float>("labels/CPS/color/g", 0.f);
		clickColor[2] = Settings::get<float>("labels/CPS/color/b", 0.f);

		if (GUI::colorEdit("Click color", clickColor))
		{
			Settings::set<float>("labels/CPS/color/r", clickColor[0]);
			Settings::set<float>("labels/CPS/color/g", clickColor[1]);
			Settings::set<float>("labels/CPS/color/b", clickColor[2]);
		}
	});
	settingsForLabel("Time", [] { GUI::checkbox("Use 12h format", Settings::get<bool*>("labels/Time/12h", false)); });
	settingsForLabel("Noclip Accuracy", [] {
		float limit = Settings::get<float>("labels/Noclip Accuracy/limit", 0.f);

		if (GUI::inputFloat("Limit", &limit, 0, 100))
			Settings::set<float>("labels/Noclip Accuracy/limit", limit);
	});
}