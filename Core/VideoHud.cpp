#include "stdafx.h"
#include "VideoHud.h"
#include "RewindManager.h"
#include "CheatManager.h"
#include "ControlManager.h"
#include "BaseControlDevice.h"
#include "RewindManager.h"
#include "ControlDeviceState.h"
#include "StandardController.h"
#include "FourScore.h"
#include "Zapper.h"
#include "MovieManager.h"
#include "Console.h"
void DrawSmallFramecounter(shared_ptr<Console> console, uint32_t* rgbaBuffer, FrameInfo& frameInfo, OverscanDimensions& overscan);

void VideoHud::DrawHud(shared_ptr<Console> console, uint32_t *outputBuffer, FrameInfo frameInfo, OverscanDimensions overscan)
{
	uint32_t displayCount = 0;
	InputDisplaySettings settings = console->GetSettings()->GetInputDisplaySettings();
	
	vector<ControlDeviceState> states = console->GetControlManager()->GetPortStates();
	for(int inputPort = 0; inputPort < 4; inputPort++) {
		if((settings.VisiblePorts >> inputPort) & 0x01 || inputPort == 0) {
			if(DisplayControllerInput(console, states[inputPort], inputPort, outputBuffer, frameInfo, overscan, displayCount)) {
				displayCount++;
			}
		}
	}

	DrawSmallFramecounter(console, outputBuffer, frameInfo, overscan);
	DrawMovieIcons(console, outputBuffer, frameInfo, overscan);
}

int GetSettingColor(shared_ptr<Console> console) {
	EmulationSettings* settings = console->GetSettings();

	// check overclocking feature
	if (settings->GetPpuExtraScanlinesAfterNmi() != 0 || settings->GetPpuExtraScanlinesBeforeNmi() != 0) {
		return 2;
	}

	shared_ptr<Debugger> dbg = console->GetDebugger(false);
	if (dbg != NULL) return 2;

	if (MovieManager::Playing()) {
		return 2;
	}

	CheatManager* cheats = console->GetCheatManager();
	if (cheats->HasCheats()) return 1;

	return 0;
}

void BlendColors2(uint32_t* output, uint32_t input)
{
	uint8_t inA = (input >> 24) & 0xFF;
	uint8_t inR = (input >> 16) & 0xFF;
	uint8_t inG = (input >> 8) & 0xFF;
	uint8_t inB = input & 0xFF;

	uint8_t invertedAlpha = 255 - inA;
	uint8_t outB = (uint8_t)((inA * inB + invertedAlpha * (*output & 0xFF)) >> 8);
	uint8_t outG = (uint8_t)((inA * inG + invertedAlpha * ((*output >> 8) & 0xFF)) >> 8);
	uint8_t outR = (uint8_t)((inA * inR + invertedAlpha * ((*output >> 16) & 0xFF)) >> 8);
	*output = 0xFF000000 | (outR << 16) | (outG << 8) | outB;
}


const vector<uint32_t> _digits[0x10] = {
	{  // 0
		1, 1, 1, 1, 0,
		1, 0, 0, 1, 0,
		1, 0, 0, 1, 0,
		1, 0, 0, 1, 0,
		1, 1, 1, 1, 0,
	},
	{ // 1
		0, 1, 1, 0, 0,
		0, 0, 1, 0, 0,
		0, 0, 1, 0, 0,
		0, 0, 1, 0, 0,
		0, 1, 1, 1, 0,
	},
	{ // 2
		1, 1, 1, 1, 0,
		0, 0, 0, 1, 0,
		1, 1, 1, 1, 0,
		1, 0, 0, 0, 0,
		1, 1, 1, 1, 0,
	},
	{ // 3
		1, 1, 1, 1, 0,
		0, 0, 0, 1, 0,
		1, 1, 1, 1, 0,
		0, 0, 0, 1, 0,
		1, 1, 1, 1, 0,
	},
	{ // 4
		1, 0, 0, 1, 0,
		1, 0, 0, 1, 0,
		1, 1, 1, 1, 0,
		0, 0, 0, 1, 0,
		0, 0, 0, 1, 0,
	},
	{ // 5
		1, 1, 1, 1, 0,
		1, 0, 0, 0, 0,
		1, 1, 1, 1, 0,
		0, 0, 0, 1, 0,
		1, 1, 1, 1, 0,
	},
	{ // 6
		1, 1, 1, 1, 0,
		1, 0, 0, 0, 0,
		1, 1, 1, 1, 0,
		1, 0, 0, 1, 0,
		1, 1, 1, 1, 0,
	},
	{ // 7
		1, 1, 1, 1, 0,
		0, 0, 0, 1, 0,
		0, 0, 0, 1, 0,
		0, 0, 0, 1, 0,
		0, 0, 0, 1, 0,
	},
	{ // 8
		1, 1, 1, 1, 0,
		1, 0, 0, 1, 0,
		1, 1, 1, 1, 0,
		1, 0, 0, 1, 0,
		1, 1, 1, 1, 0,
	},
	{ // 9
		1, 1, 1, 1, 0,
		1, 0, 0, 1, 0,
		1, 1, 1, 1, 0,
		0, 0, 0, 1, 0,
		0, 0, 0, 1, 0,
	},
};

void DrawSmallFramecounter(shared_ptr<Console> console, uint32_t* rgbaBuffer, FrameInfo& frameInfo, OverscanDimensions& overscan)
{
	bool axisInverted = (console->GetSettings()->GetScreenRotation() % 180) != 0;
	InputDisplaySettings settings = console->GetSettings()->GetInputDisplaySettings();
	uint32_t yStart, xStart;
	switch (settings.DisplayPosition) {
	case InputDisplayPosition::TopLeft:
		xStart = 3;
		yStart = 5;
		break;
	case InputDisplayPosition::TopRight:
		xStart = frameInfo.Width - 40;
		yStart = 5;
		break;
	case InputDisplayPosition::BottomLeft:
		xStart = 3;
		yStart = frameInfo.Height - 15;
		break;
	default:
	case InputDisplayPosition::BottomRight:
		xStart = frameInfo.Width - 40;
		yStart = frameInfo.Height - 15;
		break;
	}

	uint32_t settingColor = GetSettingColor(console);
	int outlineColors[3] = { 0xEF111111, 0xEF117111, 0xEF711111 };
	int number = console->GetFrameCount();
	int start = 1;
	for (int i = 1; i < 6; i += 1) {
		int digit = (number / start) % 10;
		int digitX = 39 + (i * -5);
		for (int x = 0; x < 5; ++x) {
			for (int y = 0; y < 5; ++y) {
				uint32_t bufferPos = ((yStart + y) * frameInfo.Width) + (xStart + x) + digitX;
				int bitset = _digits[digit][y * 5 + x];
				if (bitset > 0) {
					BlendColors2(rgbaBuffer + bufferPos, outlineColors[settingColor]);
				}
			}
		}
		start *= 10;
	}
}

bool VideoHud::DisplayControllerInput(shared_ptr<Console> console, ControlDeviceState &state, int inputPort, uint32_t *outputBuffer, FrameInfo &frameInfo, OverscanDimensions &overscan, uint32_t displayIndex)
{
	bool axisInverted = (console->GetSettings()->GetScreenRotation() % 180) != 0;
	uint32_t* rgbaBuffer = outputBuffer;

	InputDisplaySettings settings = console->GetSettings()->GetInputDisplaySettings();
	uint32_t yStart, xStart;
	switch(settings.DisplayPosition) {
		case InputDisplayPosition::TopLeft:
			xStart = 3 + (settings.DisplayHorizontally ? displayIndex * 40 : 0);
			yStart = 5 + (settings.DisplayHorizontally ? 0 : displayIndex * 14);
			break;
		case InputDisplayPosition::TopRight:
			xStart = frameInfo.Width - 40 - (settings.DisplayHorizontally ? displayIndex * 40 : 0);
			yStart = 5 + (settings.DisplayHorizontally ? 0 : displayIndex * 14);
			break;
		case InputDisplayPosition::BottomLeft:
			xStart = 3 + (settings.DisplayHorizontally ? displayIndex * 40 : 0);
			yStart = frameInfo.Height - 15 - (settings.DisplayHorizontally ? 0 : displayIndex * 14);
			break;
		default:
		case InputDisplayPosition::BottomRight:
			xStart = frameInfo.Width - 40 - (settings.DisplayHorizontally ? displayIndex * 40 : 0);
			yStart = frameInfo.Height - 15 - (settings.DisplayHorizontally ? 0 : displayIndex * 14);
			break;
	}

	int32_t buttonState = -1;

	shared_ptr<BaseControlDevice> device = ControlManager::CreateControllerDevice(console->GetSettings()->GetControllerType(inputPort), 0, console);
	if(!device) {
		return false;
	}

	device->SetRawState(state);

	shared_ptr<StandardController> controller = std::dynamic_pointer_cast<StandardController>(device);
	if(controller) {
		buttonState = controller->ToByte();
	}

	uint32_t settingColor = GetSettingColor(console);
	int outlineColors[3] = { 0xEF111111, 0xEF117111, 0xEF711111 };

	if(buttonState >= 0) {
		for(int y = 0; y < 13; y++) {
			for(int x = 0; x < 38; x++) {
				uint32_t bufferPos = (yStart + y)*frameInfo.Width + (xStart + x);
				uint32_t gridValue = _gamePads[inputPort][y * 38 + x];
				if(gridValue > 0) {
					if((buttonState >> (gridValue - 1)) & 0x01) {
						BlendColors(rgbaBuffer + bufferPos, 0xEFFFFFFF);
					} else {
						BlendColors(rgbaBuffer + bufferPos, outlineColors[settingColor]);
					}
				} else {
					BlendColors(rgbaBuffer + bufferPos, 0xBFAAAAAA);
				}
			}
		}
		return true;
	}

	shared_ptr<Zapper> zapper = std::dynamic_pointer_cast<Zapper>(device);
	if(zapper) {
		MousePosition pos = zapper->GetCoordinates();
		if(pos.X != -1 && pos.Y != -1) {
			for(int i = -1; i <= 1; i++) {
				int y = (pos.Y - overscan.Top) + i;
				if(y < 0 || y >(int)frameInfo.Height) continue;

				for(int j = -1; j <= 1; j++) {
					int x = (pos.X - overscan.Left) + j;
					if(x < 0 || x > (int)frameInfo.Width) continue;

					uint32_t bufferPos = y*frameInfo.Width + x;
					BlendColors(rgbaBuffer + bufferPos, 0xFFFF0000);
				}
			}
		}		
	}

	return false;
}



const vector<uint32_t> _ffwdIcon = {
	3,3,3,0,0,3,3,3,0,0,0,0,0,0,0,0,0,
	3,1,1,3,3,3,1,1,3,3,0,0,0,0,0,0,0,
	3,1,2,1,1,3,1,2,1,1,3,3,0,0,0,0,0,
	3,1,2,2,2,3,1,2,2,2,1,1,3,3,0,0,0,
	3,1,2,2,2,3,1,2,2,2,2,2,1,1,3,0,0,
	3,1,2,2,2,3,1,2,2,2,2,2,2,2,1,3,0,
	3,1,2,2,2,3,1,2,2,2,2,2,2,2,1,3,0,
	3,1,2,2,2,3,1,2,2,2,2,2,1,1,3,0,0,
	3,1,2,2,2,3,1,2,2,2,1,1,3,3,0,0,0,
	3,1,2,1,1,3,1,2,1,1,3,3,0,0,0,0,0,
	3,1,1,3,3,3,1,1,3,3,0,0,0,0,0,0,0,
	3,3,3,0,0,3,3,3,0,0,0,0,0,0,0,0,0,
};

const vector<uint32_t> _rewindIcon = {
	0,0,0,0,0,0,0,0,0,3,3,3,0,0,3,3,3,
	0,0,0,0,0,0,0,3,3,1,1,3,3,3,1,1,3,
	0,0,0,0,0,3,3,1,1,2,1,3,1,1,2,1,3,
	0,0,0,3,3,1,1,2,2,2,1,3,2,2,2,1,3,
	0,0,3,1,1,2,2,2,2,2,1,3,2,2,2,1,3,
	0,3,1,2,2,2,2,2,2,2,1,3,2,2,2,1,3,
	0,3,1,2,2,2,2,2,2,2,1,3,2,2,2,1,3,
	0,0,3,1,1,2,2,2,2,2,1,3,2,2,2,1,3,
	0,0,0,3,3,1,1,2,2,2,1,3,2,2,2,1,3,
	0,0,0,0,0,3,3,1,1,2,1,3,1,1,2,1,3,
	0,0,0,0,0,0,0,3,3,1,1,3,3,3,1,1,3,
	0,0,0,0,0,0,0,0,0,3,3,3,0,0,3,3,3,
};


void VideoHud::DrawMovieIcons(shared_ptr<Console> console, uint32_t *outputBuffer, FrameInfo &frameInfo, OverscanDimensions &overscan)
{
	InputDisplaySettings settings = console->GetSettings()->GetInputDisplaySettings();
	uint32_t xOffset = settings.VisiblePorts > 0 && settings.DisplayPosition == InputDisplayPosition::TopRight ? 50 : 27;
	uint32_t* rgbaBuffer = (uint32_t*)outputBuffer;
	int scale = frameInfo.Width / overscan.GetScreenWidth();
	uint32_t yStart = 15 * scale;
	uint32_t xStart = (frameInfo.Width - xOffset) * scale;

	if (MovieManager::Playing()) {
		for (int y = 0; y < 12 * scale; y++) {
			for (int x = 0; x < 12 * scale; x++) {
				uint32_t bufferPos = (yStart + y) * frameInfo.Width + (xStart + x);
				uint32_t gridValue = _playIcon[y / scale * 12 + x / scale];
				if (gridValue == 1) {
					BlendColors(rgbaBuffer + bufferPos, 0xEF00CF00);
				}
				else if (gridValue == 2) {
					BlendColors(rgbaBuffer + bufferPos, 0xEF009F00);
				}
				else if (gridValue == 3) {
					BlendColors(rgbaBuffer + bufferPos, 0xEF000000);
				}
			}
		}
	}

	if (console->GetSettings()->CheckFlag(EmulationFlags::DisplayMovieIcons) && MovieManager::Recording()) {
		for(int y = 0; y < 12 * scale; y++) {
			for(int x = 0; x < 12 * scale; x++) {
				uint32_t bufferPos = (yStart + y)*frameInfo.Width + (xStart + x);
				uint32_t gridValue = _recordIcon[y / scale * 12 + x / scale];
				if(gridValue == 1) {
					BlendColors(rgbaBuffer + bufferPos, 0xEFCF0000);
				} else if(gridValue == 2) {
					BlendColors(rgbaBuffer + bufferPos, 0xEF9F0000);
				} else if(gridValue == 3) {
					BlendColors(rgbaBuffer + bufferPos, 0xEF000000);
				}
			}
		}
	}

	int ffwd = 0;
	shared_ptr<RewindManager> rewinder = console->GetRewindManager();
	uint32_t speed = console->GetSettings()->GetEmulationSpeed();
	uint32_t overclock1 = console->GetSettings()->GetPpuExtraScanlinesAfterNmi();
	uint32_t overclock2 = console->GetSettings()->GetPpuExtraScanlinesBeforeNmi();
	if (speed < 100 || rewinder->IsRewinding()) ffwd = -1;
	if (speed > 100 || overclock1 != 0 || overclock2 != 0) ffwd = 1;
	if (ffwd != 0) {
		int width = 17;
		int height = 12;
		for (int y = 0; y < height * scale; y++) {
			for (int x = 0; x < width * scale; x++) {
				uint32_t bufferPos = (yStart + y) * frameInfo.Width + (xStart + x);
				uint32_t gridValue = (ffwd == 1 ? _ffwdIcon : _rewindIcon)[y / scale * width + x / scale];
				if (gridValue == 1) {
					BlendColors(rgbaBuffer + bufferPos, 0xEF00CF00);
				}
				else if (gridValue == 2) {
					BlendColors(rgbaBuffer + bufferPos, 0xEF009F00);
				}
				else if (gridValue == 3) {
					BlendColors(rgbaBuffer + bufferPos, 0xEF000000);
				}
			}
		}
	}
}

void VideoHud::BlendColors(uint32_t* output, uint32_t input)
{
	uint8_t inA = (input >> 24) & 0xFF;
	uint8_t inR = (input >> 16) & 0xFF;
	uint8_t inG = (input >> 8) & 0xFF;
	uint8_t inB = input & 0xFF;

	uint8_t invertedAlpha = 255 - inA;
	uint8_t outB = (uint8_t)((inA * inB + invertedAlpha * (*output & 0xFF)) >> 8);
	uint8_t outG = (uint8_t)((inA * inG + invertedAlpha * ((*output >> 8) & 0xFF)) >> 8);
	uint8_t outR = (uint8_t)((inA * inR + invertedAlpha * ((*output >> 16) & 0xFF)) >> 8);
	*output = 0xFF000000 | (outR << 16) | (outG << 8) | outB;
}

const vector<uint32_t> VideoHud::_playIcon = {
	3,3,3,0,0,0,0,0,0,0,0,0,
	3,1,1,3,3,0,0,0,0,0,0,0,
	3,1,2,1,1,3,3,0,0,0,0,0,
	3,1,2,2,2,1,1,3,3,0,0,0,
	3,1,2,2,2,2,2,1,1,3,0,0,
	3,1,2,2,2,2,2,2,2,1,3,0,
	3,1,2,2,2,2,2,2,2,1,3,0,
	3,1,2,2,2,2,2,1,1,3,0,0,
	3,1,2,2,2,1,1,3,3,0,0,0,
	3,1,2,1,1,3,3,0,0,0,0,0,
	3,1,1,3,3,0,0,0,0,0,0,0,
	3,3,3,0,0,0,0,0,0,0,0,0,
};

const vector<uint32_t> VideoHud::_recordIcon = {
	0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,3,3,0,0,0,0,0,
	0,0,0,3,3,1,1,3,3,0,0,0,
	0,0,3,1,1,2,2,1,1,3,0,0,
	0,0,3,1,2,2,2,2,1,3,0,0,
	0,3,1,2,2,2,2,2,2,1,3,0,
	0,3,1,2,2,2,2,2,2,1,3,0,
	0,0,3,1,2,2,2,2,1,3,0,0,
	0,0,3,1,1,2,2,1,1,3,0,0,
	0,0,0,3,3,1,1,3,3,0,0,0,
	0,0,0,0,0,3,3,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,
};

const vector<uint32_t> VideoHud::_gamePads[4] = {
 { 9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,0,0,0,0,1,1,0,0,9,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,1,1,1,1,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,3,3,3,3,0,4,4,4,4,0,0,0,2,2,2,2,0,0,1,1,1,1,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,3,3,3,3,0,4,4,4,4,0,0,0,0,2,2,0,0,0,0,1,1,0,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9 },

 { 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,9,9,9,0,0,0,0,0,0,0,2,2,0,0,0,0,1,1,0,0,9,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,1,1,1,1,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,3,3,3,3,0,4,4,4,4,0,0,0,2,2,2,2,0,0,1,1,1,1,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,3,3,3,3,0,4,4,4,4,0,0,0,0,2,2,0,0,0,0,1,1,0,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9 },

 { 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,0,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,9,9,9,0,0,0,0,0,0,0,2,2,0,0,0,0,1,1,0,0,9,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,1,1,1,1,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,3,3,3,3,0,4,4,4,4,0,0,0,2,2,2,2,0,0,1,1,1,1,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,3,3,3,3,0,4,4,4,4,0,0,0,0,2,2,0,0,0,0,1,1,0,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9 },

 { 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,9,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,9,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,5,5,5,0,0,0,0,0,0,0,0,0,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,0,0,9,0,0,0,0,0,0,0,2,2,0,0,0,0,1,1,0,0,9,
	9,0,7,7,7,9,9,9,8,8,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,1,1,1,1,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,3,3,3,3,0,4,4,4,4,0,0,0,2,2,2,2,0,0,1,1,1,1,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,3,3,3,3,0,4,4,4,4,0,0,0,0,2,2,0,0,0,0,1,1,0,0,9,
	9,0,0,0,0,6,6,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9 }
};

