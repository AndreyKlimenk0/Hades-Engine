#include "profiling.h"

static __itt_domain *domain = NULL;

__itt_domain *get_default_domain()
{
	if (!domain) {
		domain = __itt_domain_create("Profiling");
	}
	return domain;
}
