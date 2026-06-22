
#ifdef APP
#include "app.hpp"
#define MODULE_NAME APP
#endif


int main(void)
{
	static App firmware(MODULE_NAME);
	if (firmware.start())
		Error_Handler();
}
