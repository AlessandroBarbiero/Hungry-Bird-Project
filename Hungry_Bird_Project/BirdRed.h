#pragma once
#include "Bird.h"

class BirdRed : public Bird
{
	public:
		BirdRed();
		void talk();
	private:
		static const std::string _name;
	//	static Model _model;
	//	static Texture _texture;
	//	DescriptorSet _dSet;
};

