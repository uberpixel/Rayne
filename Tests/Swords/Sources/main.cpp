#include <Rayne.h>
#include "SGApplication.h"


int main(int argc, const char *argv[])
{
	RN::Initialize(argc, argv, new SG::Application());
}
