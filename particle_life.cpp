
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#include "olcUTIL_QuadTree.h"
#include "olcUTIL_Geometry2D.h"

#include <random>
// Override base class with your custom functionality
class Particle_Life : public olc::PixelGameEngine
{
public:
	Particle_Life()
	{
		// Name your application
		sAppName = "Particle Life";
	}

	//set some values
	float iCount = 0.0f;
	int nParticles = 1200;
	float fOffset = 50.0f;
	olc::vf2d vTopLeft = { fOffset, fOffset };
	float fXDif = 900.0f - fOffset;
	float fYDif = 900.0f - fOffset;
	olc::vf2d vBottomRight = { fXDif, fYDif};
	float fWidth = vBottomRight.x - vTopLeft.x;
	float fHeight = vBottomRight.y - vTopLeft.y;
	float fTimer = 0.0f;
	int nCounter = 0;
	olc::Pixel tint = olc::WHITE;
	//set screen area to match set up in main()
	olc::utils::geom2d::rect<float> rScreen = { vTopLeft, vBottomRight };
	olc::vf2d vForce = { 0.0f, 0.0f };
	olc::vf2d vSep = { 0.0f, 0.0f };
	olc::vf2d vDiag = { 0.0f, 0.0f };
	olc::vf2d vAccel = { 0.0f, 0.0f };
	float beta = 0.5f;
	float f = 0.0f;
	float frictionHalfLife = 0.4f;
	float dT = 0.15f;
	float radEff = 0.0f;
	float rad = 0.0f;
	//calculate friction factor
	float fFriction = pow(0.5f, dT / frictionHalfLife);

	// vectors to hold particle colors
	std::vector<std::string> vColor = { "RED", "GREEN", "YELLOW", "BLUE" };
	std::vector<olc::Pixel> vColors = { olc::RED, olc::GREEN, olc::YELLOW, olc::BLUE };

	// attraction factors
	std::unordered_map<std::string, float> mRedfactors = { {"RED", -0.295f}, {"GREEN", 0.31f}, {"YELLOW", -0.63f}, {"BLUE", -0.09f} };
	std::unordered_map<std::string, float> mGreenfactors = { {"RED", 0.04f}, {"GREEN", 0.04f}, {"YELLOW", -0.685f}, {"BLUE", -0.50f} };
	std::unordered_map<std::string, float> mYellowfactors = { {"RED", -0.63f}, {"GREEN", 0.365f}, {"YELLOW", -0.365f}, {"BLUE", -0.34f} };
	std::unordered_map<std::string, float> mBluefactors = { {"RED", -0.53f}, {"GREEN", 0.685f}, {"YELLOW", 0.18f}, {"BLUE", -0.50f} };
	std::unordered_map<std::string, std::unordered_map<std::string, float>> mAllfactors = { {"RED", mRedfactors}, {"GREEN", mGreenfactors}, {"YELLOW", mYellowfactors}, {"BLUE", mBluefactors} };

	// influence ranges
	std::unordered_map<std::string, float> mRedranges = { {"RED", 84.0f}, {"GREEN", 305.0f}, {"YELLOW", 431.0f}, {"BLUE", 201.0f} };
	std::unordered_map<std::string, float> mGreenranges = { {"RED", 67.0f}, {"GREEN", 169.0f}, {"YELLOW", 42.0f}, {"BLUE", 114.0f} };
	std::unordered_map<std::string, float> mYellowranges = { {"RED", 122.0f}, {"GREEN", 48.0f}, {"YELLOW", 46.0f}, {"BLUE", 73.0f} };
	std::unordered_map<std::string, float> mBlueranges = { {"RED", 251.0f}, {"GREEN", 175.0f}, {"YELLOW", 128.0f}, {"BLUE", 71.0f} };
	std::unordered_map<std::string, std::unordered_map<std::string, float>> mAllranges = { {"RED", mRedranges}, {"GREEN", mGreenranges}, {"YELLOW", mYellowranges}, {"BLUE", mBlueranges} };

	struct sParticle
	{
		olc::vf2d vPos;
		olc::vf2d vVel;
		olc::vf2d vSize;
		olc::Pixel pColor;
		std::string sColor;
		std::unordered_map<std::string, float> mFactors;
		std::unordered_map<std::string, float> mRanges;
	};

	olc::utils::QuadTreeContainer<sParticle> treeParticles;
	std::vector<olc::Renderable> vParticles;

public:
	bool OnUserCreate() override
	{
		// make rand() random(ish)
		srand(time(NULL));
		std::default_random_engine generator;
		std::uniform_int_distribution<int> distribution(0, 3);

		// Called once at the start, so create things here
		//random number (float) between limits
		auto rand_float = [](const float a, const float b) {

			return float(rand()) / float(RAND_MAX) * (b - a) + a;

		};

		// first adjust the factors
		for (auto& i : mAllfactors)
			for (auto& j : i.second)
				j.second *= rand_float(0.5f, 1.5f); 
		
		// and adjust the ranges
		for (auto& i : mAllranges)
			for (auto& j : i.second)
				j.second *= rand_float(0.5f, 1.0f); 

		// create the particles
		//for (int i = 0; i < nParticles; i++)
			for (int i = nParticles; i--;)
		{
			sParticle particle;
			particle.vPos = { rand_float(0.0f, fWidth) + fOffset, rand_float(0.0f, fHeight) + fOffset };
			particle.vVel = { 0.0f, 0.0f };
			particle.vSize = { 3.0f, 3.0f };
			//int fIndex = rand_int(0, 4);
			int fIndex = distribution(generator);
			particle.sColor = vColor[fIndex];
			particle.pColor = vColors[fIndex];
			particle.mFactors = mAllfactors[particle.sColor];
			particle.mRanges = mAllranges[particle.sColor];
			treeParticles.insert(particle, { particle.vPos, particle.vSize });
		}

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{

		//start measuring time
		auto begin = std::chrono::high_resolution_clock::now();

		// Called once per frame
		Clear(olc::BLACK);
		
		//update particles
		if (IsFocused())
		{
			for (auto particle_it = treeParticles.begin(); particle_it != treeParticles.end(); ++particle_it)
			{
				auto& b = particle_it->item;
				vForce = { 0.0f, 0.0f };
				vAccel = { 0.0f, 0.0f };
				std::unordered_map<std::string, float>::iterator it = b.mRanges.begin();

				while (it != b.mRanges.end()) {
					radEff = it->second;
					rad = radEff * 2.0f;
					std::string  thisColor = it->first;
					olc::vf2d vEffectArea = { rad, rad };
					olc::utils::geom2d::rect<float> rEffect = { b.vPos - vEffectArea / 2.0f, vEffectArea };
					auto affected = treeParticles.search(rEffect);
					float fAreax = fWidth + fOffset - rad;
					float fAreay = fHeight - fOffset - rad;
					if (!olc::utils::geom2d::contains(rScreen, rEffect)) {
						if (rEffect.pos.x < fOffset)
						{
							rEffect.pos.x += fWidth;
							auto ptAffected = treeParticles.search(rEffect);
							affected.splice(affected.end(), move(ptAffected), ptAffected.begin(), ptAffected.end());
							rEffect.pos.x -= fWidth;
						}
						if (rEffect.pos.x > fAreax)
						{
							rEffect.pos.x -= fWidth;
							auto ptAffected = treeParticles.search(rEffect);
							affected.splice(affected.end(), move(ptAffected), ptAffected.begin(), ptAffected.end());
							rEffect.pos.x += fWidth;
						}
						if (rEffect.pos.y < fOffset)
						{
							rEffect.pos.y += fHeight;
							auto ptAffected = treeParticles.search(rEffect);
							affected.splice(affected.end(), move(ptAffected), ptAffected.begin(), ptAffected.end());
							rEffect.pos.y -= fHeight;
						}
						if (rEffect.pos.y > fAreay)
						{
							rEffect.pos.y -= fHeight;
							auto ptAffected = treeParticles.search(rEffect);
							affected.splice(affected.end(), move(ptAffected), ptAffected.begin(), ptAffected.end());
							rEffect.pos.y += fHeight;
						}
						if (rEffect.pos.x < fOffset && rEffect.pos.y < fOffset)
						{
							rEffect.pos.x += fWidth;
							rEffect.pos.y += fHeight;
							auto ptAffected = treeParticles.search(rEffect);
							affected.splice(affected.end(), move(ptAffected), ptAffected.begin(), ptAffected.end());
							rEffect.pos.x -= fWidth;
							rEffect.pos.y -= fHeight;
						}
						if (rEffect.pos.x > fAreax && rEffect.pos.y > fAreay)
						{
							rEffect.pos.x -= fWidth;
							rEffect.pos.y -= fHeight;
							auto ptAffected = treeParticles.search(rEffect);
							affected.splice(affected.end(), move(ptAffected), ptAffected.begin(), ptAffected.end());
							rEffect.pos.x += fWidth;
							rEffect.pos.y += fHeight;
						}
					}

					for (auto& part : affected)
					{
						auto& ptother = part->item;
						if (ptother.sColor != thisColor) continue;
						if (ptother.vPos != b.vPos)
						{
							//treeParticles.remove(part);
							//vDiag =  rEffect.middle() - rEffect.pos;
							//float fDiag = vDiag.mag();
							float fDiag = radEff * 1.414f;

							vSep = b.vPos - ptother.vPos;
							float sep = vSep.mag();
							if (sep > radEff && sep < fDiag) continue;
							//iCount++;
							//std::cout << vSep.x << ", " << vSep.y << std::endl;
							if (abs(vSep.x) > radEff)
							{
								//std::cout << part->item.vPos.x << ", " << part->item.vPos.y << ", " << b.vPos.x << ", " << b.vPos.y << ", " << vSep.x << ", " << vSep.y << ", ";
								vSep.x -= (fWidth)*copysignf(1.0, vSep.x);

							}
							if (abs(vSep.y) > radEff)
							{
								vSep.y -= (fHeight)*copysignf(1.0, vSep.y);
							}
							//std::cout << vSep.x << ", " << vSep.y << ", " << radEff << std::endl;
							sep = vSep.mag();
							if (sep > radEff) continue;
							float rdash = sep / radEff;
							f = b.mFactors[ptother.sColor];
							if (rdash < beta)
							{
								f = -(rdash / beta - 1);
							}
							else if (beta < rdash && rdash < 1.0f)
							{
								f = f * (1.0f - abs(2.0f * rdash - 1.0f - beta) / (1.0f - beta));
							}
							else
							{
								f = 0.0f;
							}
							float g = 1.0f * f / (sep);
							vAccel += vSep * g;
						}
					}
					it++;
				}
				b.vVel = b.vVel * fFriction + vAccel * dT;
				b.vPos += b.vVel * dT;

				//wrap particles around
				if (b.vPos.x > fWidth + fOffset)
				{
					b.vPos.x -= fWidth;
				}
				if (b.vPos.x < fOffset) b.vPos.x += fWidth;
				if (b.vPos.y > fHeight + fOffset) b.vPos.y -= fHeight;
				if (b.vPos.y < fOffset) b.vPos.y += fHeight;

			}
		}

		// move particles
		for (auto particle_it = treeParticles.begin(); particle_it != treeParticles.end(); ++particle_it)
		{
			auto& b = particle_it->item;
			treeParticles.relocate(particle_it, { b.vPos, b.vSize });
		}
		//draw particles
		for (const auto& particle : treeParticles.search(rScreen))
		{
			FillRectDecal(particle->item.vPos, particle->item.vSize, particle->item.pColor);
		}
		DrawRect(50, 50, 800, 800,  olc::WHITE);
		
		// Stop measuring time and calculate the elapsed time
		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
		std::chrono::duration<double> elapsed_seconds = end - begin;
		printf("Time measured: %.5f seconds.\n", elapsed.count() * 1e-9);
		std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";

		return true;
	}

};

int main()
{
	Particle_Life demo;
	

	if (demo.Construct(900, 900, 1, 1))
		demo.Start();
	return 0;
}
