#include <am.h>
#include <klib.h>

#include <mgba/core/core.h>
#include <mgba/gba/core.h>

int main(const char *args) {
	UNUSED(args);

	struct mCore* core = GBACoreCreate();
	assert(core);
	assert(core->init(core));

	printf("mGBA AM scaffold initialized.\n");

	core->deinit(core);
	return 0;
}
