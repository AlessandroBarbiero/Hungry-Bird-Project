#include "BirdRed.h"

const std::string BirdRed::_name = "Red";

BirdRed::BirdRed() {

}

void BirdRed::talk() {
	Bird::talk(_name);
}